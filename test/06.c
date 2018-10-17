
#include <stdio.h>
#include <string.h>

struct emitter_param {
	uint64_t length;
};

static int emitter_function (void *context, uint64_t offset, const void *buffer, int64_t length)
{
	struct emitter_param *emitter_param = context;
	if (length < 0) {
		emitter_param->length = offset + length;
	} else {
		emitter_param->length = (emitter_param->length > offset + length) ? emitter_param->length : offset + length;
	}
	fprintf(stderr, "offset: 0x%08" PRIx64 ", length: %08" PRIu64 ", buffer: %p\n", offset, length, buffer);
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
	rc |= linearbuffers_output_finish(encoder);
	if (rc != 0) {
		fprintf(stderr, "can not encode output\n");
		goto bail;
	}

	fprintf(stderr, "linearized length: %" PRIu64 "\n", emitter_param.length);

	linearbuffers_encoder_destroy(encoder);

	return 0;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return -1;
}
