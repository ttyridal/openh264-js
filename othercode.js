function frame_callback(a,b,c,width,height,stride1,stride2)
{
    var data = new Uint8Array(width*height*3/2);
    var x,y;
    for (y=0;y<height;y++) {
        var ar = Module.HEAP8.subarray(a + y*stride1, a+y*stride1+width);
        data.set(ar, y * width);
    }
    var cbofs=width*height;
    for (y=0;y<height/2;y++) {
        var ar = Module.HEAP8.subarray(b + y*stride2, b+y*stride2+width/2);
        data.set(ar, cbofs + y * width/2);
    }

    var crofs=cbofs + width*height/4;
    for (y=0;y<height/2;y++) {
        var ar = Module.HEAP8.subarray(c + y*stride2, c+y*stride2+width/2);
        data.set(ar, crofs + y * width/2);
    }

    postMessage({'data':data});
}


(function(root){
        console.log("ready to load");
    
        var open_decoder = Module.cwrap('open_decoder', 'number', null);
        var close_decoder = Module.cwrap('close_decoder', null, ['number']);
        var decode_h264buffer = Module.cwrap('decode_h264buffer', 'number', ['number','array','number']);
        var decode_h264nal = Module.cwrap('decode_nal', 'number', ['number','array','number']);

        var h = open_decoder();

        root.addEventListener('message', function(e) {
            var message = e.data;
            switch(message.type) {
                case 'close':
                    close_decoder(h);
                    break;
                case 'frame':
                    if (message.data == null) {
                        decode_h264nal(h, 0, 0);
                    } else {
                        var byteArray = new Uint8Array(message.data);
                        decode_h264nal(h, byteArray, byteArray.length);
                    }
            };
        });
}(self));
