
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include "schema.h"

#define OPTION_HELP                        'h'
#define OPTION_SCHEMA                        's'
#define OPTION_OUTPUT                        'o'
#define OPTION_PRETTY                        'p'
#define OPTION_ENCODER                  'e'
#define OPTION_ENCODER_INCLUDE_LIBRARY  'l'
#define OPTION_DECODER                        'd'
#define OPTION_DECODER_USE_MEMCPY        'm'
#define OPTION_JSONIFY                        'j'

#define DEFAULT_SCHEMA                        NULL
#define DEFAULT_OUTPUT                        NULL
#define DEFAULT_PRETTY                        0
#define DEFAULT_ENCODER                        0
#define DEFAULT_ENCODER_INCLUDE_LIBRARY 0
#define DEFAULT_DECODER                        0
#define DEFAULT_DECODER_USE_MEMCPY        0
#define DEFAULT_JSONIFY                        0

int schema_generate_pretty (struct schema *schema, FILE *fp);
int schema_generate_c_encoder (struct schema *schema, FILE *fp, int encoder_include_library);
int schema_generate_c_decoder (struct schema *schema, FILE *fp, int decoder_use_memcpy);
int schema_generate_c_jsonify (struct schema *schema, FILE *fp, int decoder_use_memcpy);

static struct option options[] = {
        { "help"                        , no_argument           , 0, OPTION_HELP                        },
        { "schema"                        , required_argument, 0, OPTION_SCHEMA                        },
        { "output"                        , required_argument, 0, OPTION_OUTPUT                        },
        { "pretty"                        , required_argument, 0, OPTION_PRETTY                        },
        { "encoder"                     , required_argument, 0, OPTION_ENCODER                  },
        { "encoder-include-library"     , required_argument, 0, OPTION_ENCODER_INCLUDE_LIBRARY  },
        { "decoder"                        , required_argument, 0, OPTION_DECODER                        },
        { "decoder-use-memcpy"                , required_argument, 0, OPTION_DECODER_USE_MEMCPY        },
        { "jsonify"                        , required_argument, 0, OPTION_JSONIFY                        },
        { 0                                , 0                , 0, 0                                }
};

static void print_help (const char *name)
{
        fprintf(stdout, "%s:\n", name);
        fprintf(stdout, "\n");
        fprintf(stdout, "options:\n");
        fprintf(stdout, "  -s, --schema : schema file (default: %s)\n", (DEFAULT_SCHEMA == NULL) ? "(null)" : DEFAULT_SCHEMA);
        fprintf(stdout, "  -o, --output : output file (default: %s)\n", (DEFAULT_OUTPUT == NULL) ? "(null)" : DEFAULT_OUTPUT);
        fprintf(stdout, "  -p, --pretty : generate pretty (values: { 0, 1 }, default: %d)\n", DEFAULT_PRETTY);
        fprintf(stdout, "  -e, --encoder: generate encoder (values: { 0, 1 }, default: %d)\n", DEFAULT_ENCODER);
        fprintf(stdout, "  -l, --encoder-include-library: generate encoder with builtin library(values: { 0, 1 }, default: %d)\n", DEFAULT_ENCODER_INCLUDE_LIBRARY);
        fprintf(stdout, "  -d, --decoder: generate decoder (values: { 0, 1 }, default: %d)\n", DEFAULT_DECODER);
        fprintf(stdout, "  -m, --decoder-use-memcpy: decode using memcpy, rather than casting (values: { 0, 1 }, default: %d)\n", DEFAULT_DECODER_USE_MEMCPY);
        fprintf(stdout, "  -j, --jsonify: generate jsonify (values: { 0, 1 }, default: %d)\n", DEFAULT_JSONIFY);
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
        int option_encoder_include_library;
        int option_decoder;
        int option_decoder_use_memcpy;
        int option_jsonify;

        int rc;
        struct schema *schema;

        schema = NULL;
        output_file = NULL;

        option_schema                   = DEFAULT_SCHEMA;
        option_output                   = DEFAULT_OUTPUT;
        option_pretty                   = DEFAULT_PRETTY;
        option_encoder                  = DEFAULT_ENCODER;
        option_encoder_include_library  = DEFAULT_ENCODER_INCLUDE_LIBRARY;
        option_decoder                  = DEFAULT_DECODER;
        option_decoder_use_memcpy       = DEFAULT_DECODER_USE_MEMCPY;
        option_jsonify                  = DEFAULT_JSONIFY;

        while (1) {
                c = getopt_long(argc, argv, "s:o:p:e:l:d:m:j:h", options, &option_index);
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
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_pretty = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_pretty = 0;
                                } else {
                                        option_pretty = !!atoi(optarg);
                                }
                                break;
                        case OPTION_ENCODER:
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_encoder = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_encoder = 0;
                                } else {
                                        option_encoder = !!atoi(optarg);
                                }
                                break;
                        case OPTION_ENCODER_INCLUDE_LIBRARY:
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_encoder_include_library = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_encoder_include_library = 0;
                                } else {
                                        option_encoder_include_library = !!atoi(optarg);
                                }
                                break;
                        case OPTION_DECODER:
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_decoder = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_decoder = 0;
                                } else {
                                        option_decoder = !!atoi(optarg);
                                }
                                break;
                        case OPTION_DECODER_USE_MEMCPY:
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_decoder_use_memcpy = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_decoder_use_memcpy = 0;
                                } else {
                                        option_decoder_use_memcpy = !!atoi(optarg);
                                }
                                break;
                        case OPTION_JSONIFY:
                                if (strcasecmp(optarg, "t") == 0 ||
                                    strcasecmp(optarg, "true") == 0 ||
                                    strcasecmp(optarg, "y") == 0 ||
                                    strcasecmp(optarg, "yes") == 0) {
                                        option_jsonify = 1;
                                } else if (strcasecmp(optarg, "f") == 0 ||
                                           strcasecmp(optarg, "false") == 0 ||
                                           strcasecmp(optarg, "n") == 0 ||
                                           strcasecmp(optarg, "no") == 0) {
                                        option_jsonify = 0;
                                } else {
                                        option_jsonify = !!atoi(optarg);
                                }
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
                rc = schema_generate_c_encoder(schema, output_file, option_encoder_include_library);
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

out:     return 0;
bail:   if (output_file != NULL &&
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
