
#include <stdio.h>

#include "06-encoder.h"

int main (int argc, char *argv[])
{
	uint8_t data[10];
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
	linearbuffers_output_timevals_start(encoder);
	linearbuffers_output_timevals_timeval_start(encoder);
	linearbuffers_output_timevals_timeval_set_seconds(encoder, 2);
	linearbuffers_output_timevals_timeval_set_useconds(encoder, 3);
	linearbuffers_output_timevals_timeval_end(encoder);
	linearbuffers_output_timevals_timeval_start(encoder);
	linearbuffers_output_timevals_timeval_set_seconds(encoder, 4);
	linearbuffers_output_timevals_timeval_set_useconds(encoder, 5);
	linearbuffers_output_timevals_timeval_end(encoder);
	linearbuffers_output_timevals_end(encoder);
	linearbuffers_output_set_data(encoder, data, sizeof(data) / sizeof(data[0]));
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
