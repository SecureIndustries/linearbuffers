
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "debug.h"
#include "schema.h"
#include "schema-private.h"


int schema_generate_js_encoder (struct schema *schema, FILE *fp, int encoder_include_library)
{
        (void) schema;
        (void) fp;
        (void) encoder_include_library;
        return 0;
}

int schema_generate_js_decoder (struct schema *schema, FILE *fp, int decoder_use_memcpy)
{
        (void) schema;
        (void) fp;
        (void) decoder_use_memcpy;
        return 0;
}

int schema_generate_js_jsonify (struct schema *schema, FILE *fp, int decoder_use_memcpy)
{
        (void) schema;
        (void) fp;
        (void) decoder_use_memcpy;
        return 0;
}
