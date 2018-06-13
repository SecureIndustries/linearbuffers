
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
	linearbuffers_output_a_set(encoder, 0);
	linearbuffers_output_b_set(encoder, 1);
	linearbuffers_output_c_set(encoder, 2);
	linearbuffers_output_d_set(encoder, 3);
	linearbuffers_output_1_set(encoder, 4);
	linearbuffers_output_2_set(encoder, 5);
	linearbuffers_output_3_set(encoder, 6);
	linearbuffers_output_4_set(encoder, 7);
	linearbuffers_output_end(encoder);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
