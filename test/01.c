
#include <stdio.h>

#include "01-encoder.h"

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
	linearbuffers_output_set_a(encoder, 0);
	linearbuffers_output_set_b(encoder, 1);
	linearbuffers_output_set_c(encoder, 2);
	linearbuffers_output_set_d(encoder, 3);
	linearbuffers_output_set_1(encoder, 4);
	linearbuffers_output_set_2(encoder, 5);
	linearbuffers_output_set_3(encoder, 6);
	linearbuffers_output_set_4(encoder, 7);
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
