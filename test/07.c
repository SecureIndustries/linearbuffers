
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "07-encoder.h"
#include "07-decoder.h"
#include "07-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;
	struct linearbuffers_encoder *encoder;

	size_t i;

	int8_t int8s[2];
	int16_t int16s[2];
	int32_t int32s[2];
	int64_t int64s[2];

	uint8_t uint8s[2];
	uint16_t uint16s[2];
	uint32_t uint32s[2];
	uint64_t uint64s[2];

	const char *strings[2];

	uint64_t linearized_length;
	const char *linearized_buffer;

	(void) argc;
	(void) argv;

	srand(time(NULL));

	for (i = 0; i < sizeof(int8s) / sizeof(int8s[0]); i++) {
		int8s[i] = rand();
	}
	for (i = 0; i < sizeof(int16s) / sizeof(int16s[0]); i++) {
		int16s[i] = rand();
	}
	for (i = 0; i < sizeof(int32s) / sizeof(int32s[0]); i++) {
		int32s[i] = rand();
	}
	for (i = 0; i < sizeof(int64s) / sizeof(int64s[0]); i++) {
		int64s[i] = rand();
	}

	for (i = 0; i < sizeof(uint8s) / sizeof(uint8s[0]); i++) {
		uint8s[i] = rand();
	}
	for (i = 0; i < sizeof(uint16s) / sizeof(uint16s[0]); i++) {
		uint16s[i] = rand();
	}
	for (i = 0; i < sizeof(uint32s) / sizeof(uint32s[0]); i++) {
		uint32s[i] = rand();
	}
	for (i = 0; i < sizeof(uint64s) / sizeof(uint64s[0]); i++) {
		uint64s[i] = rand();
	}

	for (i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
		strings[i] = "string";
	}

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_output_int8s_set(encoder, int8s, sizeof(int8s) / sizeof(int8s[0]));
	rc |= linearbuffers_output_int16s_set(encoder, int16s, sizeof(int16s) / sizeof(int16s[0]));
	rc |= linearbuffers_output_int32s_set(encoder, int32s, sizeof(int32s) / sizeof(int32s[0]));
	rc |= linearbuffers_output_int64s_set(encoder, int64s, sizeof(int64s) / sizeof(int64s[0]));
	rc |= linearbuffers_output_uint8s_set(encoder, uint8s, sizeof(uint8s) / sizeof(uint8s[0]));
	rc |= linearbuffers_output_uint16s_set(encoder, uint16s, sizeof(uint16s) / sizeof(uint16s[0]));
	rc |= linearbuffers_output_uint32s_set(encoder, uint32s, sizeof(uint32s) / sizeof(uint32s[0]));
	rc |= linearbuffers_output_uint64s_set(encoder, uint64s, sizeof(uint64s) / sizeof(uint64s[0]));
	rc |= linearbuffers_output_strings_set(encoder, strings, sizeof(strings) / sizeof(strings[0]));
	rc |= linearbuffers_output_tables_start(encoder);
	rc |= linearbuffers_output_tables_end(encoder);
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

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
