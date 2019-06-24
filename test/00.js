
var LinearBuffers = require('../src/encoder.js');

var encoder;

encoder = new LinearBuffers.LinearBuffersEncoder();
if (encoder == null) {
        throw "can not create encoder";
}
delete encoder;
