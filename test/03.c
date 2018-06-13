
#include <stdio.h>

#include "03-encoder.h"

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
	linearbuffers_output_1_start(encoder);
	linearbuffers_output_1_s_set(encoder, 1);
	linearbuffers_output_1_t_start(encoder);
	linearbuffers_output_1_t_foo_set(encoder, 2);
	linearbuffers_output_1_t_end(encoder);
	linearbuffers_output_1_end(encoder);
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
