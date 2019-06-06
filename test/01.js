
var LinearBuffers = require('../src/encoder.js');
var Output        = require('../test/01-encoder.js');

var rc;
var encoder;

encoder = new LinearBuffers.LinearBuffersEncoder();
if (encoder == null) {
	throw "can not create encoder";
}

rc  = Output.linearbuffers_output_start(encoder);
rc |= Output.linearbuffers_output_int8_set(encoder, 0);
rc |= Output.linearbuffers_output_int16_set(encoder, 1);
rc |= Output.linearbuffers_output_int32_set(encoder, 2);
rc |= Output.linearbuffers_output_int64_set(encoder, 3);
rc |= Output.linearbuffers_output_uint8_set(encoder, 4);
rc |= Output.linearbuffers_output_uint16_set(encoder, 5);
rc |= Output.linearbuffers_output_uint32_set(encoder, 6);
rc |= Output.linearbuffers_output_uint64_set(encoder, 7);
rc |= Output.linearbuffers_output_float_set(encoder, 0.8);
rc |= Output.linearbuffers_output_double_set(encoder, 0.9);
rc |= Output.linearbuffers_output_string_create(encoder, "1234567890");
rc |= Output.linearbuffers_output_anum_set(encoder, Output.linearbuffers_a_enum_1);
rc |= Output.linearbuffers_output_finish(encoder);
if (rc != 0) {
	throw "can not encode output";
}

delete encoder;
