
#include <stdio.h>
#include <string.h>

#include "06-encoder.h"
#include "06-decoder.h"
#include "06-jsonify.h"

struct emitter_param {
	uint64_t length;
};

static int emitter_function (void *context, uint64_t offset, const void *buffer, uint64_t length)
{
	struct emitter_param *emitter_param = context;
	emitter_param->length = (emitter_param->length > offset + length) ? emitter_param->length : offset + length;
	fprintf(stderr, "offset: 0x%08lx, length: 0x%08lx, buffer: %p\n", offset, length, buffer);
	return 0;
}

int main (int argc, char *argv[])
{
	int rc;

	struct emitter_param emitter_param;
	struct linearbuffers_encoder *encoder;
	struct linearbuffers_encoder_create_options encoder_create_options;

	(void) argc;
	(void) argv;

	memset(&emitter_param, 0, sizeof(struct emitter_param));

	memset(&encoder_create_options, 0, sizeof(struct linearbuffers_encoder_create_options));
	encoder_create_options.emitter.context = &emitter_param;
	encoder_create_options.emitter.function = emitter_function;

	encoder = linearbuffers_encoder_create(&encoder_create_options);
	if (encoder == NULL) {
		fprintf(stderr, "can not create linearbuffers encoder\n");
		goto bail;
	}

	rc  = linearbuffers_output_start(encoder);
	rc |= linearbuffers_output_0_set(encoder, 1);
	rc |= linearbuffers_output_end(encoder);
	if (rc != 0) {
		fprintf(stderr, "can not encode output\n");
		goto bail;
	}

	fprintf(stderr, "linearized length: %ld\n", emitter_param.length);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
