
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "08-encoder.h"
#include "08-decoder.h"
#include "08-jsonify.h"

#define ARRAY_COUNT	4

int main (int argc, char *argv[])
{
	int rc;
	struct linearbuffers_encoder *encoder;
	struct linearbuffers_decoder decoder;

	size_t i;

	int8_t int8s[ARRAY_COUNT];
	int16_t int16s[ARRAY_COUNT];
	int32_t int32s[ARRAY_COUNT];
	int64_t int64s[ARRAY_COUNT];

	uint8_t uint8s[ARRAY_COUNT];
	uint16_t uint16s[ARRAY_COUNT];
	uint32_t uint32s[ARRAY_COUNT];
	uint64_t uint64s[ARRAY_COUNT];

	linearbuffers_a_enum_enum_t enums[ARRAY_COUNT];

	char *strings[ARRAY_COUNT];

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
		int64s[i] = ((int64_t) rand() << 32) | rand();
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
		uint64s[i] = ((uint64_t) rand() << 32) | rand();
	}

	for (i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
		asprintf(&strings[i], "string-%ld", i);
	}

	for (i = 0; i < sizeof(enums) / sizeof(enums[0]); i++) {
		enums[i] = i;
	}

	encoder = linearbuffers_encoder_create(NULL);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);

	rc |= linearbuffers_output_int8s_start(encoder);
	for (i = 0; i < sizeof(int8s) / sizeof(int8s[0]); i++) {
		rc |= linearbuffers_output_int8s_push(encoder, int8s[i]);
	}
	rc |= linearbuffers_output_int8s_end(encoder);

	rc |= linearbuffers_output_int16s_start(encoder);
	for (i = 0; i < sizeof(int16s) / sizeof(int16s[0]); i++) {
		rc |= linearbuffers_output_int16s_push(encoder, int16s[i]);
	}
	rc |= linearbuffers_output_int16s_end(encoder);

	rc |= linearbuffers_output_int32s_start(encoder);
	for (i = 0; i < sizeof(int32s) / sizeof(int32s[0]); i++) {
		rc |= linearbuffers_output_int32s_push(encoder, int32s[i]);
	}
	rc |= linearbuffers_output_int32s_end(encoder);

	rc |= linearbuffers_output_int64s_start(encoder);
	for (i = 0; i < sizeof(int64s) / sizeof(int64s[0]); i++) {
		rc |= linearbuffers_output_int64s_push(encoder, int64s[i]);
	}
	rc |= linearbuffers_output_int64s_end(encoder);

	rc |= linearbuffers_output_uint8s_start(encoder);
	for (i = 0; i < sizeof(uint8s) / sizeof(uint8s[0]); i++) {
		rc |= linearbuffers_output_uint8s_push(encoder, uint8s[i]);
	}
	rc |= linearbuffers_output_uint8s_end(encoder);

	rc |= linearbuffers_output_uint16s_start(encoder);
	for (i = 0; i < sizeof(uint16s) / sizeof(uint16s[0]); i++) {
		rc |= linearbuffers_output_uint16s_push(encoder, uint16s[i]);
	}
	rc |= linearbuffers_output_uint16s_end(encoder);


	rc |= linearbuffers_output_uint32s_start(encoder);
	for (i = 0; i < sizeof(uint32s) / sizeof(uint32s[0]); i++) {
		rc |= linearbuffers_output_uint32s_push(encoder, uint32s[i]);
	}
	rc |= linearbuffers_output_uint32s_end(encoder);

	rc |= linearbuffers_output_uint64s_start(encoder);
	for (i = 0; i < sizeof(uint64s) / sizeof(uint64s[0]); i++) {
		rc |= linearbuffers_output_uint64s_push(encoder, uint64s[i]);
	}
	rc |= linearbuffers_output_uint64s_end(encoder);

	rc |= linearbuffers_output_strings_start(encoder);
	for (i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
		rc |= linearbuffers_output_strings_push(encoder, strings[i]);
	}
	rc |= linearbuffers_output_strings_end(encoder);

	rc |= linearbuffers_output_enums_start(encoder);
	for (i = 0; i < sizeof(enums) / sizeof(enums[0]); i++) {
		rc |= linearbuffers_output_enums_push(encoder, enums[i]);
	}
	rc |= linearbuffers_output_enums_end(encoder);

	rc |= linearbuffers_output_tables_start(encoder);
	for (i = 0; i < ARRAY_COUNT; i++) {
		rc |= linearbuffers_output_tables_a_table_start(encoder);
		rc |= linearbuffers_output_tables_a_table_int8_set(encoder, int8s[i]);
		rc |= linearbuffers_output_tables_a_table_int16_set(encoder, int16s[i]);
		rc |= linearbuffers_output_tables_a_table_int32_set(encoder, int32s[i]);
		rc |= linearbuffers_output_tables_a_table_int64_set(encoder, int64s[i]);
		rc |= linearbuffers_output_tables_a_table_uint8_set(encoder, uint8s[i]);
		rc |= linearbuffers_output_tables_a_table_uint16_set(encoder, uint16s[i]);
		rc |= linearbuffers_output_tables_a_table_uint32_set(encoder, uint32s[i]);
		rc |= linearbuffers_output_tables_a_table_uint64_set(encoder, uint64s[i]);
		rc |= linearbuffers_output_tables_a_table_string_set(encoder, strings[i]);
		rc |= linearbuffers_output_tables_a_table_anum_set(encoder, enums[i]);
		rc |= linearbuffers_output_tables_a_table_end(encoder);
	}
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
	rc = linearbuffers_output_decode(&decoder, linearized_buffer, linearized_length);
	if (rc != 0) {
		fprintf(stderr, "decoder failed: linearbuffers_output_decode\n");
		goto bail;
	}

	if (linearbuffers_output_int8s_get_count(&decoder) != sizeof(int8s) / sizeof(int8s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int8s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_int8s_get_length(&decoder) != sizeof(int8s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int8s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_int8s_get(&decoder), int8s, sizeof(int8s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int8s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_int8s_get_count(&decoder); i++) {
		if (int8s[i] != linearbuffers_output_int8s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_int8s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_int16s_get_count(&decoder) != sizeof(int16s) / sizeof(int16s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int16s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_int16s_get_length(&decoder) != sizeof(int16s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int16s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_int16s_get(&decoder), int16s, sizeof(int16s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int16s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_int16s_get_count(&decoder); i++) {
		if (int16s[i] != linearbuffers_output_int16s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_int16s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_int32s_get_count(&decoder) != sizeof(int32s) / sizeof(int32s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int32s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_int32s_get_length(&decoder) != sizeof(int32s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int32s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_int32s_get(&decoder), int32s, sizeof(int32s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int32s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_int32s_get_count(&decoder); i++) {
		if (int32s[i] != linearbuffers_output_int32s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_int32s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_int64s_get_count(&decoder) != sizeof(int64s) / sizeof(int64s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int64s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_int64s_get_length(&decoder) != sizeof(int64s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int64s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_int64s_get(&decoder), int64s, sizeof(int64s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_int64s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_int64s_get_count(&decoder); i++) {
		if (int64s[i] != linearbuffers_output_int64s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_int64s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_uint8s_get_count(&decoder) != sizeof(uint8s) / sizeof(uint8s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint8s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_uint8s_get_length(&decoder) != sizeof(uint8s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint8s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_uint8s_get(&decoder), uint8s, sizeof(uint8s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint8s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_uint8s_get_count(&decoder); i++) {
		if (uint8s[i] != linearbuffers_output_uint8s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_uint8s_get_at(%ld) (%u != %u)\n", i, uint8s[i], linearbuffers_output_uint8s_get_at(&decoder, i));
			goto bail;
		}
	}

	if (linearbuffers_output_uint16s_get_count(&decoder) != sizeof(uint16s) / sizeof(uint16s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint16s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_uint16s_get_length(&decoder) != sizeof(uint16s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint16s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_uint16s_get(&decoder), uint16s, sizeof(uint16s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint16s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_uint16s_get_count(&decoder); i++) {
		if (uint16s[i] != linearbuffers_output_uint16s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_uint16s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_uint32s_get_count(&decoder) != sizeof(uint32s) / sizeof(uint32s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint32s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_uint32s_get_length(&decoder) != sizeof(uint32s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint32s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_uint32s_get(&decoder), uint32s, sizeof(uint32s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint32s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_uint32s_get_count(&decoder); i++) {
		if (uint32s[i] != linearbuffers_output_uint32s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_uint32s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_uint64s_get_count(&decoder) != sizeof(uint64s) / sizeof(uint64s[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint64s_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_uint64s_get_length(&decoder) != sizeof(uint64s)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint64s_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_uint64s_get(&decoder), uint64s, sizeof(uint64s))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_uint64s_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_uint64s_get_count(&decoder); i++) {
		if (uint64s[i] != linearbuffers_output_uint64s_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_uint64s_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_strings_get_count(&decoder) != sizeof(strings) / sizeof(strings[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_strings_get_count\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_strings_get_count(&decoder); i++) {
		if (strcmp(strings[i], linearbuffers_output_strings_get_at(&decoder, i)) != 0) {
			fprintf(stderr, "decoder failed: linearbuffers_output_strings_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_enums_get_count(&decoder) != sizeof(enums) / sizeof(enums[0])) {
		fprintf(stderr, "decoder failed: linearbuffers_output_enums_get_count\n");
		goto bail;
	}
	if (linearbuffers_output_enums_get_length(&decoder) != sizeof(enums)) {
		fprintf(stderr, "decoder failed: linearbuffers_output_enums_get_length\n");
		goto bail;
	}
	if (memcmp(linearbuffers_output_enums_get(&decoder), enums, sizeof(enums))) {
		fprintf(stderr, "decoder failed: linearbuffers_output_enums_get\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_enums_get_count(&decoder); i++) {
		if (enums[i] != linearbuffers_output_enums_get_at(&decoder, i)) {
			fprintf(stderr, "decoder failed: linearbuffers_output_enums_get_at\n");
			goto bail;
		}
	}

	if (linearbuffers_output_tables_get_count(&decoder) != ARRAY_COUNT) {
		fprintf(stderr, "decoder failed\n");
		goto bail;
	}
	for (i = 0; i < linearbuffers_output_tables_get_count(&decoder); i++) {
		if (linearbuffers_output_tables_a_table_int8_get(&decoder, i) != int8s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_int16_get(&decoder, i) != int16s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_int32_get(&decoder, i) != int32s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_int64_get(&decoder, i) != int64s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_uint8_get(&decoder, i) != uint8s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_uint16_get(&decoder, i) != uint16s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_uint32_get(&decoder, i) != uint32s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_uint64_get(&decoder, i) != uint64s[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (strcmp(linearbuffers_output_tables_a_table_string_get(&decoder, i), strings[i]) != 0) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
		if (linearbuffers_output_tables_a_table_anum_get(&decoder, i) != enums[i]) {
			fprintf(stderr, "decoder failed\n");
			goto bail;
		}
	}

	linearbuffers_encoder_destroy(encoder);

	for (i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
		free(strings[i]);
	}
	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
