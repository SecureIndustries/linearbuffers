
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "schema.h"

#define OPTION_HELP		'h'
#define OPTION_SCHEMA		's'
#define OPTION_PRETTY		'p'
#define OPTION_ENCODER		'e'
#define OPTION_DECODER		'd'

#define DEFAULT_SCHEMA		NULL
#define DEFAULT_PRETTY		NULL
#define DEFAULT_ENCODER		NULL
#define DEFAULT_DECODER		NULL

static struct option options[] = {
	{ "help"	, no_argument	   , 0, OPTION_HELP	},
	{ "schema"	, required_argument, 0, OPTION_SCHEMA	},
	{ "pretty"	, required_argument, 0, OPTION_PRETTY	},
	{ "encoder"	, required_argument, 0, OPTION_ENCODER	},
	{ "decoder"	, required_argument, 0, OPTION_DECODER	},
	{ 0		, 0                , 0, 0		}
};

static void print_help (const char *name)
{
	fprintf(stdout, "%s:\n", name);
	fprintf(stdout, "\n");
	fprintf(stdout, "options:\n");
	fprintf(stdout, "  -s, --schema : schema file (default: %s)\n", (DEFAULT_SCHEMA == NULL) ? "(null)" : DEFAULT_SCHEMA);
	fprintf(stdout, "  -p, --pretty : generated pretty file (default: %s)\n", (DEFAULT_PRETTY == NULL) ? "(null)" : DEFAULT_PRETTY);
	fprintf(stdout, "  -e, --encoder: generated encoder file (default: %s)\n", (DEFAULT_ENCODER == NULL) ? "(null)" : DEFAULT_ENCODER);
	fprintf(stdout, "  -d, --decoder: generated decoder file (default: %s)\n", (DEFAULT_DECODER == NULL) ? "(null)" : DEFAULT_DECODER);
	fprintf(stdout, "  -h, --help   : this text\n");
}

int main (int argc, char *argv[])
{
	int c;
	int option_index;

	const char *option_schema;
	const char *option_pretty;
	const char *option_encoder;
	const char *option_decoder;

	int rc;
	struct schema *schema;

	option_schema  = DEFAULT_SCHEMA;
	option_pretty  = DEFAULT_PRETTY;
	option_encoder = DEFAULT_ENCODER;
	option_decoder = DEFAULT_DECODER;

	while (1) {
		c = getopt_long(argc, argv, "s:p:e:d:h", options, &option_index);
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
			case OPTION_PRETTY:
				option_pretty = optarg;
				break;
			case OPTION_ENCODER:
				option_encoder = optarg;
				break;
			case OPTION_DECODER:
				option_decoder = optarg;
				break;
		}
	}

	if (option_schema == NULL) {
		fprintf(stderr, "schema file is invalid\n");
		goto bail;
	}
	if (option_pretty == NULL &&
	    option_encoder == NULL &&
	    option_decoder == NULL) {
		fprintf(stderr, "nothing to generate\n");
		goto bail;
	}

	schema = schema_parse_file(option_schema);
	if (schema == NULL) {
		fprintf(stderr, "can not read schema file: %s\n", option_schema);
		goto bail;
	}

	if (option_pretty != NULL) {
		rc = schema_generate_pretty(schema, option_pretty);
		if (rc != 0) {
			fprintf(stderr, "can not dump schema file: %s\n", option_schema);
			goto bail;
		}
	}
	if (option_encoder != NULL) {
		rc = schema_generate_encoder(schema, option_encoder);
		if (rc != 0) {
			fprintf(stderr, "can not dump schema file: %s\n", option_schema);
			goto bail;
		}
	}
	if (option_decoder != NULL) {
		rc = schema_generate_decoder(schema, option_decoder);
		if (rc != 0) {
			fprintf(stderr, "can not dump schema file: %s\n", option_schema);
			goto bail;
		}
	}

	schema_destroy(schema);

out:	return 0;
bail:	return -1;
}
