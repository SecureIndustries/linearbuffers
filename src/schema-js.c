
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "debug.h"
#include "schema.h"
#include "schema-private.h"

static int schema_table_has_vector (struct schema_table *schema_table, const char *type)
{
        struct schema_table_field *schema_table_field;
        TAILQ_FOREACH(schema_table_field, &schema_table->fields, list) {
                if ((schema_table_field->container == schema_container_type_vector) &&
                    (strcmp(schema_table_field->type, type) == 0)) {
                        return 1;
                }
        }
        return 0;
}

static int schema_has_vector (struct schema *schema, const char *type)
{
        int rc;
        struct schema_table *schema_table;
        TAILQ_FOREACH(schema_table, &schema->tables, list) {
                rc = schema_table_has_vector(schema_table, type);
                if (rc != 0) {
                        return rc;
                }
        }
        return 0;
}

static int schema_table_has_string (struct schema_table *schema_table)
{
        struct schema_table_field *schema_table_field;
        TAILQ_FOREACH(schema_table_field, &schema_table->fields, list) {
                if (strcmp(schema_table_field->type, "string") == 0) {
                        return 1;
                }
        }
        return 0;
}

static int schema_has_string (struct schema *schema)
{
        int rc;
        struct schema_table *schema_table;
        TAILQ_FOREACH(schema_table, &schema->tables, list) {
                rc = schema_table_has_string(schema_table);
                if (rc != 0) {
                        return rc;
                }
        }
        return 0;
}

static int schema_generate_enum (struct schema *schema, struct schema_enum *anum, FILE *fp)
{
        struct schema_enum_field *anum_field;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (anum == NULL) {
                linearbuffers_errorf("enum is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        fprintf(fp, "typedef %s_t %s_%s_t;\n", anum->type, schema->namespace, anum->name);

        TAILQ_FOREACH(anum_field, &anum->fields, list) {
                if (anum_field->value != NULL) {
                        fprintf(fp, "#define %s_%s_%s ((%s_%s_t) %s_C(%s))\n", schema->namespace, anum->name, anum_field->name, schema->namespace, anum->name, anum->TYPE, anum_field->value);
                } else {
                        fprintf(fp, "#define %s_%s_%s\n", schema->namespace, anum->name, anum_field->name);
                }
        }

        fprintf(fp, "\n");
        fprintf(fp, "__attribute__((unused)) static inline const char * %s_%s_string (%s_%s_t value)\n", schema->namespace, anum->name, schema->namespace, anum->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    switch (value) {\n");
        TAILQ_FOREACH(anum_field, &anum->fields, list) {
                fprintf(fp, "        case %s_%s_%s: return \"%s\";\n", schema->namespace, anum->name, anum_field->name, anum_field->name);
        }
        fprintf(fp, "    }\n");
        fprintf(fp, "    return \"%s\";\n", "");
        fprintf(fp, "}\n");
        fprintf(fp, "__attribute__((unused)) static inline int %s_%s_is_valid (%s_%s_t value)\n", schema->namespace, anum->name, schema->namespace, anum->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    switch (value) {\n");
        TAILQ_FOREACH(anum_field, &anum->fields, list) {
                fprintf(fp, "        case %s_%s_%s: return 1;\n", schema->namespace, anum->name, anum_field->name);
        }
        fprintf(fp, "    }\n");
        fprintf(fp, "    return 0;\n");
        fprintf(fp, "}\n");

        return 0;
bail:   return -1;
}

static int schema_generate_vector_decoder (struct schema *schema, const char *type, int decoder_use_memcpy, FILE *fp)
{
        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }
        if (schema_type_is_scalar(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    return *(%s_t *) memcpy(&count, ((const uint8_t *) decoder), sizeof(count));\n", schema_count_type_name(schema->count_type));
                } else {
                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder));\n", schema_count_type_name(schema->count_type));
                }
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s_t);\n", schema->namespace, type, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline const %s_t * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const %s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline %s_t %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return ((const %s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
        } else if (schema_type_is_float(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    return *(%s_t *) memcpy(&count, ((const uint8_t *) decoder), sizeof(count));\n", schema_count_type_name(schema->count_type));
                } else {
                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder));\n", schema_count_type_name(schema->count_type));
                }
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s);\n", schema->namespace, type, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline const %s * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const %s *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline %s %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return ((const %s *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
        } else if (schema_type_is_enum(schema, type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    return *(%s_t *) memcpy(&count, ((const uint8_t *) decoder), sizeof(count));\n", schema_count_type_name(schema->count_type));
                } else {
                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder));\n", schema_count_type_name(schema->count_type));
                }
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s_%s_t);\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline const %s_%s_t * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const %s_%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema->namespace, type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline %s_%s_t %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return ((const %s_%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", schema->namespace, type, schema_count_type_size(schema->count_type));
                fprintf(fp, "}\n");
        } else if (schema_type_is_string(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    return *(%s_t *) memcpy(&count, ((const uint8_t *) decoder), sizeof(count));\n", schema_count_type_name(schema->count_type));
                } else {
                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder));\n", schema_count_type_name(schema->count_type));
                }
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline const char * %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    %s_t offset;\n", schema_offset_type_name(schema->offset_type));
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t toffset;\n", schema_offset_type_name(schema->offset_type));
                }
                if (decoder_use_memcpy) {
                        fprintf(fp, "    offset  = *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(offset));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type));
                        fprintf(fp, "    offset += *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + offset + (sizeof(offset) * at), sizeof(offset));\n", schema_offset_type_name(schema->offset_type));
                } else {
                        fprintf(fp, "    offset  = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type));
                        fprintf(fp, "    offset += ((%s_t *) (((const uint8_t *) decoder) + offset))[at];\n", schema_offset_type_name(schema->offset_type));
                }
                        fprintf(fp, "    return (const char *) (((const uint8_t *) decoder) + offset);\n");
                fprintf(fp, "}\n");
        } else if (schema_type_is_table(schema, type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    if (decoder == NULL) {\n");
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    return *(%s_t *) memcpy(&count, ((const uint8_t *) decoder), sizeof(count));\n", schema_count_type_name(schema->count_type));
                } else {
                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder));\n", schema_count_type_name(schema->count_type));
                }
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s * %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    %s_t offset;\n", schema_offset_type_name(schema->offset_type));
                if (decoder_use_memcpy) {
                        fprintf(fp, "    %s_t toffset;\n", schema_offset_type_name(schema->offset_type));
                }
                if (decoder_use_memcpy) {
                        fprintf(fp, "    offset  = *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(offset));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type));
                        fprintf(fp, "    offset += *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + offset + (sizeof(offset) * at), sizeof(offset));\n", schema_offset_type_name(schema->offset_type));
                } else {
                        fprintf(fp, "    offset  = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type));
                        fprintf(fp, "    offset += ((%s_t *) (((const uint8_t *) decoder) + offset))[at];\n", schema_offset_type_name(schema->offset_type));
                }
                        fprintf(fp, "    return (const struct %s_%s *) (((const uint8_t *) decoder) + offset);\n", schema->namespace, type);
                fprintf(fp, "}\n");
        }
        return 0;
bail:   return -1;
}

static int schema_generate_vector_encoder (struct schema *schema, const char *type, FILE *fp)
{
        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }
        if (schema_type_is_scalar(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_start_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s);\n", type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", type);
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_cancel_%s(encoder);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push (struct linearbuffers_encoder *encoder, %s_t value)\n", schema->namespace, type, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, value);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_create (struct linearbuffers_encoder *encoder, const %s_t *value, uint64_t count)\n", schema->namespace, type, schema->namespace, type, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_create_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s, &offset, value, count);\n", type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
        } else if (schema_type_is_float(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_start_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s);\n", type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", type);
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_cancel_%s(encoder);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push (struct linearbuffers_encoder *encoder, %s value)\n", schema->namespace, type, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, value);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_create (struct linearbuffers_encoder *encoder, const %s *value, uint64_t count)\n", schema->namespace, type, schema->namespace, type, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_create_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s, &offset, value, count);\n", type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
        } else if (schema_type_is_enum(schema, type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_start_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s);\n", schema_type_get_enum(schema, type)->type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", schema_type_get_enum(schema, type)->type);
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_cancel_%s(encoder);\n", schema_type_get_enum(schema, type)->type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push (struct linearbuffers_encoder *encoder, %s_%s_t value)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, value);\n", schema_type_get_enum(schema, type)->type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_create (struct linearbuffers_encoder *encoder, const %s_%s_t *value, uint64_t count)\n", schema->namespace, type, schema->namespace, type, schema->namespace, schema_type_get_enum(schema, type)->name);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_create_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s, &offset, value, count);\n", schema_type_get_enum(schema, type)->type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
        } else if (schema_type_is_string(type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_start_%s(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s);\n", type, schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", type);
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_cancel_%s(encoder);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push (struct linearbuffers_encoder *encoder, const struct %s_string *value)\n", schema->namespace, type, schema->namespace);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, (uint64_t) (ptrdiff_t) value);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push_create (struct linearbuffers_encoder *encoder, const char *value)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    const struct %s_%s *string;\n", schema->namespace, type);
                fprintf(fp, "    string = %s_%s_create(encoder, value);\n", schema->namespace, type);
                fprintf(fp, "    if (string == NULL) {\n");
                fprintf(fp, "        return -1;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, (uint64_t) (ptrdiff_t) string);\n", type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push_ncreate (struct linearbuffers_encoder *encoder, uint64_t n, const char *value)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    const struct %s_%s *string;\n", schema->namespace, type);
                fprintf(fp, "    string = %s_%s_ncreate(encoder, n, value);\n", schema->namespace, type);
                fprintf(fp, "    if (string == NULL) {\n");
                fprintf(fp, "        return -1;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_%s(encoder, (uint64_t) (ptrdiff_t) string);\n", type);
                fprintf(fp, "}\n");
        } else if (schema_type_is_table(schema, type)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_%s;\n", schema->namespace, type);
                fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_start_table(encoder, linearbuffers_encoder_count_type_%s, linearbuffers_encoder_offset_type_%s);\n", schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type));
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_vector_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_vector_end_table(encoder, &offset);\n");
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_%s_vector *) (ptrdiff_t) offset;\n", schema->namespace, type);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_cancel_table(encoder);\n");
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_vector_push (struct linearbuffers_encoder *encoder, const struct %s_%s *value)\n", schema->namespace, type, schema->namespace, type);
                fprintf(fp, "{\n");
                fprintf(fp, "    return linearbuffers_encoder_vector_push_table(encoder, (uint64_t) (ptrdiff_t) value);\n");
                fprintf(fp, "}\n");
        }
        return 0;
bail:   return -1;
}

TAILQ_HEAD(element_entries, element_entry);
struct element_entry {
        TAILQ_ENTRY(element_entry) list;
        struct schema_table *table;
        uint64_t id;
        uint64_t offset;
        int vector;
};

struct element {
        struct element_entries entries;
        uint64_t nentries;
};

static void element_entry_destroy (struct element_entry *entry)
{
        if (entry == NULL) {
                return;
        }
        free(entry);
}

static struct element_entry * element_entry_create (struct schema_table *table, uint64_t id, uint64_t offset, int vector)
{
        struct element_entry *entry;
        entry = malloc(sizeof(struct element_entry));
        if (entry == NULL) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        memset(entry, 0, sizeof(struct element_entry));
        entry->table = table;
        entry->id = id;
        entry->offset = offset;
        entry->vector = vector;
        return entry;
bail:   if (entry != NULL) {
                element_entry_destroy(entry);
        }
        return NULL;
}

static int element_push (struct element *element, struct schema_table *table, uint64_t id, uint64_t offset, int vector)
{
        struct element_entry *entry;
        entry = NULL;
        if (element == NULL) {
                linearbuffers_errorf("element is invalid");
                goto bail;
        }
        entry = element_entry_create(table, id, offset, vector);
        if (entry == NULL) {
                linearbuffers_errorf("can not create element entry");
                goto bail;
        }
        element->nentries += 1;
        TAILQ_INSERT_TAIL(&element->entries, entry, list);
        return 0;
bail:   if (entry != NULL) {
                element_entry_destroy(entry);
        }
        return -1;
}

static int element_pop (struct element *element)
{
        struct element_entry *entry;
        entry = NULL;
        if (element == NULL) {
                linearbuffers_errorf("element is invalid");
                goto bail;
        }
        if (TAILQ_EMPTY(&element->entries)) {
                linearbuffers_errorf("element is empty");
                goto bail;
        }
        entry = TAILQ_LAST(&element->entries, element_entries);
        TAILQ_REMOVE(&element->entries, entry, list);
        element->nentries -= 1;
        element_entry_destroy(entry);
        return 0;
bail:   return -1;
}

static void element_destroy (struct element *element)
{
        struct element_entry *entry;
        struct element_entry *nentry;
        if (element == NULL) {
                return;
        }
        TAILQ_FOREACH_SAFE(entry, &element->entries, list, nentry) {
                TAILQ_REMOVE(&element->entries, entry, list);
                element_entry_destroy(entry);
        }
        free(element);
}

static struct element * element_create (void)
{
        struct element *element;
        element = malloc(sizeof(struct element));
        if (element == NULL) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        memset(element, 0, sizeof(struct element));
        TAILQ_INIT(&element->entries);
        return element;
bail:   if (element != NULL) {
                element_destroy(element);
        }
        return NULL;
}

TAILQ_HEAD(namespace_entries, namespace_entry);
struct namespace_entry {
        TAILQ_ENTRY(namespace_entry) list;
        char *string;
        size_t length;
};

struct namespace {
        struct namespace_entries entries;
        int dirty;
        char *linearized;
        size_t slinearized;
};

static void namespace_entry_destroy (struct namespace_entry *entry)
{
        if (entry == NULL) {
                return;
        }
        if (entry->string != NULL) {
                free(entry->string);
        }
        free(entry);
}

static struct namespace_entry * namespace_entry_create (const char *string, va_list ap)
{
        va_list aq;
        int size;
        struct namespace_entry *entry;
        entry = malloc(sizeof(struct namespace_entry));
        if (entry == NULL) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        memset(entry, 0, sizeof(struct namespace_entry));
        va_copy(aq, ap);
        size = vsnprintf(NULL, 0, string, aq);
        va_end(aq);
        if (size < 0) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        size += 1;
        entry->string = malloc(size);
        if (entry->string == NULL) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        va_copy(aq, ap);
        size = vsnprintf(entry->string, size, string, aq);
        va_end(aq);
        if (size < 0) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        entry->length = strlen(entry->string);
        return entry;
bail:   if (entry != NULL) {
                namespace_entry_destroy(entry);
        }
        return NULL;
}

__attribute__((format(printf, 2, 3))) static int namespace_push (struct namespace *namespace, const char *push, ...)
{
        va_list ap;
        struct namespace_entry *entry;
        entry = NULL;
        if (namespace == NULL) {
                linearbuffers_errorf("namespace is invalid");
                goto bail;
        }
        if (push == NULL) {
                linearbuffers_errorf("push is invalid");
                goto bail;
        }
        va_start(ap, push);
        entry = namespace_entry_create(push, ap);
        va_end(ap);
        if (entry == NULL) {
                linearbuffers_errorf("can not create namespace entry");
                goto bail;
        }
        TAILQ_INSERT_TAIL(&namespace->entries, entry, list);
        namespace->dirty = 1;
        return 0;
bail:   if (entry != NULL) {
                namespace_entry_destroy(entry);
        }
        return -1;
}

static int namespace_pop (struct namespace *namespace)
{
        struct namespace_entry *entry;
        entry = NULL;
        if (namespace == NULL) {
                linearbuffers_errorf("namespace is invalid");
                goto bail;
        }
        if (TAILQ_EMPTY(&namespace->entries)) {
                linearbuffers_errorf("namespace is empty");
                goto bail;
        }
        entry = TAILQ_LAST(&namespace->entries, namespace_entries);
        TAILQ_REMOVE(&namespace->entries, entry, list);
        namespace_entry_destroy(entry);
        namespace->dirty = 1;
        return 0;
bail:   if (entry != NULL) {
                namespace_entry_destroy(entry);
        }
        return -1;
}

static const char * namespace_linearized (struct namespace *namespace)
{
        size_t slinearized;
        struct namespace_entry *entry;
        if (namespace == NULL) {
                return NULL;
        }
        if (namespace->dirty == 0) {
                return namespace->linearized;
        }
        slinearized = 0;
        TAILQ_FOREACH(entry, &namespace->entries, list) {
                slinearized += entry->length;
        }
        if (slinearized > namespace->slinearized) {
                if (namespace->linearized != NULL) {
                        free(namespace->linearized);
                        namespace->linearized = NULL;
                }
                namespace->linearized = malloc(slinearized + 1 + 4);
                if (namespace->linearized == NULL) {
                        linearbuffers_errorf("can not allocate memory");
                        namespace->slinearized = 0;
                        goto bail;
                }
                namespace->slinearized = slinearized;
        }
        slinearized = 0;
        TAILQ_FOREACH(entry, &namespace->entries, list) {
                slinearized += sprintf(namespace->linearized + slinearized, "%s", entry->string);
        }
        namespace->dirty = 0;
        return namespace->linearized;
bail:   return NULL;
}

static void namespace_destroy (struct namespace *namespace)
{
        struct namespace_entry *entry;
        struct namespace_entry *nentry;
        if (namespace == NULL) {
                return;
        }
        TAILQ_FOREACH_SAFE(entry, &namespace->entries, list, nentry) {
                TAILQ_REMOVE(&namespace->entries, entry, list);
                namespace_entry_destroy(entry);
        }
        free(namespace);
}

static struct namespace * namespace_create (void)
{
        struct namespace *namespace;
        namespace = malloc(sizeof(struct namespace));
        if (namespace == NULL) {
                linearbuffers_errorf("can not allocate memory");
                goto bail;
        }
        memset(namespace, 0, sizeof(struct namespace));
        TAILQ_INIT(&namespace->entries);
        return namespace;
bail:   if (namespace != NULL) {
                namespace_destroy(namespace);
        }
        return NULL;
}

static int schema_generate_encoder_table (struct schema *schema, struct schema_table *table, FILE *fp)
{
        uint64_t table_field_i;
        uint64_t table_field_s;
        struct schema_table_field *table_field;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (table == NULL) {
                linearbuffers_errorf("table is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        table_field_s = 0;
        TAILQ_FOREACH(table_field, &table->fields, list) {
                if (table_field->container == schema_container_type_vector) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_scalar(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_float(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_string(table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_enum(schema, table_field->type)) {
                        table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
                } else if (schema_type_is_table(schema, table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else {
                        linearbuffers_errorf("type is invalid: %s", table_field->type);
                        goto bail;
                }

        }

        fprintf(fp, "\n");
        fprintf(fp, "function %s_%s_start (encoder)\n", schema->namespace, table->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    return encoder.tableStart(encoder.LinearBufferEncoderCountType.%s, encoder.LinearBufferEncoderOffsetType.%s, %" PRIu64 ", %" PRIu64 ");\n", schema_count_type_name(schema->count_type), schema_offset_type_name(schema->offset_type), table->nfields, table_field_s);
        fprintf(fp, "}\n");

        table_field_i = 0;
        table_field_s = 0;
        TAILQ_FOREACH(table_field, &table->fields, list) {
                struct schema_attribute *attribute;
                struct namespace *attribute_string;
                attribute_string = namespace_create();
                namespace_push(attribute_string, "__attribute__ (( unused");
                TAILQ_FOREACH(attribute, &table_field->attributes, list) {
                        if (strcmp(attribute->name, "deprecated") == 0) {
                                if (attribute->value == NULL ||
                                    strcmp(attribute->value, "1") == 0 ||
                                    strcmp(attribute->value, "yes") == 0 ||
                                    strcmp(attribute->value, "true") == 0) {
                                        namespace_push(attribute_string, ", deprecated");
                                }
                        }
                }
                namespace_push(attribute_string, " )) static inline");

                if (table_field->container == schema_container_type_vector) {
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value);\n", table_field_i, table_field_s);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_create (struct linearbuffers_encoder *encoder, const %s_t *values, uint64_t count)\n", schema->namespace, table->name, table_field->name, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    const struct %s_%s_vector *vector;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    vector = %s_%s_vector_create(encoder, values, count);\n", schema->namespace, table_field->type);
                                fprintf(fp, "    if (vector == NULL) {\n");
                                fprintf(fp, "        return -1;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    return %s_%s_%s_set(encoder, vector);\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_cancel(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s_t value)\n", schema->namespace, table->name, table_field->name, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value);\n", table_field_i, table_field_s);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_create (struct linearbuffers_encoder *encoder, const %s *values, uint64_t count)\n", schema->namespace, table->name, table_field->name, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    const struct %s_%s_vector *vector;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    vector = %s_%s_vector_create(encoder, values, count);\n", schema->namespace, table_field->type);
                                fprintf(fp, "    if (vector == NULL) {\n");
                                fprintf(fp, "        return -1;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    return %s_%s_%s_set(encoder, vector);\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_cancel(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s value)\n", schema->namespace, table->name, table_field->name, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value);\n", table_field_i, table_field_s);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_create (struct linearbuffers_encoder *encoder, const %s_%s_t *values, uint64_t count)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    const struct %s_%s_vector *vector;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    vector = %s_%s_vector_create(encoder, values, count);\n", schema->namespace, table_field->type);
                                fprintf(fp, "    if (vector == NULL) {\n");
                                fprintf(fp, "        return -1;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    return %s_%s_%s_set(encoder, vector);\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_cancel(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s_%s_t value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_string_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value);\n", table_field_i, table_field_s);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_create (struct linearbuffers_encoder *encoder, const char **values, uint64_t count)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    int rc;\n");
                                fprintf(fp, "    uint64_t i;\n");
                                fprintf(fp, "    const struct %s_%s *string;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    const struct %s_%s_vector *vector;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    rc = %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "    if (rc != 0) {\n");
                                fprintf(fp, "        return rc;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    for (i = 0; i < count; i++) {\n");
                                fprintf(fp, "        string = %s_%s_create(encoder, values[i]);\n", schema->namespace, table_field->type);
                                fprintf(fp, "        if (string == NULL) {\n");
                                fprintf(fp, "            return -1;\n");
                                fprintf(fp, "        }\n");
                                fprintf(fp, "        rc = %s_%s_vector_push(encoder, string);\n", schema->namespace, table_field->type);
                                fprintf(fp, "        if (rc != 0) {\n");
                                fprintf(fp, "            return rc;\n");
                                fprintf(fp, "        }\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    vector = %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "    if (vector == NULL) {\n");
                                fprintf(fp, "        return -1;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    return %s_%s_%s_set(encoder, vector);\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_cancel(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, const struct %s_%s *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push_create (struct linearbuffers_encoder *encoder, const char *value)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push_create(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push_ncreate (struct linearbuffers_encoder *encoder, uint64_t n, const char *value)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push_ncreate(encoder, n, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_table(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value);\n", table_field_i, table_field_s);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_start(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_end(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_cancel(encoder);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                                fprintf(fp, "__attribute__((unused)) static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, const struct %s_%s *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_push(encoder, value);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        } else {
                                linearbuffers_errorf("type is invalid: %s", table_field->type);
                                goto bail;
                        }
                } else {
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "%s int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, table_field->type, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "%s int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, table_field->type, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "%s int %s_%s_%s_create (struct linearbuffers_encoder *encoder, const char *value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    int rc;\n");
                                fprintf(fp, "    uint64_t offset;\n");
                                fprintf(fp, "    rc = linearbuffers_encoder_string_create(encoder, &offset, value_%s);\n", table_field->name);
                                fprintf(fp, "    if (rc != 0) { \n");
                                fprintf(fp, "        return rc;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), offset);\n", table_field->type, table_field_i, table_field_s);
                                fprintf(fp, "    return rc;\n");
                                fprintf(fp, "}\n");
                                fprintf(fp, "%s int %s_%s_%s_createf (struct linearbuffers_encoder *encoder, const char *value_%s, ...)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    int rc;\n");
                                fprintf(fp, "    va_list ap;\n");
                                fprintf(fp, "    uint64_t offset;\n");
                                fprintf(fp, "    va_start(ap, value_%s);\n", table_field->name);
                                fprintf(fp, "    rc = linearbuffers_encoder_string_createv(encoder, &offset, value_%s, ap);\n", table_field->name);
                                fprintf(fp, "    va_end(ap);\n");
                                fprintf(fp, "    if (rc != 0) { \n");
                                fprintf(fp, "        return rc;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), offset);\n", table_field->type, table_field_i, table_field_s);
                                fprintf(fp, "    return rc;\n");
                                fprintf(fp, "}\n");
                                fprintf(fp, "%s int %s_%s_%s_ncreate (struct linearbuffers_encoder *encoder, uint64_t n, const char *value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    int rc;\n");
                                fprintf(fp, "    uint64_t offset;\n");
                                fprintf(fp, "    rc = linearbuffers_encoder_string_ncreate(encoder, &offset, n, value_%s);\n", table_field->name);
                                fprintf(fp, "    if (rc != 0) { \n");
                                fprintf(fp, "        return rc;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), offset);\n", table_field->type, table_field_i, table_field_s);
                                fprintf(fp, "    return rc;\n");
                                fprintf(fp, "}\n");
                                fprintf(fp, "%s int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_string *value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, schema->namespace, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s_%s_t value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", schema_type_get_enum(schema, table_field->type)->type, table_field_i, table_field_s, table_field->name);
                                fprintf(fp, "}\n");
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "%s int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s *value_%s)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return linearbuffers_encoder_table_set_table(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) (ptrdiff_t) value_%s);\n", table_field_i, table_field_s, table_field->name);
                                fprintf(fp, "}\n");
                        } else {
                                linearbuffers_errorf("type is invalid: %s", table_field->type);
                                goto bail;
                        }
                }
                table_field_i += 1;
                if (table_field->container == schema_container_type_vector) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_scalar(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_float(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_string(table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_enum(schema, table_field->type)) {
                        table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
                } else if (schema_type_is_table(schema, table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                }
                namespace_destroy(attribute_string);
        }

        fprintf(fp, "function %s_%s_end (encoder)\n", schema->namespace, table->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    var offset;\n");
        fprintf(fp, "    offset = encoder.tableEnd();\n");
        fprintf(fp, "    if (offset < 0) {\n");
        fprintf(fp, "        return -1;\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    return offset;\n");
        fprintf(fp, "}\n");

        fprintf(fp, "function %s_%s_cancel (encoder)\n", schema->namespace, table->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    return encoder.tableCancel();\n");
        fprintf(fp, "}\n");

        fprintf(fp, "function %s_%s_finish (encoder)\n", schema->namespace, table->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    var offset;\n");
        fprintf(fp, "    offset = encoder.tableEnd();\n");
        fprintf(fp, "    if (offset < 0) {\n");
        fprintf(fp, "        return -1;\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    return 0;\n");
        fprintf(fp, "}\n");

        return 0;
bail:   return -1;
}

int schema_generate_js_encoder (struct schema *schema, FILE *fp, int encoder_include_library)
{
        int rc;

        struct schema_enum *anum;
        struct schema_table *table;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        if (encoder_include_library == 0) {
        }

        TAILQ_FOREACH(anum, &schema->enums, list) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, anum->name);
                fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, anum->name);
                fprintf(fp, "\n");

                schema_generate_enum(schema, anum, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        if (schema_has_string(schema)) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_STRING_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_STRING_ENCODER_API\n", schema->NAMESPACE);

                fprintf(fp, "\n");
                fprintf(fp, "struct %s_string;\n", schema->namespace);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_string * %s_string_createv (struct linearbuffers_encoder *encoder, const char *value, va_list va)\n", schema->namespace, schema->namespace);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_string_createv(encoder, &offset, value, va);\n");
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_string *) (ptrdiff_t) offset;\n", schema->namespace);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_string * %s_string_create (struct linearbuffers_encoder *encoder, const char *value)\n", schema->namespace, schema->namespace);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_string_create(encoder, &offset, value);\n");
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_string *) (ptrdiff_t) offset;\n", schema->namespace);
                fprintf(fp, "}\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const struct %s_string * %s_string_ncreate (struct linearbuffers_encoder *encoder, uint64_t n, const char *value)\n", schema->namespace, schema->namespace);
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    uint64_t offset;\n");
                fprintf(fp, "    rc = linearbuffers_encoder_string_ncreate(encoder, &offset, n, value);\n");
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        return NULL;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return (const struct %s_string *) (ptrdiff_t) offset;\n", schema->namespace);
                fprintf(fp, "}\n");

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        rc = 0;
        if (schema_has_vector(schema, "int8")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int8_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int8_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "int8", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int16")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int16_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int16_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "int16", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int32")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int32_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int32_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "int32", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int64")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int64_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int64_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "int64", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint8")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint8_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint8_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "uint8", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint16")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint16_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint16_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "uint16", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint32")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint32_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint32_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "uint32", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint64")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint64_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint64_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "uint64", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "float")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_float_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_float_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "float", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "double")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_double_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_double_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "double", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "string")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_string_VECTOR_ENCODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_string_VECTOR_ENCODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_encoder(schema, "string", fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        TAILQ_FOREACH(anum, &schema->enums, list) {
                if (schema_has_vector(schema, anum->name)) {
                        fprintf(fp, "\n");
                        fprintf(fp, "#if !defined(%s_%s_VECTOR_ENCODER_API)\n", schema->NAMESPACE, anum->name);
                        fprintf(fp, "#define %s_%s_VECTOR_ENCODER_API\n", schema->NAMESPACE, anum->name);

                        rc |= schema_generate_vector_encoder(schema, anum->name, fp);

                        fprintf(fp, "\n");
                        fprintf(fp, "#endif\n");
                }
        }
        TAILQ_FOREACH(table, &schema->tables, list) {
                if (schema_has_vector(schema, table->name)) {
                        fprintf(fp, "\n");
                        fprintf(fp, "#if !defined(%s_%s_VECTOR_ENCODER_API)\n", schema->NAMESPACE, table->name);
                        fprintf(fp, "#define %s_%s_VECTOR_ENCODER_API\n", schema->NAMESPACE, table->name);

                        rc |= schema_generate_vector_encoder(schema, table->name, fp);

                        fprintf(fp, "\n");
                        fprintf(fp, "#endif\n");
                }
        }
        if (rc != 0) {
                linearbuffers_errorf("can not generate decoder for vector: %s", table->name);
                goto bail;
        }

        TAILQ_FOREACH(table, &schema->tables, list) {
                rc = schema_generate_encoder_table(schema, table, fp);
                if (rc != 0) {
                        linearbuffers_errorf("can not generate decoder for table: %s", table->name);
                        goto bail;
                }
        }

        return 0;
bail:   return -1;
}

static int schema_generate_decoder_table (struct schema *schema, struct schema_table *table, int decoder_use_memcpy, FILE *fp)
{
        uint64_t table_field_i;
        uint64_t table_field_s;
        struct schema_table_field *table_field;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (table == NULL) {
                linearbuffers_errorf("table is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        fprintf(fp, "\n");
        fprintf(fp, "struct %s_%s;\n", schema->namespace, table->name);
        fprintf(fp, "\n");

        fprintf(fp, "__attribute__((unused)) static inline const struct %s_%s * %s_%s_decode (const void *buffer, uint64_t length)\n", schema->namespace, table->name, schema->namespace, table->name);
        fprintf(fp, "{\n");
        fprintf(fp, "    (void) length;\n");
        fprintf(fp, "    return (const struct %s_%s *) buffer;\n", schema->namespace, table->name);
        fprintf(fp, "}\n");

        table_field_i = 0;
        table_field_s = 0;
        TAILQ_FOREACH(table_field, &table->fields, list) {
                struct schema_attribute *attribute;
                struct namespace *attribute_string;
                attribute_string = namespace_create();
                namespace_push(attribute_string, "__attribute__ (( unused");
                TAILQ_FOREACH(attribute, &table_field->attributes, list) {
                        if (strcmp(attribute->name, "deprecated") == 0) {
                                if (attribute->value == NULL ||
                                    strcmp(attribute->value, "1") == 0 ||
                                    strcmp(attribute->value, "yes") == 0 ||
                                    strcmp(attribute->value, "true") == 0) {
                                }
                        }
                }
                namespace_push(attribute_string, " )) static inline");

                fprintf(fp, "%s int %s_%s_%s_present (const struct %s_%s *decoder)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                fprintf(fp, "{\n");
                fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                fprintf(fp, "    uint8_t present;\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    memcpy(&count, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(count));\n", UINT64_C(0));
                } else {
                        fprintf(fp, "    count = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_name(schema->count_type), UINT64_C(0));
                }
                fprintf(fp, "    if (UINT64_C(%" PRIu64 ") >= count) {\n", table_field_i);
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                if (decoder_use_memcpy) {
                        fprintf(fp, "    memcpy(&present, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(present));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                } else {
                        fprintf(fp, "    present = *(uint8_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                }
                fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (table_field_i % 8)));
                fprintf(fp, "        return 0;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return 1;\n");
                fprintf(fp, "}\n");

                if (table_field->container == schema_container_type_vector) {
                        fprintf(fp, "%s ", namespace_linearized(attribute_string));
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "const struct %s_%s_vector * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "const struct %s_%s_vector * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "const struct %s_%s_vector * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "const struct %s_%s_vector * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "const struct %s_%s_vector * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        }
                        fprintf(fp, "{\n");
                        fprintf(fp, "    %s_t offset;\n", schema_offset_type_name(schema->offset_type));
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    %s_t toffset;\n", schema_offset_type_name(schema->offset_type));
                        }
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    uint8_t present;\n");
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    memcpy(&count, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(count));\n", UINT64_C(0));
                        } else {
                                fprintf(fp, "    count = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_name(schema->count_type), UINT64_C(0));
                        }
                        fprintf(fp, "    if (UINT64_C(%" PRIu64 ") >= count) {\n", table_field_i);
                        fprintf(fp, "        return NULL;\n");
                        fprintf(fp, "    }\n");
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    memcpy(&present, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(present));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                        } else {
                                fprintf(fp, "    present = *(uint8_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                        }
                        fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (table_field_i % 8)));
                        fprintf(fp, "        return NULL;\n");
                        fprintf(fp, "    }\n");
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    offset = *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(offset));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                        } else {
                                fprintf(fp, "    offset = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                        }
                        fprintf(fp, "    return (const struct %s_%s_vector *) (((const uint8_t *) decoder) + offset);\n", schema->namespace, table_field->type);
                        fprintf(fp, "}\n");

                        fprintf(fp, "%s ", namespace_linearized(attribute_string));
                        fprintf(fp, "uint64_t %s_%s_%s_get_count (const struct %s_%s *decoder)\n", schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        fprintf(fp, "{\n");
                        fprintf(fp, "    return %s_%s_vector_get_count(%s_%s_%s_get(decoder));\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                        fprintf(fp, "}\n");

                        if (schema_type_is_scalar(table_field->type) ||
                            schema_type_is_float(table_field->type) ||
                            schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s ", namespace_linearized(attribute_string));
                                fprintf(fp, "uint64_t %s_%s_%s_get_length (const struct %s_%s *decoder)\n", schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_get_length(%s_%s_%s_get(decoder));\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");

                                fprintf(fp, "%s ", namespace_linearized(attribute_string));
                                if (schema_type_is_scalar(table_field->type)) {
                                        fprintf(fp, "const %s_t * %s_%s_%s_get_values (const struct %s_%s *decoder)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                                } else if (schema_type_is_float(table_field->type)) {
                                        fprintf(fp, "const %s * %s_%s_%s_get_values (const struct %s_%s *decoder)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                                } else if (schema_type_is_enum(schema, table_field->type)) {
                                        fprintf(fp, "const %s_%s_t * %s_%s_%s_get_values (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                                }
                                fprintf(fp, "{\n");
                                fprintf(fp, "    return %s_%s_vector_get_values(%s_%s_%s_get(decoder));\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                                fprintf(fp, "}\n");
                        }

                        fprintf(fp, "%s ", namespace_linearized(attribute_string));
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "%s_t %s_%s_%s_get_at (const struct %s_%s *decoder, uint64_t at)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "%s %s_%s_%s_get_at (const struct %s_%s *decoder, uint64_t at)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s_%s_t %s_%s_%s_get_at (const struct %s_%s *decoder, uint64_t at)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "const char * %s_%s_%s_get_at (const struct %s_%s *decoder, uint64_t at)\n", schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "const struct %s_%s * %s_%s_%s_get_at (const struct %s_%s *decoder, uint64_t at)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        }
                        fprintf(fp, "{\n");
                        fprintf(fp, "    return %s_%s_vector_get_at(%s_%s_%s_get(decoder), at);\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
                        fprintf(fp, "}\n");
                } else {
                        fprintf(fp, "%s ", namespace_linearized(attribute_string));
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "%s_t %s_%s_%s_get (const struct %s_%s *decoder)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "%s %s_%s_%s_get (const struct %s_%s *decoder)\n", table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "const struct %s_%s * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s_%s_t %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        } else         if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "const struct %s_%s * %s_%s_%s_get (const struct %s_%s *decoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                        }
                        fprintf(fp, "{\n");
                        fprintf(fp, "    %s_t count;\n", schema_count_type_name(schema->count_type));
                        fprintf(fp, "    uint8_t present;\n");
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    memcpy(&count, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(count));\n", UINT64_C(0));
                        } else {
                                fprintf(fp, "    count = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_name(schema->count_type), UINT64_C(0));
                        }
                        fprintf(fp, "    if (UINT64_C(%" PRIu64 ") >= count) {\n", table_field_i);
                        if (schema_type_is_scalar(table_field->type) ||
                            schema_type_is_float(table_field->type)) {
                                fprintf(fp, "        return %s;\n", (table_field->value) ? table_field->value : "0");
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                if (table_field->value != NULL) {
                                        fprintf(fp, "        return %s_%s_%s;\n", schema->namespace, table_field->type, table_field->value);
                                } else {
                                        fprintf(fp, "        return 0;\n");
                                }
                        } else if (schema_type_is_string(table_field->type)) {
                                if (table_field->value != NULL) {
                                        fprintf(fp, "        return (const struct %s_%s *) \"%s\";\n", schema->namespace, table_field->type, table_field->value);
                                } else {
                                        fprintf(fp, "        return NULL;\n");
                                }
                        } else {
                                fprintf(fp, "        return NULL;\n");
                        }
                        fprintf(fp, "    }\n");
                        if (decoder_use_memcpy) {
                                fprintf(fp, "    memcpy(&present, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "), sizeof(present));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                        } else {
                                fprintf(fp, "    present = *(uint8_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 "));\n", schema_count_type_size(schema->count_type) + sizeof(uint8_t) * (table_field_i / 8));
                        }
                        fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (table_field_i % 8)));
                        if (schema_type_is_scalar(table_field->type) ||
                            schema_type_is_float(table_field->type)) {
                                fprintf(fp, "        return %s;\n", (table_field->value) ? table_field->value : "0");
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                if (table_field->value != NULL) {
                                        fprintf(fp, "        return %s_%s_%s;\n", schema->namespace, table_field->type, table_field->value);
                                } else {
                                        fprintf(fp, "        return 0;\n");
                                }
                        } else {
                                fprintf(fp, "        return NULL;\n");
                        }
                        fprintf(fp, "    }\n");
                        if (schema_type_is_scalar(table_field->type)) {
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    %s_t value;\n", table_field->type);
                                        fprintf(fp, "    return *(%s_t *) memcpy(&value, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(value));\n", table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                } else {
                                        fprintf(fp, "    return *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                }
                        } else if (schema_type_is_float(table_field->type)) {
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    %s value;\n", table_field->type);
                                        fprintf(fp, "    return *(%s *) memcpy(&value, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(value));\n", table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                } else {
                                        fprintf(fp, "    return *(%s *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                }
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    %s_%s_t value;\n", schema->namespace, table_field->type);
                                        fprintf(fp, "    return *(%s_%s_t *) memcpy(&value, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(value));\n", schema->namespace, table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                } else {
                                        fprintf(fp, "    return *(%s_%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", schema->namespace, table_field->type, schema_count_type_size(schema->count_type), table_field_s);
                                }
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "    %s_t offset;\n", schema_offset_type_name(schema->offset_type));
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    %s_t toffset;\n", schema_offset_type_name(schema->offset_type));
                                }
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    offset = *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(offset));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                                } else {
                                        fprintf(fp, "    offset = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                                }
                                fprintf(fp, "    return (const struct %s_%s *) (((const uint8_t *) decoder) + offset);\n", schema->namespace, table_field->type);
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "    %s_t offset;\n", schema_offset_type_name(schema->offset_type));
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    %s_t toffset;\n", schema_offset_type_name(schema->offset_type));
                                }
                                if (decoder_use_memcpy) {
                                        fprintf(fp, "    offset = *(%s_t *) memcpy(&toffset, ((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "), sizeof(offset));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                                } else {
                                        fprintf(fp, "    offset = *(%s_t *) (((const uint8_t *) decoder) + UINT64_C(%" PRIu64 ") + ((count + 7) / 8) + UINT64_C(%" PRIu64 "));\n", schema_offset_type_name(schema->offset_type), schema_count_type_size(schema->count_type), table_field_s);
                                }
                                fprintf(fp, "    return (const struct %s_%s *) (((const uint8_t *) decoder) + offset);\n", schema->namespace, table_field->type);
                        }
                        fprintf(fp, "}\n");

                        if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "%s const char * %s_%s_%s_get_value (const struct %s_%s *decoder)\n", namespace_linearized(attribute_string), schema->namespace, table->name, table_field->name, schema->namespace, table->name);
                                fprintf(fp, "{\n");
                                fprintf(fp, "    const struct %s_%s *string;\n", schema->namespace, table_field->type);
                                fprintf(fp, "    string = %s_%s_%s_get(decoder);\n", schema->namespace, table->name, table_field->name);
                                fprintf(fp, "    if (string == NULL) {\n");
                                fprintf(fp, "        return NULL;\n");
                                fprintf(fp, "    }\n");
                                fprintf(fp, "    return %s_%s_value(string);\n", schema->namespace, table_field->type);
                                fprintf(fp, "}\n");
                        }
                }
                table_field_i += 1;
                if (table_field->container == schema_container_type_vector) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_scalar(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_float(table_field->type)) {
                        table_field_s += schema_inttype_size(table_field->type);
                } else if (schema_type_is_string(table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                } else if (schema_type_is_enum(schema, table_field->type)) {
                        table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
                } else if (schema_type_is_table(schema, table_field->type)) {
                        table_field_s += schema_offset_type_size(schema->offset_type);
                }
                namespace_destroy(attribute_string);
        }

        return 0;
bail:   return -1;
}

int schema_generate_js_decoder (struct schema *schema, FILE *fp, int decoder_use_memcpy)
{
        int rc;

        struct schema_enum *anum;
        struct schema_table *table;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        fprintf(fp, "\n");
        fprintf(fp, "#include <stddef.h>\n");
        fprintf(fp, "#include <stdint.h>\n");
        fprintf(fp, "#include <string.h>\n");

        if (!TAILQ_EMPTY(&schema->enums)) {
                TAILQ_FOREACH(anum, &schema->enums, list) {
                        fprintf(fp, "\n");
                        fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, anum->name);
                        fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, anum->name);
                        fprintf(fp, "\n");

                        schema_generate_enum(schema, anum, fp);

                        fprintf(fp, "\n");
                        fprintf(fp, "#endif\n");
                }
        }

        if (schema_has_string(schema)) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_STRING_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_STRING_DECODER_API\n", schema->NAMESPACE);

                fprintf(fp, "\n");
                fprintf(fp, "struct %s_string;\n", schema->namespace);
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused, warn_unused_result)) static inline const char * %s_string_value (const struct %s_string *string)\n", schema->namespace, schema->namespace);
                fprintf(fp, "{\n");
                fprintf(fp, "    return (const char *) string;\n");
                fprintf(fp, "}\n");

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        rc = 0;
        if (schema_has_vector(schema, "int8")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int8_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int8_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "int8", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int16")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int16_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int16_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "int16", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int32")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int32_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int32_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "int32", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "int64")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_int64_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_int64_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "int64", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint8")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint8_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint8_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "uint8", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint16")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint16_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint16_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "uint16", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint32")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint32_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint32_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "uint32", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "uint64")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_uint64_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_uint64_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "uint64", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "float")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_float_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_float_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "float", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "double")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_double_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_double_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "double", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }
        if (schema_has_vector(schema, "string")) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_string_VECTOR_DECODER_API)\n", schema->NAMESPACE);
                fprintf(fp, "#define %s_string_VECTOR_DECODER_API\n", schema->NAMESPACE);

                rc |= schema_generate_vector_decoder(schema, "string", decoder_use_memcpy, fp);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        TAILQ_FOREACH(anum, &schema->enums, list) {
                if (schema_has_vector(schema, anum->name)) {
                        fprintf(fp, "\n");
                        fprintf(fp, "#if !defined(%s_%s_VECTOR_DECODER_API)\n", schema->NAMESPACE, anum->name);
                        fprintf(fp, "#define %s_%s_VECTOR_DECODER_API\n", schema->NAMESPACE, anum->name);

                        rc |= schema_generate_vector_decoder(schema, anum->name, decoder_use_memcpy, fp);

                        fprintf(fp, "\n");
                        fprintf(fp, "#endif\n");
                }
        }
        TAILQ_FOREACH(table, &schema->tables, list) {
                if (schema_has_vector(schema, table->name)) {
                        fprintf(fp, "\n");
                        fprintf(fp, "#if !defined(%s_%s_VECTOR_DECODER_API)\n", schema->NAMESPACE, table->name);
                        fprintf(fp, "#define %s_%s_VECTOR_DECODER_API\n", schema->NAMESPACE, table->name);

                        rc |= schema_generate_vector_decoder(schema, table->name, decoder_use_memcpy, fp);

                        fprintf(fp, "\n");
                        fprintf(fp, "#endif\n");
                }
        }
        if (rc != 0) {
                linearbuffers_errorf("can not generate decoder for vector: %s", table->name);
                goto bail;
        }

        TAILQ_FOREACH(table, &schema->tables, list) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_%s_DECODER_API)\n", schema->NAMESPACE, table->name);
                fprintf(fp, "#define %s_%s_DECODER_API\n", schema->NAMESPACE, table->name);

                rc = schema_generate_decoder_table(schema, table, decoder_use_memcpy, fp);
                if (rc != 0) {
                        linearbuffers_errorf("can not generate decoder for table: %s", table->name);
                        goto bail;
                }

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        return 0;
bail:   return -1;
}

static int schema_generate_jsonify_table (struct schema *schema, struct schema_table *head, struct schema_table *table, struct namespace *namespace, struct element *element, FILE *fp)
{
        int rc;
        char *prefix;
        uint64_t prefix_count;

        uint64_t table_field_i;
        struct schema_table_field *table_field;
        struct schema_table_field *ntable_field;

        struct element_entry *element_entry;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (table == NULL) {
                linearbuffers_errorf("table is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        prefix_count = element->nentries + 1;
        TAILQ_FOREACH(element_entry, &element->entries, list) {
                if (element_entry->vector) {
                        prefix_count += 1;
                }
        }

        prefix = malloc((prefix_count * 4) + 1);
        memset(prefix, ' ', prefix_count * 4);
        prefix[prefix_count * 4] = '\0';

        if (TAILQ_EMPTY(&element->entries)) {
                fprintf(fp, "__attribute__((unused)) static inline int %s_jsonify (const struct %s *%s, unsigned int flags, int (*emitter) (void *context, const char *format, ...), void *context)\n", namespace_linearized(namespace), namespace_linearized(namespace), namespace_linearized(namespace));
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    if (%s == NULL) {\n", namespace_linearized(namespace));
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    if (emitter == NULL) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    rc = emitter(context, \"{\");\n");
                fprintf(fp, "    if (rc < 0) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n");
                fprintf(fp, "        rc = emitter(context, \"\\n\");\n");
                fprintf(fp, "        if (rc < 0) {\n");
                fprintf(fp, "            goto bail;\n");
                fprintf(fp, "        }\n");
                fprintf(fp, "    }\n");
        }

        table_field_i = 0;
        TAILQ_FOREACH(table_field, &table->fields, list) {
                fprintf(fp, "%sif (%s_%s_%s_present(%s)) {\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                if (table_field->container == schema_container_type_vector) {
                        fprintf(fp, "%s    uint64_t at_%" PRIu64 ";\n", prefix, element->nentries);
                        fprintf(fp, "%s    uint64_t count;\n", prefix);
                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s_vector *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s_vector *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s_vector *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s_vector *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s_vector *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        }
                        fprintf(fp, "%s    %s_%s = %s_%s_%s_get(%s);\n", prefix, namespace_linearized(namespace), table_field->name, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                        fprintf(fp, "%s    count = %s_%s_vector_get_count(%s_%s);\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                        fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                        fprintf(fp, "%s        rc = emitter(context, \"%s\");\n", prefix, prefix);
                        fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s            goto bail;\n", prefix);
                        fprintf(fp, "%s        }\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);
                        fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\": [\");\n", prefix, table_field->name);
                        fprintf(fp, "%s    if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s        goto bail;\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);
                        fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                        fprintf(fp, "%s        rc = emitter(context, \"\\n\");\n", prefix);
                        fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s            goto bail;\n", prefix);
                        fprintf(fp, "%s        }\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);

                        fprintf(fp, "%s    for (at_%" PRIu64 " = 0; at_%" PRIu64 " < count; at_%" PRIu64 "++) {\n", prefix, element->nentries, element->nentries, element->nentries);

                        fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                        fprintf(fp, "%s            rc = emitter(context, \"%s    \");\n", prefix, prefix);
                        fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s                goto bail;\n", prefix);
                        fprintf(fp, "%s            }\n", prefix);
                        fprintf(fp, "%s        }\n", prefix);

                        if (schema_type_is_scalar(table_field->type)) {
                                fprintf(fp, "%s        %s_t value;\n", prefix, table_field->type);
                                fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
                                if (strncmp(table_field->type, "int", 3) == 0) {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIi64 \"%%s\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                } else {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIu64 \"%%s\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                }
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        } else if (schema_type_is_float(table_field->type)) {
                                fprintf(fp, "%s        %s value;\n", prefix, table_field->type);
                                fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
                                if (strncmp(table_field->type, "int", 3) == 0) {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIi64 \"%%s\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                } else {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIu64 \"%%s\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                }
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        } else if (schema_type_is_enum(schema, table_field->type)) {
                                fprintf(fp, "%s        %s_%s_t value;\n", prefix, schema->namespace, table_field->type);
                                fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
                                if (strncmp(schema_type_get_enum(schema, table_field->type)->type, "int", 3) == 0) {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIi64 \"%%s\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                } else {
                                        fprintf(fp, "%s        rc = emitter(context, \"%%\" PRIu64 \"%%s\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                }
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        } else if (schema_type_is_string(table_field->type)) {
                                fprintf(fp, "%s        const char *value;\n", prefix);
                                fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
                                fprintf(fp, "%s        rc = emitter(context, \"\\\"%%s\\\"%%s\", value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        } else if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "%s        const struct %s_%s *%s_%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, table_field->type);
                                fprintf(fp, "%s        %s_%s_%s = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, namespace_linearized(namespace), table_field->name, table_field->type, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
                                fprintf(fp, "%s        rc = emitter(context, \"{\");\n", prefix);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                namespace_push(namespace, "_%s_%s", table_field->name, table_field->type);
                                element_push(element, table, table_field_i, 0, 1);
                                rc = schema_generate_jsonify_table(schema, head, schema_type_get_table(schema, table_field->type), namespace, element, fp);
                                if (rc != 0) {
                                        linearbuffers_errorf("can not generate decoder for table: %s", table->name);
                                        goto bail;
                                }
                                element_pop(element);
                                namespace_pop(namespace);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"%s    \");\n", prefix, prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        rc = emitter(context, \"}%%s\", ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, element->nentries);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s        if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s            rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s            if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                goto bail;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        } else {
                                linearbuffers_errorf("type is invalid: %s", table_field->type);
                                goto bail;
                        }
                        fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                        fprintf(fp, "%s        rc = emitter(context, \"%s\");\n", prefix, prefix);
                        fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s            goto bail;\n", prefix);
                        fprintf(fp, "%s        }\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);
                        fprintf(fp, "%s    rc = emitter(context, \"]\");\n", prefix);
                        fprintf(fp, "%s    if (rc < 0) {\n", prefix);
                        fprintf(fp, "%s        goto bail;\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);
                } else {
                        if (schema_type_is_scalar(table_field->type) ||
                            schema_type_is_float(table_field->type) ||
                            schema_type_is_string(table_field->type) ||
                            schema_type_is_enum(schema, table_field->type)) {
                                if (schema_type_is_scalar(table_field->type)) {
                                        fprintf(fp, "%s    %s_t value;\n", prefix, table_field->type);
                                        fprintf(fp, "%s    value = %s_%s_%s_get(%s);\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                                } else if (schema_type_is_float(table_field->type)) {
                                        fprintf(fp, "%s    %s value;\n", prefix, table_field->type);
                                        fprintf(fp, "%s    value = %s_%s_%s_get(%s);\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                                } else if (schema_type_is_enum(schema, table_field->type)) {
                                        fprintf(fp, "%s    %s_%s_t value;\n", prefix, schema->namespace, table_field->type);
                                        fprintf(fp, "%s    value = %s_%s_%s_get(%s);\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                                } else if (schema_type_is_string(table_field->type)) {
                                        fprintf(fp, "%s    const char *value;\n", prefix);
                                        fprintf(fp, "%s    value = %s_%s_%s_get_value(%s);\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                                }
                                fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                                fprintf(fp, "%s        rc = emitter(context, \"%s\");\n", prefix, prefix);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                                if (schema_type_is_scalar(table_field->type)) {
                                        if (strncmp(table_field->type, "int", 3) == 0) {
                                                fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":%%\" PRIi64 \"\", (int64_t) value);\n", prefix, table_field->name);
                                        } else {
                                                fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":%%\" PRIu64 \"\", (uint64_t) value);\n", prefix, table_field->name);
                                        }
                                } else if (schema_type_is_float(table_field->type)) {
                                        fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":%%f\", value);\n", prefix, table_field->name);
                                } else if (schema_type_is_enum(schema, table_field->type)) {
                                        if (strncmp(schema_type_get_enum(schema, table_field->type)->type, "int", 3) == 0) {
                                                fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":%%\" PRIi64 \"\", (int64_t) value);\n", prefix, table_field->name);
                                        } else {
                                                fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":%%\" PRIu64 \"\", (uint64_t) value);\n", prefix, table_field->name);
                                        }
                                } else if (schema_type_is_string(table_field->type)) {
                                        fprintf(fp, "%s    rc  = emitter(context, \"\\\"%s\\\":\");\n", prefix, table_field->name);
                                        fprintf(fp, "%s    rc |= %s_jsonify_string_emitter(value, emitter, context);\n", prefix, schema->namespace);
                                }
                                fprintf(fp, "%s    if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s        goto bail;\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        }

                        if (schema_type_is_table(schema, table_field->type)) {
                                fprintf(fp, "%s    const struct %s_%s *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
                                fprintf(fp, "%s    (void) %s_%s;\n", prefix, namespace_linearized(namespace), table_field->name);
                                fprintf(fp, "%s    %s_%s = %s_%s_%s_get(%s);\n", prefix, namespace_linearized(namespace), table_field->name, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
                                fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                                fprintf(fp, "%s        rc = emitter(context, \"%s\");\n", prefix, prefix);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                                fprintf(fp, "%s    rc = emitter(context, \"\\\"%s\\\":{\");\n", prefix, table_field->name);
                                fprintf(fp, "%s    if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s        goto bail;\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                                fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                                fprintf(fp, "%s        rc = emitter(context, \"\\n\");\n", prefix);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                                namespace_push(namespace, "_%s", table_field->name);
                                element_push(element, table, table_field_i, 0, 0);
                                rc = schema_generate_jsonify_table(schema, head, schema_type_get_table(schema, table_field->type), namespace, element, fp);
                                if (rc != 0) {
                                        linearbuffers_errorf("can not generate jsonify for table: %s", table->name);
                                        goto bail;
                                }
                                element_pop(element);
                                namespace_pop(namespace);
                                fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE) {\n", prefix);
                                fprintf(fp, "%s        rc = emitter(context, \"%s\");\n", prefix, prefix);
                                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s            goto bail;\n", prefix);
                                fprintf(fp, "%s        }\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                                fprintf(fp, "%s    rc = emitter(context, \"}\");\n", prefix);
                                fprintf(fp, "%s    if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s        goto bail;\n", prefix);
                                fprintf(fp, "%s    }\n", prefix);
                        }
                }
                if (table_field->list.tqe_next != NULL) {
                        fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_COMMA) {\n", prefix);
                        fprintf(fp, "%s        do {\n", prefix);
                        for (ntable_field = table_field->list.tqe_next; ntable_field; ntable_field = ntable_field->list.tqe_next) {
                                fprintf(fp, "%s            if (%s_%s_%s_present(%s)) {\n", prefix, schema->namespace, table->name, ntable_field->name, namespace_linearized(namespace));
                                fprintf(fp, "%s                rc = emitter(context, \"%s\");\n", prefix, ",");
                                fprintf(fp, "%s                if (rc < 0) {\n", prefix);
                                fprintf(fp, "%s                    goto bail;\n", prefix);
                                fprintf(fp, "%s                }\n", prefix);
                                fprintf(fp, "%s                break;\n", prefix);
                                fprintf(fp, "%s            }\n", prefix);
                        }
                        fprintf(fp, "%s        } while (0);\n", prefix);
                        fprintf(fp, "%s    }\n", prefix);
                }
                fprintf(fp, "%s    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE) {\n", prefix);
                fprintf(fp, "%s        rc = emitter(context, \"\\n\");\n", prefix);
                fprintf(fp, "%s        if (rc < 0) {\n", prefix);
                fprintf(fp, "%s            goto bail;\n", prefix);
                fprintf(fp, "%s        }\n", prefix);
                fprintf(fp, "%s    }\n", prefix);
                fprintf(fp, "%s}\n", prefix);
                table_field_i += 1;
        }

        if (TAILQ_EMPTY(&element->entries)) {
                fprintf(fp, "    rc = emitter(context, \"}\");\n");
                fprintf(fp, "    if (rc < 0) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    if (flags & LINEARBUFFERS_JSONIFY_FLAG_PRETTY_ENDLINE) {\n");
                fprintf(fp, "        rc = emitter(context, \"\\n\");\n");
                fprintf(fp, "        if (rc < 0) {\n");
                fprintf(fp, "            goto bail;\n");
                fprintf(fp, "        }\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return 0;\n");
                fprintf(fp, "bail:\n");
                fprintf(fp, "    return -1;\n");
                fprintf(fp, "}\n");
        }

        if (TAILQ_EMPTY(&element->entries)) {
                fprintf(fp, "\n");
                fprintf(fp, "struct %s_jsonify_string_emitter_param {\n", namespace_linearized(namespace));
                fprintf(fp, "    char *beg;\n");
                fprintf(fp, "    int off;\n");
                fprintf(fp, "    int len;\n");
                fprintf(fp, "};\n");
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline int %s_jsonify_string_emitter (struct %s_jsonify_string_emitter_param *param, const char *fmt, ...)\n", namespace_linearized(namespace), namespace_linearized(namespace));
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    va_list ap;\n");
                fprintf(fp, "    va_start(ap, fmt);\n");
                fprintf(fp, "    rc = vsnprintf(param->beg + param->off, 0, fmt, ap);\n");
                fprintf(fp, "    va_end(ap);\n");
                fprintf(fp, "    if (rc < 0) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    if (rc >= param->len - param->off) {\n");
                fprintf(fp, "        char *tmp;\n");
                fprintf(fp, "        tmp = (char *) realloc(param->beg, param->off + rc + 1);\n");
                fprintf(fp, "        if (tmp == NULL) {\n");
                fprintf(fp, "            tmp = (char *) malloc(param->off + rc + 1);\n");
                fprintf(fp, "            if (tmp == NULL) {\n");
                fprintf(fp, "                goto bail;\n");
                fprintf(fp, "            }\n");
                fprintf(fp, "            if (param->beg != NULL) {\n");
                fprintf(fp, "                memcpy(tmp, param->beg, param->off);\n");
                fprintf(fp, "                free(param->beg);\n");
                fprintf(fp, "            }\n");
                fprintf(fp, "        }\n");
                fprintf(fp, "        param->beg = tmp;\n");
                fprintf(fp, "        param->len = param->off + rc + 1;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    va_start(ap, fmt);\n");
                fprintf(fp, "    rc = vsnprintf(param->beg + param->off, param->len - param->off, fmt, ap);\n");
                fprintf(fp, "    va_end(ap);\n");
                fprintf(fp, "    if (rc < 0) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    param->off += rc;\n");
                fprintf(fp, "    return 0;\n");
                fprintf(fp, "bail:\n");
                fprintf(fp, "    return -1;\n");
                fprintf(fp, "}\n");
                fprintf(fp, "\n");
                fprintf(fp, "__attribute__((unused)) static inline char * %s_jsonify_string (const struct %s *%s, unsigned int flags)\n", namespace_linearized(namespace), namespace_linearized(namespace), namespace_linearized(namespace));
                fprintf(fp, "{\n");
                fprintf(fp, "    int rc;\n");
                fprintf(fp, "    struct %s_jsonify_string_emitter_param param;\n", namespace_linearized(namespace));
                fprintf(fp, "    memset(&param, 0, sizeof(param));\n");
                fprintf(fp, "    rc = %s_jsonify(%s, flags, (int (*) (void *context, const char *format, ...)) %s_jsonify_string_emitter, &param);\n", namespace_linearized(namespace), namespace_linearized(namespace), namespace_linearized(namespace));
                fprintf(fp, "    if (rc != 0) {\n");
                fprintf(fp, "        goto bail;\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return param.beg;\n");
                fprintf(fp, "bail:\n");
                fprintf(fp, "    if (param.beg != NULL) {\n");
                fprintf(fp, "        free(param.beg);\n");
                fprintf(fp, "    }\n");
                fprintf(fp, "    return NULL;\n");
                fprintf(fp, "}\n");
        }

        free(prefix);
        return 0;
bail:   return -1;
}

int schema_generate_js_jsonify (struct schema *schema, FILE *fp, int decoder_use_memcpy)
{
        int rc;

        struct element *element;
        struct namespace *namespace;

        struct schema_table *table;

        element = NULL;
        namespace = NULL;

        if (schema == NULL) {
                linearbuffers_errorf("schema is invalid");
                goto bail;
        }
        if (fp == NULL) {
                linearbuffers_errorf("fp is invalid");
                goto bail;
        }

        rc = schema_generate_js_decoder(schema, fp, decoder_use_memcpy);
        if (rc != 0) {
                linearbuffers_errorf("can not generate decoder for schema");
                goto bail;
        }

        fprintf(fp, "\n");
        fprintf(fp, "#include <stdio.h>\n");
        fprintf(fp, "#include <stdlib.h>\n");
        fprintf(fp, "#include <stdarg.h>\n");
        fprintf(fp, "#include <inttypes.h>\n");

        fprintf(fp, "\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE    0x00000001\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE     0x00000002\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_PRETTY_COMMA)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_PRETTY_COMMA    0x00000004\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_PRETTY_ENDLINE)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_PRETTY_ENDLINE  0x00000008\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_PRETTY)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_PRETTY          (LINEARBUFFERS_JSONIFY_FLAG_PRETTY_SPACE | LINEARBUFFERS_JSONIFY_FLAG_PRETTY_LINE | LINEARBUFFERS_JSONIFY_FLAG_PRETTY_COMMA | LINEARBUFFERS_JSONIFY_FLAG_PRETTY_ENDLINE)\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_DEFAULT)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_DEFAULT         LINEARBUFFERS_JSONIFY_FLAG_PRETTY\n");
        fprintf(fp, "#endif\n");
        fprintf(fp, "#if !defined(LINEARBUFFERS_JSONIFY_FLAG_NONE)\n");
        fprintf(fp, "#define LINEARBUFFERS_JSONIFY_FLAG_NONE            0\n");
        fprintf(fp, "#endif\n");

        fprintf(fp, "\n");
        fprintf(fp, "#if !defined(%s_JSONIFY_STRING_EMITTER)\n", schema->NAMESPACE);
        fprintf(fp, "#define %s_JSONIFY_STRING_EMITTER\n", schema->NAMESPACE);
        fprintf(fp, "static inline int %s_jsonify_string_emitter (const char *str, int (*emitter) (void *context, const char *format, ...), void *context)\n", schema->namespace);
        fprintf(fp, "{\n");
        fprintf(fp, "    int rc;\n");
        fprintf(fp, "    const char *ptr;\n");
        fprintf(fp, "    if (str == NULL) {\n");
        fprintf(fp, "        return emitter(context, \"\\\"\\\"\");\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    for (ptr = str; *ptr; ptr++) {\n");
        fprintf(fp, "        if (((*ptr > 0) && (*ptr < 32)) ||\n");
        fprintf(fp, "            (*ptr == '\\\"') ||\n");
        fprintf(fp, "            (*ptr == '\\\\')) {\n");
        fprintf(fp, "            break;\n");
        fprintf(fp, "        }\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    if (*ptr == '\\0') {\n");
        fprintf(fp, "        return emitter(context, \"\\\"%%s\\\"\", str);\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    rc = emitter(context, \"\\\"\");\n");
        fprintf(fp, "    if (rc < 0) {\n");
        fprintf(fp, "        return rc;\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    for (ptr = str; *ptr; ptr++) {\n");
        fprintf(fp, "        if (*ptr == '\\\"') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\\\\"\");\n");
        fprintf(fp, "        } else if (*ptr == '\\\\') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\\\\\\");\n");
        fprintf(fp, "        } else if (*ptr == '\\b') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\b\");\n");
        fprintf(fp, "        } else if (*ptr == '\\f') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\f\");\n");
        fprintf(fp, "        } else if (*ptr == '\\n') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\n\");\n");
        fprintf(fp, "        } else if (*ptr == '\\r') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\r\");\n");
        fprintf(fp, "        } else if (*ptr == '\\t') {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\t\");\n");
        fprintf(fp, "        } else if (*ptr < 32) {\n");
        fprintf(fp, "            rc = emitter(context, \"\\\\u%%04x\", *ptr);\n");
        fprintf(fp, "        } else {\n");
        fprintf(fp, "            rc = emitter(context, \"%%c\", *ptr);\n");
        fprintf(fp, "        }\n");
        fprintf(fp, "        if (rc < 0) {\n");
        fprintf(fp, "           return rc;\n");
        fprintf(fp, "        }\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    rc = emitter(context, \"\\\"\");\n");
        fprintf(fp, "    if (rc < 0) {\n");
        fprintf(fp, "       return rc;\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    return 0;\n");
        fprintf(fp, "}\n");
        fprintf(fp, "#endif\n");

        TAILQ_FOREACH(table, &schema->tables, list) {
                fprintf(fp, "\n");
                fprintf(fp, "#if !defined(%s_%s_JSONIFY_API)\n", schema->NAMESPACE, table->name);
                fprintf(fp, "#define %s_%s_JSONIFY_API\n", schema->NAMESPACE, table->name);
                fprintf(fp, "\n");

                namespace = namespace_create();
                if (namespace == NULL) {
                        linearbuffers_errorf("can not create namespace");
                        goto bail;
                }
                rc = namespace_push(namespace, "%s_%s", schema->namespace, table->name);
                if (rc != 0) {
                        linearbuffers_errorf("can not build namespace");
                        goto bail;
                }

                element = element_create();
                if (element == NULL) {
                        linearbuffers_errorf("can not create element");
                        goto bail;
                }

                rc = schema_generate_jsonify_table(schema, table, table, namespace, element, fp);
                if (rc != 0) {
                        linearbuffers_errorf("can not generate jsonify for table: %s", table->name);
                        goto bail;
                }

                namespace_destroy(namespace);
                element_destroy(element);

                fprintf(fp, "\n");
                fprintf(fp, "#endif\n");
        }

        return 0;
bail:   if (namespace != NULL) {
                namespace_destroy(namespace);
        }
        if (element != NULL) {
                element_destroy(element);
        }
        return -1;
}
