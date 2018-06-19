
#include <stdio.h>

#include "01-encoder.h"
#include "01-decoder.h"
#include "01-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;

	uint64_t linearized_length;
	const uint8_t *linearized_buffer;

	struct linearbuffers_encoder *encoder;
	struct linearbuffers_decoder decoder;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_output_int8_set(encoder, 0);
	rc |= linearbuffers_output_int16_set(encoder, 1);
	rc |= linearbuffers_output_int32_set(encoder, 2);
	rc |= linearbuffers_output_int64_set(encoder, 3);
	rc |= linearbuffers_output_uint8_set(encoder, 4);
	rc |= linearbuffers_output_uint16_set(encoder, 5);
	rc |= linearbuffers_output_uint32_set(encoder, 6);
	rc |= linearbuffers_output_uint64_set(encoder, 7);
	rc |= linearbuffers_output_string_set(encoder, "1234567890");
	rc |= linearbuffers_output_anum_set(encoder, linearbuffers_a_enum_1);
	rc |= linearbuffers_output_end(encoder);
	if (rc != 0) {
		fprintf(stderr, "can not encode output\n");
		goto bail;
	}

	linearized_buffer = linearbuffers_encoder_linearized(encoder, &linearized_length);
	if (linearized_buffer == NULL) {
		fprintf(stderr, "can not get linearized buffer\n");
		goto bail;
	}
	fprintf(stderr, "linearized: %p, length: %ld\n", linearized_buffer, linearized_length);

	linearbuffers_output_jsonify(linearized_buffer, linearized_length, printf);

	linearbuffers_output_decode(&decoder, linearized_buffer, linearized_length);
	if (linearbuffers_output_int8_get(&decoder) != 0) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_int16_get(&decoder) != 1) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_int32_get(&decoder) != 2) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_int64_get(&decoder) != 3) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_uint8_get(&decoder) != 4) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_uint16_get(&decoder) != 5) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_uint32_get(&decoder) != 6) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_uint64_get(&decoder) != 7) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (strcmp(linearbuffers_output_string_get(&decoder), "string") != 0) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
