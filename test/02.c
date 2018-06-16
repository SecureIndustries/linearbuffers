
#include <stdio.h>

#include "02-encoder.h"
#include "02-decoder.h"
#include "02-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;

	uint64_t linearized_length;
	const char *linearized_buffer;

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
	rc |= linearbuffers_output_1_start(encoder);
	rc |= linearbuffers_output_1_foo_set(encoder, 1);
	rc |= linearbuffers_output_1_end(encoder);
	rc |= linearbuffers_output_2_start(encoder);
	rc |= linearbuffers_output_2_bar_set(encoder, 2);
	rc |= linearbuffers_output_2_end(encoder);
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
	if (linearbuffers_output_1_foo_get(&decoder) != 1) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_2_bar_get(&decoder) != 2) {
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
