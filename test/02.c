
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

	const struct linearbuffers_output *output;
	const struct linearbuffers_output_a *output_a;
	const struct linearbuffers_output_b *output_b;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_output_a_start(encoder);
	rc |= linearbuffers_output_a_foo_set(encoder, 1);
	rc |= linearbuffers_output_1_set(encoder, linearbuffers_output_a_end(encoder));
	rc |= linearbuffers_output_b_start(encoder);
	rc |= linearbuffers_output_b_bar_set(encoder, 2);
	rc |= linearbuffers_output_2_set(encoder, linearbuffers_output_b_end(encoder));
	rc |= linearbuffers_output_finish(encoder);
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

	linearbuffers_output_jsonify(linearized_buffer, linearized_length, (int (*) (void *context, const char *fmt, ...)) fprintf, stderr);

	output = linearbuffers_output_decode(linearized_buffer, linearized_length);
	if (output == NULL) {
		fprintf(stderr, "decoder failed: linearbuffers_output_decode\n");
		goto bail;
	}

	if (linearbuffers_output_a_foo_get(linearbuffers_output_1_get(output)) != 1) {
		fprintf(stderr, "decoder failed: linearbuffers_output_1_foo_get\n");
		goto bail;
	}
	if (linearbuffers_output_b_bar_get(linearbuffers_output_2_get(output)) != 2) {
		fprintf(stderr, "decoder failed: linearbuffers_output_2_bar_get\n");
		goto bail;
	}

	output_a = linearbuffers_output_1_get(output);
	if (linearbuffers_output_a_foo_get(output_a) != 1) {
		fprintf(stderr, "decoder failed: linearbuffers_output_a_foo_get\n");
		goto bail;
	}

	output_b = linearbuffers_output_2_get(output);
	if (linearbuffers_output_b_bar_get(output_b) != 2) {
		fprintf(stderr, "decoder failed: linearbuffers_output_b_bar_get\n");
		goto bail;
	}

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
