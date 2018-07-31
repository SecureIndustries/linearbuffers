
#include <stdio.h>

#include "11-encoder.h"
#include "11-decoder.h"
#include "11-jsonify.h"

int main (int argc, char *argv[])
{
	int rc;

	uint64_t linearized_length;
	const uint8_t *linearized_buffer;

	struct linearbuffers_encoder *encoder;
	const struct linearbuffers_output_old *output_old;
	const struct linearbuffers_output_new *output_new;

	(void) argc;
	(void) argv;

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_old_start(encoder);
	rc |= linearbuffers_output_old_finish(encoder);
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

	linearbuffers_output_old_jsonify(linearized_buffer, linearized_length, LINEARBUFFERS_JSONIFY_FLAG_DEFAULT, (int (*) (void *context, const char *fmt, ...)) fprintf, stderr);

	output_old = linearbuffers_output_old_decode(linearized_buffer, linearized_length);
	if (output_old == NULL) {
		fprintf(stderr, "decoder failed: linearbuffers_output_decode\n");
		goto bail;
	}
	if (linearbuffers_output_old_8_get(output_old) != -8) {
		fprintf(stderr, "decoder failed: linearbuffers_output_old_8_get\n");
		goto bail;
	}
	if (linearbuffers_output_old_16_get(output_old) != -16) {
		fprintf(stderr, "decoder failed: linearbuffers_output_old_16_get\n");
		goto bail;
	}
	if (linearbuffers_output_old_32_get(output_old) != -32) {
		fprintf(stderr, "decoder failed: linearbuffers_output_old_32_get\n");
		goto bail;
	}
	if (linearbuffers_output_old_64_get(output_old) != -64) {
		fprintf(stderr, "decoder failed: linearbuffers_output_old_64_get\n");
		goto bail;
	}
        if (linearbuffers_output_old_anum_get(output_old) != linearbuffers_anum_a) {
                fprintf(stderr, "decoder failed: linearbuffers_output_old_anum_get\n");
                goto bail;
        }

	linearbuffers_output_new_jsonify(linearized_buffer, linearized_length, LINEARBUFFERS_JSONIFY_FLAG_DEFAULT, (int (*) (void *context, const char *fmt, ...)) fprintf, stderr);

	output_new = linearbuffers_output_new_decode(linearized_buffer, linearized_length);
	if (output_new == NULL) {
		fprintf(stderr, "decoder failed: linearbuffers_output_decode\n");
		goto bail;
	}
	if (linearbuffers_output_new_int8_get(output_new) != -8) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_int8_get\n");
		goto bail;
	}
	if (linearbuffers_output_new_int16_get(output_new) != -16) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_int16_get\n");
		goto bail;
	}
	if (linearbuffers_output_new_int32_get(output_new) != -32) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_int32_get\n");
		goto bail;
	}
	if (linearbuffers_output_new_int64_get(output_new) != -64) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_int64_get\n");
		goto bail;
	}
        if (linearbuffers_output_new_anum_get(output_new) != linearbuffers_anum_a) {
                fprintf(stderr, "decoder failed: linearbuffers_output_new_anum_get\n");
                goto bail;
        }
	if (linearbuffers_output_new_uint8_get(output_new) != 8) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_uint8_get (%d != 8)\n", linearbuffers_output_new_uint8_get(output_new));
		goto bail;
	}
	if (linearbuffers_output_new_uint16_get(output_new) != 16) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_uint16_get\n");
		goto bail;
	}
	if (linearbuffers_output_new_uint32_get(output_new) != 32) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_uint32_get\n");
		goto bail;
	}
	if (linearbuffers_output_new_uint64_get(output_new) != 64) {
		fprintf(stderr, "decoder failed: linearbuffers_output_new_uint64_get\n");
		goto bail;
	}
        if (strcmp(linearbuffers_output_new_string_get_value(output_new), "string string") != 0) {
                fprintf(stderr, "decoder failed: linearbuffers_output_new_string_get_value\n");
                goto bail;
        }

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
