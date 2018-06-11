
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
	linearbuffers_output_set_length(encoder, 1);
	linearbuffers_output_timeval_start(encoder);
	linearbuffers_output_timeval_set_seconds(encoder, 2);
	linearbuffers_output_timeval_set_useconds(encoder, 3);
	linearbuffers_output_timeval_end(encoder);
	linearbuffers_output_set_data(encoder, NULL, 0);
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
