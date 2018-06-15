
#include <stdio.h>

#include "01-encoder.h"
#include "01-decoder.h"
#include "01-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;

	uint64_t linearized_length;
	const char *linearized_buffer;

	struct linearbuffers_encoder *encoder;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_output_a_set(encoder, 0);
	rc |= linearbuffers_output_b_set(encoder, 1);
	rc |= linearbuffers_output_c_set(encoder, 2);
	rc |= linearbuffers_output_d_set(encoder, 3);
	rc |= linearbuffers_output_1_set(encoder, 4);
	rc |= linearbuffers_output_2_set(encoder, 5);
	rc |= linearbuffers_output_3_set(encoder, 6);
	rc |= linearbuffers_output_4_set(encoder, 7);
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

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
