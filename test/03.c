
#include <stdio.h>

#include "03-encoder.h"
#include "03-decoder.h"
#include "03-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;

	uint64_t linearized_length;
	const char *linearized_buffer;

	struct linearbuffers_encoder *encoder;
	const struct linearbuffers_output *output;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_b_start(encoder);
	rc |= linearbuffers_b_s_set(encoder, 1);
	rc |= linearbuffers_a_start(encoder);
	rc |= linearbuffers_a_foo_set(encoder, 2);
	rc |= linearbuffers_b_t_set(encoder, linearbuffers_a_end(encoder));
	rc |= linearbuffers_output_1_set(encoder, linearbuffers_b_end(encoder));
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

	output = linearbuffers_output_decode(linearized_buffer, linearized_length);
	if (output == NULL) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_1_s_get(output) != 1) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_1_t_foo_get(output) != 2) {
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
