
#include <stdio.h>

#include "05-encoder.h"
#include "05-decoder.h"

int main (int argc, char *argv[])
{
	uint8_t data[10];
	const char *linearized;
	uint64_t linearized_length;
	struct linearbuffers_encoder *encoder;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	linearbuffers_output_start(encoder);
	linearbuffers_output_set_type(encoder, linearbuffers_type_type_3);
	linearbuffers_output_set_length(encoder, 1);
	linearbuffers_output_timeval_start(encoder);
	linearbuffers_output_timeval_set_seconds(encoder, 2);
	linearbuffers_output_timeval_set_useconds(encoder, 3);
	linearbuffers_output_timeval_end(encoder);
	linearbuffers_output_set_data(encoder, data, sizeof(data) / sizeof(data[0]));
	linearbuffers_output_end(encoder);

	linearized = linearbuffers_encoder_linearized(encoder, &linearized_length);
	if (linearized == NULL) {
		fprintf(stderr, "can not get linearized buffer\n");
		goto bail;
	}
	fprintf(stderr, "linearized: %p, length: %ld\n", linearized, linearized_length);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
