#include "codec_api.h"
#include "codec_def.h"
#include "codec_app_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include <emscripten.h>
#include <time.h>

extern "C"
void openh264_log(void * ctx, int level, const char * msg)
{
    emscripten_log(EM_LOG_CONSOLE, "openh264 trace %d %s\n", level, msg);
}

extern "C"
void * open_decoder(void)
{
    ISVCDecoder* decoder_ = NULL;
    if ( WelsCreateDecoder (&decoder_) ) {
        emscripten_log(EM_LOG_CONSOLE, "Create Decoder failed\n");
        return NULL;
    }
    if (! decoder_) {
        emscripten_log(EM_LOG_CONSOLE, "Create Decoder failed (no handle)\n");
        return NULL;
    }

    WelsTraceCallback cb = openh264_log;
    int32_t tracelevel = 3;//0x7fffffff;
    if ( decoder_->SetOption(DECODER_OPTION_TRACE_CALLBACK, (void*)&cb) )
    {
        emscripten_log(EM_LOG_CONSOLE, "SetOption failed\n");
    }
    if ( decoder_->SetOption(DECODER_OPTION_TRACE_LEVEL, &tracelevel) )
    {
        emscripten_log(EM_LOG_CONSOLE, "SetOption failed\n");
    }

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    if ( decoder_->Initialize (&decParam) ) {
        emscripten_log(EM_LOG_CONSOLE, "initialize failed\n");
        return NULL;
    }

    return decoder_;
}

extern "C"
void close_decoder(void * dec)
{
    ISVCDecoder* decoder_ = (ISVCDecoder*)dec;
    if (decoder_ != NULL) {
        decoder_->Uninitialize();
        WelsDestroyDecoder (decoder_);
    }
}

extern "C"
int decode_nal(void * dec, unsigned char const * nal, size_t nalsz)
{
    ISVCDecoder* decoder_ = (ISVCDecoder*)dec;
    uint8_t* data[3];
    SBufferInfo bufInfo;
    memset (data, 0, sizeof (data));
    memset (&bufInfo, 0, sizeof (SBufferInfo));
    if (nalsz <= 0) {
        int32_t iEndOfStreamFlag = 1;
        decoder_->SetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);
        nalsz = 0;
        nal = NULL;
    }


    DECODING_STATE rv = decoder_->DecodeFrame2 (nal, (int) nalsz, data, &bufInfo);
    if (rv == 0 && bufInfo.iBufferStatus) {
        EM_ASM_({
            frame_callback($0, $1, $2, $3, $4, $5, $6);
        },
            data[0],
            data[1],
            data[2],
            bufInfo.UsrData.sSystemBuffer.iWidth,
            bufInfo.UsrData.sSystemBuffer.iHeight,
            bufInfo.UsrData.sSystemBuffer.iStride[0],
            bufInfo.UsrData.sSystemBuffer.iStride[1]
        );
        return 1;
    }
    else if (rv != 0) {
        char statusstr[100] = {0};
        if (rv & dsFramePending) strcat(statusstr,",FramePending");
        if (rv & dsRefLost) strcat(statusstr,",RefLost");
        if (rv & dsBitstreamError) strcat(statusstr,",BitstreamError");
        if (rv & dsDepLayerLost) strcat(statusstr,",DepLayerLost");
        if (rv & dsNoParamSets) strcat(statusstr,",NoParamSets");
        if (rv & dsDataErrorConcealed) strcat(statusstr,",DataErrorConcealed");
        if (rv & dsInvalidArgument) strcat(statusstr,",InvalidArgument");
        if (rv & dsInitialOptExpected) strcat(statusstr,",InitialOptExcpected");
        if (rv & dsOutOfMemory) strcat(statusstr,",OutOfMemory");
        if (rv & dsDstBufNeedExpan) strcat(statusstr,",DstBufNeedExpan");
        emscripten_log(EM_LOG_CONSOLE, "Decode failed: %#x - %s\n", rv, statusstr);
        return -1;
    } else
        return 0;

//     emscripten_log(EM_LOG_CONSOLE, " frame ready:%d\n"
//            " BsTimeStamp:%llu\n"
//            " OutYuvTimeS:%llu\n"
//            "    Width   :%d\n"
//            "    Height  :%d\n"
//            "    Format  :%d\n"
//            "    Stride  :%d %d\n"
//             ,bufInfo.iBufferStatus
//             ,bufInfo.uiInBsTimeStamp
//             ,bufInfo.uiOutYuvTimeStamp
//             ,bufInfo.UsrData.sSystemBuffer.iWidth
//             ,bufInfo.UsrData.sSystemBuffer.iHeight
//             ,bufInfo.UsrData.sSystemBuffer.iFormat
//             ,bufInfo.UsrData.sSystemBuffer.iStride[0]
//             ,bufInfo.UsrData.sSystemBuffer.iStride[1]
//           );
}

ssize_t getnal(unsigned char const * mmaped, size_t offs, size_t sz)
{
    size_t ofs = offs+3;
    int zerocnt = 0;

    if (offs >= sz) return -1;

    for(;ofs<sz;ofs++) {
        switch (mmaped[ofs]) {
            case 0:
                zerocnt++;
                break;
            case 1:
                if (zerocnt == 3)
                    return ofs - offs - 4 + 1;
            default:
                zerocnt = 0;
        }
    }
    return ofs - offs;
}

extern "C"
int decode_h264buffer(void * h, unsigned char const * nalbuf, size_t sz)
{
    size_t ofs = 0;
    ssize_t nalsz;

    int framecnt = -2;

    do {
        nalsz = getnal(nalbuf, ofs, sz);
        const unsigned char * nal = nalbuf + ofs;

        decode_nal(h, nal, nalsz);

        framecnt++;

        ofs += nalsz;
    } while (nalsz>0);


    return 0;
}


#ifdef NATIVE
#include <sys/mman.h>
unsigned char * mmap_open(const char * fname, size_t * sz)
{
    FILE * f = fopen(fname, "rb");
    fseek(f, 0, SEEK_END);
    *sz = ftell(f);
    return (unsigned char*)mmap(NULL, *sz, PROT_READ,MAP_SHARED, fileno(f), 0);
}


int main(int argc, char * argv[])
{
    void * h = open_decoder();


    emscripten_log(EM_LOG_CONSOLE, "ready to decode\n");
    size_t sz;
    const unsigned char * nalbuf = mmap_open(argv[1], &sz);
    size_t ofs = 0;
    ssize_t nalsz;

    do {
        nalsz = getnal(nalbuf, ofs, sz);
        const unsigned char * nal = nalbuf + ofs;

        decode_nal(h, nal, nalsz);

        ofs += nalsz;
    } while (nalsz>0);


    close_decoder(h);

}
#endif
