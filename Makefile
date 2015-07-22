TOTALMEMORY:=-s TOTAL_MEMORY=33554432

all: decoder.js.gz WebGLCanvas.js

h264symbols='["_open_decoder","_close_decoder","_decode_h264buffer","_decode_nal"]'

decoder.js.gz: decoder.js
	gzip -f -k decoder.js

decoder.js: decoder_em.js othercode.js
	cat decoder_em.js othercode.js > decoder.js

decoder_em.js: decoder.cpp openh264/libopenh264.so.0
	em++ -O1 $(TOTALMEMORY) -o $@ $^ -s EXPORTED_FUNCTIONS=$(h264symbols) -Iopenh264/codec/api/svc/

openh264:
	git clone git@github.com:cisco/openh264.git openh264

openh264/libopenh264.so.0: openh264
	cd openh264 && git checkout 41caf381520005c612a2089e070b35c045cc3e77 && emmake make OS=linux ARCH=asmjs

WebGLCanvas.js:
	wget "https://raw.githubusercontent.com/mbebenita/Broadway/master/Player/WebGLCanvas.js"

.PHONY: clean cleanall distclean

clean:
	rm -f decoder.js decoder.js.gz decoder_em.js

cleanall: clean
	cd openh264; make clean

distclean: clean
	rm -Rf openh264 WebGLCanvas.js
