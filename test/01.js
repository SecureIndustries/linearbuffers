
var LinearBuffers = require('../src/encoder.js');
var Encoder       = require('../test/01-encoder.js');
var Decoder       = require('../test/01-decoder.js');

var rc;
var encoder;
var linearized;
var output;

encoder = new LinearBuffers.LinearBuffersEncoder();
if (encoder == null) {
        throw "can not create encoder";
}

rc  = Encoder.linearbuffers_output_start(encoder);
rc |= Encoder.linearbuffers_output_int8_set(encoder, 0);
rc |= Encoder.linearbuffers_output_int16_set(encoder, 1);
rc |= Encoder.linearbuffers_output_int32_set(encoder, 2);
rc |= Encoder.linearbuffers_output_int64_set(encoder, 3);
rc |= Encoder.linearbuffers_output_uint8_set(encoder, 4);
rc |= Encoder.linearbuffers_output_uint16_set(encoder, 5);
rc |= Encoder.linearbuffers_output_uint32_set(encoder, 6);
rc |= Encoder.linearbuffers_output_uint64_set(encoder, 7);
rc |= Encoder.linearbuffers_output_float_set(encoder, 0.8);
rc |= Encoder.linearbuffers_output_double_set(encoder, 0.9);
rc |= Encoder.linearbuffers_output_string_create(encoder, "1234567890");
rc |= Encoder.linearbuffers_output_anum_set(encoder, Encoder.linearbuffers_a_enum_1);
rc |= Encoder.linearbuffers_output_finish(encoder);
if (rc != 0) {
        throw "can not encode output";
}

linearized = encoder.linearized();
if (linearized == null) {
        throw "can not get linearized buffer";
}

console.log(linearized);

output = Decoder.linearbuffers_output_decode(linearized);
if (output == null) {
        throw "decoder failed: linearbuffers_output_decode";
}
if (Decoder.linearbuffers_output_int8_get(output) != 0) {
        throw "decoder failed: linearbuffers_output_int8_get";
}
if (Decoder.linearbuffers_output_int16_get(output) != 1) {
        throw "decoder failed: linearbuffers_output_int16_get";
}
if (Decoder.linearbuffers_output_int32_get(output) != 2) {
        throw "decoder failed: linearbuffers_output_int32_get";
}
if (Decoder.linearbuffers_output_int64_get(output) != 3) {
        throw "decoder failed: linearbuffers_output_int64_get";
}
if (Decoder.linearbuffers_output_uint8_get(output) != 4) {
        throw "decoder failed: linearbuffers_output_uint8_get";
}
if (Decoder.linearbuffers_output_uint16_get(output) != 5) {
        throw "decoder failed: linearbuffers_output_uint16_get";
}
if (Decoder.linearbuffers_output_uint32_get(output) != 6) {
        throw "decoder failed: linearbuffers_output_uint32_get";
}
if (Decoder.linearbuffers_output_uint64_get(output) != 7) {
        throw "decoder failed: linearbuffers_output_uint64_get";
}
if (Decoder.linearbuffers_output_float_get(output).toFixed(1) != 0.8) {
        throw "decoder failed: linearbuffers_output_float_get";
}
if (Decoder.linearbuffers_output_double_get(output).toFixed(1) != 0.9) {
        throw "decoder failed: linearbuffers_output_double_get";
}
if (Decoder.linearbuffers_output_string_get_value(output) != "1234567890") {
        throw "decoder failed: linearbuffers_output_string_get_value";
}
if (Decoder.linearbuffers_output_anum_get(output) != Encoder.linearbuffers_a_enum_1) {
        throw "decoder failed: linearbuffers_output_anum_get";
}

delete encoder;
