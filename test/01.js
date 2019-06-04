
var LinearBuffers = require('../src/encoder.js');
var Output        = require('../test/01-encoder.js');

var encoder;

encoder = new LinearBuffers.LinearBuffersEncoder();
if (encoder == null) {
	throw "can not create encoder";
}

Output.linearbuffers_output_start(encoder);
Output.linearbuffers_output_finish(encoder);

delete encoder;
