
#include <stdio.h>

#include "04-encoder.h"

int main (int argc, char *argv[])
{
	struct linearbuffers_encoder *encoder;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	linearbuffers_output_start(encoder);
	linearbuffers_output_length_set(encoder, 1);
	linearbuffers_output_timeval_start(encoder);
	linearbuffers_output_timeval_seconds_set(encoder, 2);
	linearbuffers_output_timeval_useconds_set(encoder, 3);
	linearbuffers_output_timeval_end(encoder);
	linearbuffers_output_data_set(encoder, NULL, 0);
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
