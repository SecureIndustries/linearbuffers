
#include <stdio.h>

#include "04-encoder.h"
#include "04-decoder.h"
#include "04-jsonify.h"

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
	rc |= linearbuffers_output_length_set(encoder, 1);
	rc |= linearbuffers_output_timeval_start(encoder);
	rc |= linearbuffers_output_timeval_seconds_set(encoder, 2);
	rc |= linearbuffers_output_timeval_useconds_set(encoder, 3);
	rc |= linearbuffers_output_timeval_set(encoder, linearbuffers_output_timeval_end(encoder));
	rc |= linearbuffers_output_data_set_values(encoder, NULL, 0);
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
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_length_get(output) != 1) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_timeval_seconds_get(linearbuffers_output_timeval_get(output)) != 2) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_output_timeval_useconds_get(linearbuffers_output_timeval_get(output)) != 3) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_uint8_vector_get_count(linearbuffers_output_data_get(output)) != 0) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	if (linearbuffers_uint8_vector_get_length(linearbuffers_output_data_get(output)) != 0) {
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
