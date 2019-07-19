
var LinearBuffers = require('../src/encoder.js');
var Encoder       = require('../test/00-encoder.js');
var Decoder       = require('../test/00-decoder.js');

var encoder;

encoder = new LinearBuffers.LinearBuffersEncoder();
if (encoder == null) {
        throw "can not create encoder";
}
delete encoder;
