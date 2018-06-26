
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include "schema.h"

#define OPTION_HELP			'h'
#define OPTION_SCHEMA			's'
#define OPTION_OUTPUT			'o'
#define OPTION_PRETTY			'p'
#define OPTION_ENCODER			'e'
#define OPTION_DECODER			'd'
#define OPTION_DECODER_USE_MEMCPY	'm'
#define OPTION_JSONIFY			'j'

#define DEFAULT_SCHEMA			NULL
#define DEFAULT_OUTPUT			NULL
#define DEFAULT_PRETTY			0
#define DEFAULT_ENCODER			0
#define DEFAULT_DECODER			0
#define DEFAULT_DECODER_USE_MEMCPY	0
#define DEFAULT_JSONIFY			0

int schema_generate_pretty (struct schema *schema, FILE *fp);
int schema_generate_c_encoder (struct schema *schema, FILE *fp, int decoder_use_memcpy);
int schema_generate_c_decoder (struct schema *schema, FILE *fp, int decoder_use_memcpy);
int schema_generate_c_jsonify (struct schema *schema, FILE *fp, int decoder_use_memcpy);

static struct option options[] = {
	{ "help"		, no_argument	   , 0, OPTION_HELP			},
	{ "schema"		, required_argument, 0, OPTION_SCHEMA			},
	{ "output"		, required_argument, 0, OPTION_OUTPUT			},
	{ "pretty"		, required_argument, 0, OPTION_PRETTY			},
	{ "encoder"		, required_argument, 0, OPTION_ENCODER			},
	{ "decoder"		, required_argument, 0, OPTION_DECODER			},
	{ "decoder-use-memcpy"	, required_argument, 0, OPTION_DECODER_USE_MEMCPY	},
	{ "jsonify"		, required_argument, 0, OPTION_JSONIFY			},
	{ 0			, 0                , 0, 0				}
};

static void print_help (const char *name)
{
	fprintf(stdout, "%s:\n", name);
	fprintf(stdout, "\n");
	fprintf(stdout, "options:\n");
	fprintf(stdout, "  -s, --schema : schema file (default: %s)\n", (DEFAULT_SCHEMA == NULL) ? "(null)" : DEFAULT_SCHEMA);
	fprintf(stdout, "  -o, --output : output file (default: %s)\n", (DEFAULT_OUTPUT == NULL) ? "(null)" : DEFAULT_OUTPUT);
	fprintf(stdout, "  -p, --pretty : generate pretty (default: %d)\n", DEFAULT_PRETTY);
	fprintf(stdout, "  -e, --encoder: generate encoder (default: %d)\n", DEFAULT_ENCODER);
	fprintf(stdout, "  -d, --decoder: generate decoder (default: %d)\n", DEFAULT_DECODER);
	fprintf(stdout, "  -m, --decoder-use-memcpy: decoding using memcpy, rather than casting (default: %d)\n", DEFAULT_DECODER_USE_MEMCPY);
	fprintf(stdout, "  -j, --jsonify: generate jsonify (default: %d)\n", DEFAULT_JSONIFY);
	fprintf(stdout, "  -h, --help   : this text\n");
}

int main (int argc, char *argv[])
{
	int c;
	int option_index;

	FILE *output_file;

	const char *option_schema;
	const char *option_output;
	int option_pretty;
	int option_encoder;
	int option_decoder;
	int option_decoder_use_memcpy;
	int option_jsonify;

	int rc;
	struct schema *schema;

	schema = NULL;
	output_file = NULL;

	option_schema  = DEFAULT_SCHEMA;
	option_output  = DEFAULT_OUTPUT;
	option_pretty  = DEFAULT_PRETTY;
	option_encoder = DEFAULT_ENCODER;
	option_decoder = DEFAULT_DECODER;
	option_decoder_use_memcpy = DEFAULT_DECODER_USE_MEMCPY;
	option_jsonify = DEFAULT_JSONIFY;

	while (1) {
		c = getopt_long(argc, argv, "s:o:p:e:d:m:j:h", options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case OPTION_HELP:
				print_help(argv[0]);
				goto out;
			case OPTION_SCHEMA:
				option_schema = optarg;
				break;
			case OPTION_OUTPUT:
				option_output = optarg;
				break;
			case OPTION_PRETTY:
				option_pretty = !!atoi(optarg);
				break;
			case OPTION_ENCODER:
				option_encoder = !!atoi(optarg);
				break;
			case OPTION_DECODER:
				option_decoder = !!atoi(optarg);
				break;
			case OPTION_DECODER_USE_MEMCPY:
				option_decoder_use_memcpy = !!atoi(optarg);
				break;
			case OPTION_JSONIFY:
				option_jsonify = !!atoi(optarg);
				break;
		}
	}

	if (option_schema == NULL) {
		fprintf(stderr, "schema file is invalid\n");
		goto bail;
	}
	if (option_output == NULL) {
		fprintf(stderr, "output file is invalid\n");
		goto bail;
	}
	if (option_pretty == 0 &&
	    option_encoder == 0 &&
	    option_decoder == 0 &&
	    option_jsonify == 0) {
		fprintf(stderr, "nothing to generate\n");
		goto bail;
	}
	if (option_pretty && (option_encoder || option_decoder || option_jsonify)) {
		fprintf(stderr, "pretty and (encoder | decoder | jsonify) are different things\n");
		goto bail;
	}

	schema = schema_parse_file(option_schema);
	if (schema == NULL) {
		fprintf(stderr, "can not read schema file: %s\n", option_schema);
		goto bail;
	}

	if (strcmp(option_output, "stdout") == 0) {
		output_file = stdout;
	} else if (strcmp(option_output, "stderr") == 0) {
		output_file = stderr;
	} else {
		unlink(option_output);
		output_file = fopen(option_output, "w");
	}
	if (output_file == NULL) {
		fprintf(stderr, "can not create file: %s\n", option_output);
		goto bail;
	}

	if (option_pretty) {
		rc = schema_generate_pretty(schema, output_file);
		if (rc != 0) {
			fprintf(stderr, "can not generate schema file: %s\n", option_output);
			goto bail;
		}
	}
	if (option_encoder) {
		rc = schema_generate_c_encoder(schema, output_file, option_decoder_use_memcpy);
		if (rc != 0) {
			fprintf(stderr, "can not generate encoder file: %s\n", option_output);
			goto bail;
		}
	}
	if (option_decoder) {
		rc = schema_generate_c_decoder(schema, output_file, option_decoder_use_memcpy);
		if (rc != 0) {
			fprintf(stderr, "can not generate decoder file: %s\n", option_output);
			goto bail;
		}
	}
	if (option_jsonify) {
		rc = schema_generate_c_jsonify(schema, output_file, option_decoder_use_memcpy);
		if (rc != 0) {
			fprintf(stderr, "can not generate jsonify file: %s\n", option_output);
			goto bail;
		}
	}

	if (output_file != NULL &&
	    output_file != stdout &&
	    output_file != stderr) {
		fclose(output_file);
	}
	schema_destroy(schema);

out:	return 0;
bail:	if (output_file != NULL &&
	    output_file != stdout &&
	    output_file != stderr) {
		fclose(output_file);
	}
	if (option_output != NULL &&
	    output_file != stdout &&
	    output_file != stderr) {
		unlink(option_output);
	}
	if (schema != NULL) {
		schema_destroy(schema);
	}
	return -1;
}
