
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "debug.h"
#include "schema.h"
#include "schema-private.h"

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

	fprintf(fp, "typedef %s_t %s_%s_enum_t;\n", anum->type, schema->namespace, anum->name);

	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		if (anum_field->value != NULL) {
			fprintf(fp, "#define %s_%s_%s ((%s_%s_enum_t) %s_C(%s))\n", schema->namespace, anum->name, anum_field->name, schema->namespace, anum->name, anum->TYPE, anum_field->value);
		} else {
			fprintf(fp, "#define %s_%s_%s\n", schema->namespace, anum->name, anum_field->name);
		}
	}

	fprintf(fp, "\n");
	fprintf(fp, "static inline const char * %s_%s_string (%s_%s_enum_t value)\n", schema->namespace, anum->name, schema->namespace, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s_%s_%s: return \"%s\";\n", schema->namespace, anum->name, anum_field->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return \"%s\";\n", "");
	fprintf(fp, "}\n");
	fprintf(fp, "static inline int %s_%s_is_valid (%s_%s_enum_t value)\n", schema->namespace, anum->name, schema->namespace, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s_%s_%s: return 1;\n", schema->namespace, anum->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return 0;\n");
	fprintf(fp, "}\n");

	return 0;
bail:	return -1;
}

static int schema_generate_vector (struct schema *schema, const char *type, int decoder_use_memcpy, FILE *fp)
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
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t count;\n");
			fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder), sizeof(count));\n");
		} else {
			fprintf(fp, "    return *(uint64_t *) (((const void *) decoder));\n");
		}
		fprintf(fp, "}\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s_t);\n", schema->namespace, type, type);
		fprintf(fp, "}\n");
		fprintf(fp, "static inline const %s_t * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const void *) decoder) + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
		fprintf(fp, "}\n");
		fprintf(fp, "static inline %s_t %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const %s_t *) (((const void *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", type, sizeof(uint64_t));
		fprintf(fp, "}\n");
	} else if (schema_type_is_float(type)) {
		fprintf(fp, "\n");
		fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
		fprintf(fp, "\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t count;\n");
			fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder), sizeof(count));\n");
		} else {
			fprintf(fp, "    return *(uint64_t *) (((const void *) decoder));\n");
		}
		fprintf(fp, "}\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s);\n", schema->namespace, type, type);
		fprintf(fp, "}\n");
		fprintf(fp, "static inline const %s * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const void *) decoder) + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
		fprintf(fp, "}\n");
		fprintf(fp, "static inline %s %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const %s *) (((const void *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", type, sizeof(uint64_t));
		fprintf(fp, "}\n");
	} else if (schema_type_is_enum(schema, type)) {
		fprintf(fp, "\n");
		fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
		fprintf(fp, "\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t count;\n");
			fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder), sizeof(count));\n");
		} else {
			fprintf(fp, "    return *(uint64_t *) (((const void *) decoder));\n");
		}
		fprintf(fp, "}\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_length (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return %s_%s_vector_get_count(decoder) * sizeof(%s_%s_enum_t);\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "}\n");
		fprintf(fp, "static inline const %s_%s_enum_t * %s_%s_vector_get_values (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const void *) decoder) + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
		fprintf(fp, "}\n");
		fprintf(fp, "static inline %s_%s_enum_t %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    return ((const %s_%s_enum_t *) (((const void *) decoder) + UINT64_C(%" PRIu64 ")))[at];\n", schema->namespace, type, sizeof(uint64_t));
		fprintf(fp, "}\n");
	} else if (schema_type_is_string(type)) {
		fprintf(fp, "\n");
		fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
		fprintf(fp, "\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t count;\n");
			fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder), sizeof(count));\n");
		} else {
			fprintf(fp, "    return *(uint64_t *) (((const void *) decoder));\n");
		}
		fprintf(fp, "}\n");
		fprintf(fp, "static inline const char * %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    uint64_t offset;\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t toffset;\n");
		}
		fprintf(fp, "    offset = 0;\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", sizeof(uint64_t));
			fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + (sizeof(uint64_t) * at), sizeof(offset));\n");
		} else {
			fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", sizeof(uint64_t));
			fprintf(fp, "    offset += ((uint64_t *) (((const void *) decoder) + offset))[at];\n");
		}
			fprintf(fp, "    return (const char *) (((const void *) decoder) + offset);\n");
		fprintf(fp, "}\n");
	} else if (schema_type_is_table(schema, type)) {
		fprintf(fp, "\n");
		fprintf(fp, "struct %s_%s_vector;\n", schema->namespace, type);
		fprintf(fp, "\n");
		fprintf(fp, "static inline uint64_t %s_%s_vector_get_count (const struct %s_%s_vector *decoder)\n", schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t count;\n");
			fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder), sizeof(count));\n");
		} else {
			fprintf(fp, "    return *(uint64_t *) (((const void *) decoder));\n");
		}
		fprintf(fp, "}\n");
		fprintf(fp, "static inline const struct %s_%s * %s_%s_vector_get_at (const struct %s_%s_vector *decoder, uint64_t at)\n", schema->namespace, type, schema->namespace, type, schema->namespace, type);
		fprintf(fp, "{\n");
		fprintf(fp, "    uint64_t offset;\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    uint64_t toffset;\n");
		}
		fprintf(fp, "    offset = 0;\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", sizeof(uint64_t));
			fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + (sizeof(uint64_t) * at), sizeof(offset));\n");
		} else {
			fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", sizeof(uint64_t));
			fprintf(fp, "    offset += ((uint64_t *) (((const void *) decoder) + offset))[at];\n");
		}
			fprintf(fp, "    return (const struct %s_%s *) (((const void *) decoder) + offset);\n", schema->namespace, type);
		fprintf(fp, "}\n");
	}
	return 0;
bail:	return -1;
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
bail:	if (entry != NULL) {
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
bail:	if (entry != NULL) {
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
bail:	return -1;
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
bail:	if (element != NULL) {
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
bail:	if (entry != NULL) {
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
bail:	if (entry != NULL) {
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
bail:	if (entry != NULL) {
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
bail:	return NULL;
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
bail:	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return NULL;
}

static int schema_generate_encoder_table (struct schema *schema, struct schema_table *table, int root, FILE *fp)
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
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_scalar(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_float(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_enum(schema, table_field->type)) {
			table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
		} else if (schema_type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else {
			linearbuffers_errorf("type is invalid: %s", table_field->type);
			goto bail;
		}

	}

	fprintf(fp, "static inline int %s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_start(encoder, %s, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "));\n", (root) ? "1" : "0", table->nfields, table_field_s);
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");

	table_field_i = 0;
	table_field_s = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set_values (struct linearbuffers_encoder *encoder, const %s_t *value_%s, uint64_t value_%s_count)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", table_field->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_%s(encoder);\n", table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", table_field->type);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return NULL;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return (const struct %s_%s_vector *) offset;\n", schema->namespace, table_field->type);
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_cancel_%s(encoder);\n", table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set_values (struct linearbuffers_encoder *encoder, const %s *value_%s, uint64_t value_%s_count)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", table_field->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_%s(encoder);\n", table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s value_%s)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", table_field->type);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return NULL;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return (const struct %s_%s_vector *) offset;\n", schema->namespace, table_field->type);
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_cancel_%s(encoder);\n", table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set_values (struct linearbuffers_encoder *encoder, %s_%s_enum_t *value_%s, uint64_t value_%s_count)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", schema_type_get_enum(schema, table_field->type)->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_%s(encoder);\n", schema_type_get_enum(schema, table_field->type)->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, %s_%s_enum_t value_%s)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", schema_type_get_enum(schema, table_field->type)->type, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "__attribute__((warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_%s(encoder, &offset);\n", schema_type_get_enum(schema, table_field->type)->type);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return NULL;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return (const struct %s_%s_vector *) offset;\n", schema->namespace, table_field->type);
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_cancel_%s(encoder);\n", schema_type_get_enum(schema, table_field->type)->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set_values (struct linearbuffers_encoder *encoder, const char **value_%s, uint64_t value_%s_count)\n", schema->namespace, table->name, table_field->name, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t i;\n");
				fprintf(fp, "    uint64_t value;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_string(encoder);\n");
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    for (i = 0; i < value_%s_count; i++) {\n", table_field->name);
				fprintf(fp, "        rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s[i]);\n", table_field->type, table_field->name);
				fprintf(fp, "        if (rc != 0) {\n");
				fprintf(fp, "            return rc;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_string(encoder, &value);\n");
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_string_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_string(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, const char *value_%s)\n", schema->namespace, table->name, table_field->name, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "__attribute__((warn_unused_result)) static inline const struct %s_string_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_string(encoder, &offset);\n");
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return NULL;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return (const struct %s_string_vector *) offset;\n", schema->namespace);
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_cancel_string(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s_vector *value)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_table(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value);\n", table_field_i, table_field_s);
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return rc;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_start (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start_table(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_push (struct linearbuffers_encoder *encoder, const struct %s_%s *value_%s)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_table(encoder, (uint64_t) value_%s);\n", table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "__attribute__((warn_unused_result)) static inline const struct %s_%s_vector * %s_%s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table_field->type, schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end_table(encoder, &offset);\n");
				fprintf(fp, "    if (rc != 0) {\n");
				fprintf(fp, "        return NULL;\n");
				fprintf(fp, "    }\n");
				fprintf(fp, "    return (const struct %s_%s_vector *) offset;\n", schema->namespace, table_field->type);
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_cancel_table(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		} else {
			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s value_%s)\n", schema->namespace, table->name, table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const char *value_%s)\n", schema->namespace, table->name, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s_%s_%s_nset (struct linearbuffers_encoder *encoder, const char *value_%s, uint64_t n)\n", schema->namespace, table->name, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_nset_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, n);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, %s_%s_enum_t value_%s)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", schema_type_get_enum(schema, table_field->type)->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline int %s_%s_%s_set (struct linearbuffers_encoder *encoder, const struct %s_%s *value_%s)\n", schema->namespace, table->name, table_field->name, schema->namespace, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_table(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), (uint64_t) value_%s);\n", table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_scalar(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_float(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_enum(schema, table_field->type)) {
			table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
		} else if (schema_type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		}
	}

	fprintf(fp, "__attribute__((warn_unused_result)) static inline const struct %s_%s * %s_%s_end (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name, schema->namespace, table->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    uint64_t offset;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_end(encoder, &offset);\n");
	fprintf(fp, "    if (rc != 0) {\n");
	fprintf(fp, "        return NULL;\n");
	fprintf(fp, "    }\n");
	fprintf(fp, "    return (const struct %s_%s *) offset;\n", schema->namespace, table->name);
	fprintf(fp, "}\n");

	fprintf(fp, "static inline int %s_%s_cancel (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_cancel(encoder);\n");
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");

	fprintf(fp, "static inline int %s_%s_finish (struct linearbuffers_encoder *encoder)\n", schema->namespace, table->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_end(encoder, NULL);\n");
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");

	return 0;
bail:	return -1;
}

int schema_generate_c_encoder (struct schema *schema, FILE *fp, int decoder_use_memcpy)
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
	if (decoder_use_memcpy) {
		fprintf(fp, "#include <string.h>\n");
	}
	fprintf(fp, "#include <linearbuffers/encoder.h>\n");

	TAILQ_FOREACH(anum, &schema->enums, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, anum->name);
		fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, anum->name);
		fprintf(fp, "\n");

		schema_generate_enum(schema, anum, fp);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	fprintf(fp, "\n");
	fprintf(fp, "#if !defined(%s_VECTOR_API)\n", schema->NAMESPACE);
	fprintf(fp, "#define %s_VECTOR_API\n", schema->NAMESPACE);

	schema_generate_vector(schema, "int8", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int16", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int32", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int64", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint8", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint16", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint32", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint64", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "float", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "double", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "string", decoder_use_memcpy, fp);

	fprintf(fp, "\n");
	fprintf(fp, "#endif\n");

	TAILQ_FOREACH(anum, &schema->enums, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_VECTOR_API)\n", schema->NAMESPACE, anum->name);
		fprintf(fp, "#define %s_%s_VECTOR_API\n", schema->NAMESPACE, anum->name);

		schema_generate_vector(schema, anum->name, decoder_use_memcpy, fp);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_VECTOR_API)\n", schema->NAMESPACE, table->name);
		fprintf(fp, "#define %s_%s_VECTOR_API\n", schema->NAMESPACE, table->name);

		schema_generate_vector(schema, table->name, decoder_use_memcpy, fp);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENCODER_API)\n", schema->NAMESPACE, table->name);
		fprintf(fp, "#define %s_%s_ENCODER_API\n", schema->NAMESPACE, table->name);

		fprintf(fp, "\n");
		fprintf(fp, "struct %s_%s;\n", schema->namespace, table->name);
		fprintf(fp, "\n");

		rc = schema_generate_encoder_table(schema, table, 0, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate decoder for table: %s", table->name);
			goto bail;
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	return 0;
bail:	return -1;
}

static int schema_generate_decoder_table (struct schema *schema, struct schema_table *head, struct schema_table *table, struct namespace *namespace, int decoder_use_memcpy, FILE *fp)
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
	fprintf(fp, "struct %s_%s;\n", schema->namespace, head->name);
	fprintf(fp, "\n");

	fprintf(fp, "static inline const struct %s_%s * %s_%s_decode (const void *buffer, uint64_t length)\n", schema->namespace, head->name, schema->namespace, head->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    (void) length;\n");
	fprintf(fp, "    return buffer;\n");
	fprintf(fp, "}\n");

	table_field_i = 0;
	table_field_s = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		fprintf(fp, "static inline int %s%s_present (const struct %s_%s *decoder)", namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
		fprintf(fp, "{\n");
		fprintf(fp, "    uint8_t present;\n");
		if (decoder_use_memcpy) {
			fprintf(fp, "    memcpy(&present, ((const void *) decoder) + UINT64_C(%" PRIu64 "), sizeof(present));\n", sizeof(uint8_t) * (table_field_i / 8));
		} else {
			fprintf(fp, "    present = *(uint8_t *) (((const void *) decoder) + UINT64_C(%" PRIu64 "));\n", sizeof(uint8_t) * (table_field_i / 8));
		}
		fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (table_field_i % 8)));
		fprintf(fp, "        return 0;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    return 1;\n");
		fprintf(fp, "}\n");

		if (table_field->vector) {
			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s_vector * %s%s_get (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s_vector * %s%s_get (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s_vector * %s%s_get (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s_vector * %s%s_get (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s_vector * %s%s_get (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			}
			fprintf(fp, "{\n");
			fprintf(fp, "    uint64_t offset;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    uint64_t toffset;\n");
			}
			fprintf(fp, "    offset = 0;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
			} else {
				fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
			}
			fprintf(fp, "    return ((const void *) decoder) + offset;\n");
			fprintf(fp, "}\n");

			fprintf(fp, "static inline uint64_t %s%s_get_count (const struct %s_%s *decoder)", namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			fprintf(fp, "{\n");
			fprintf(fp, "    uint64_t offset;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    uint64_t toffset;\n");
			}
			fprintf(fp, "    offset = 0;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				fprintf(fp, "    return *(uint64_t *) memcpy(&count, ((const void *) decoder) + offset, sizeof(count));\n");
			} else {
				fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				fprintf(fp, "    return *(uint64_t *) (((const void *) decoder) + offset);\n");
			}
			fprintf(fp, "}\n");

			if (schema_type_is_scalar(table_field->type) ||
			    schema_type_is_float(table_field->type) ||
			    schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline uint64_t %s%s_get_length (const struct %s_%s *decoder)", namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
				fprintf(fp, "{\n");
				if (schema_type_is_scalar(table_field->type)) {
					fprintf(fp, "    return %s%s_get_count(decoder) * sizeof(%s_t);\n", namespace_linearized(namespace), table_field->name, table_field->type);
				} else if (schema_type_is_float(table_field->type)) {
					fprintf(fp, "    return %s%s_get_count(decoder) * sizeof(%s);\n", namespace_linearized(namespace), table_field->name, table_field->type);
				} else {
					fprintf(fp, "    return %s%s_get_count(decoder) * sizeof(%s_%s_enum_t);\n", namespace_linearized(namespace), table_field->name, schema->namespace, table_field->type);
				}
				fprintf(fp, "}\n");

				if (schema_type_is_scalar(table_field->type)) {
					fprintf(fp, "static inline const %s_t * %s%s_get_values (const struct %s_%s *decoder)", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
				} else if (schema_type_is_float(table_field->type)) {
					fprintf(fp, "static inline const %s * %s%s_get_values (const struct %s_%s *decoder)", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
				} else if (schema_type_is_enum(schema, table_field->type)) {
					fprintf(fp, "static inline const %s_%s_enum_t * %s%s_get_values (const struct %s_%s *decoder)", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
				}
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				if (decoder_use_memcpy) {
					fprintf(fp, "    uint64_t toffset;\n");
				}
				fprintf(fp, "    offset = 0;\n");
				if (decoder_use_memcpy) {
					fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return ((const void *) decoder) + offset + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
				fprintf(fp, "}\n");
			}

			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline %s_t %s%s_get_at (const struct %s_%s *decoder", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "static inline %s %s%s_get_at (const struct %s_%s *decoder", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline %s_%s_enum_t %s%s_get_at (const struct %s_%s *decoder", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "static inline const char * %s%s_get_at (const struct %s_%s *decoder", namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s * %s%s_get_at (const struct %s_%s *decoder", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			}
			fprintf(fp, ", uint64_t at)\n");
			fprintf(fp, "{\n");
			fprintf(fp, "    uint64_t offset;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    uint64_t toffset;\n");
			}
			fprintf(fp, "    offset = 0;\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
			} else {
				fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
			}
			if (schema_type_is_string(table_field->type) ||
			    schema_type_is_table(schema, table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", sizeof(uint64_t));
					fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + (sizeof(uint64_t) * at), sizeof(offset));\n");
				} else {
					fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", sizeof(uint64_t));
					fprintf(fp, "    offset += ((uint64_t *) (((const void *) decoder) + offset))[at];\n");
				}
			}
			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "    return ((%s_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 ")))[at];\n", table_field->type, sizeof(uint64_t));
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "    return ((%s *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 ")))[at];\n", table_field->type, sizeof(uint64_t));
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "    return ((%s_%s_enum_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 ")))[at];\n", schema->namespace, table_field->type, sizeof(uint64_t));
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "    return (const char *) (((const void *) decoder) + offset);\n");
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "    return ((void *) decoder) + offset;\n");
			}
			fprintf(fp, "}\n");
		} else {
			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline %s_t %s%s_get (const struct %s_%s *decoder", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "static inline %s %s%s_get (const struct %s_%s *decoder", table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "static inline const char * %s%s_get (const struct %s_%s *decoder", namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline %s_%s_enum_t %s%s_get (const struct %s_%s *decoder", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			} else 	if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline const struct %s_%s * %s%s_get (const struct %s_%s *decoder", schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, schema->namespace, head->name);
			}
			fprintf(fp, ")\n");
			fprintf(fp, "{\n");
			if (schema_type_is_scalar(table_field->type)) {
			} else if (schema_type_is_float(table_field->type)) {
			} else if (schema_type_is_enum(schema, table_field->type)) {
			} else {
				fprintf(fp, "    uint64_t offset;\n");
				if (decoder_use_memcpy) {
					fprintf(fp, "    uint64_t toffset;;\n");
				}
			}
			if (schema_type_is_scalar(table_field->type)) {
			} else if (schema_type_is_float(table_field->type)) {
			} else if (schema_type_is_enum(schema, table_field->type)) {
			} else {
				fprintf(fp, "    offset = 0;\n");
			}
			if (schema_type_is_scalar(table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    %s_t value;\n", table_field->type);
					fprintf(fp, "    return *(%s_t *) memcpy(&value, ((const void *) decoder) + UINT64_C(%" PRIu64 "), sizeof(value));\n", table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    return *(%s_t *) (((const void *) decoder) + UINT64_C(%" PRIu64 "));\n", table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				}
			} else if (schema_type_is_float(table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    %s value;\n", table_field->type);
					fprintf(fp, "    return *(%s *) memcpy(&value, ((const void *) decoder) + UINT64_C(%" PRIu64 "), sizeof(value));\n", table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    return *(%s *) (((const void *) decoder) + UINT64_C(%" PRIu64 "));\n", table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				}
			} else if (schema_type_is_enum(schema, table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    %s_%s_enum_t value;\n", schema->namespace, table_field->type);
					fprintf(fp, "    return *(%s_%s_enum_t *) memcpy(&value, ((const void *) decoder) + UINT64_C(%" PRIu64 "), sizeof(value));\n", schema->namespace, table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    return *(%s_%s_enum_t *) (((const void *) decoder) + UINT64_C(%" PRIu64 "));\n", schema->namespace, table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				}
			} else if (schema_type_is_string(table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return (const char *) (((const void *) decoder) + offset);\n");
			} else if (schema_type_is_table(schema, table_field->type)) {
				if (decoder_use_memcpy) {
					fprintf(fp, "    offset += *(uint64_t *) memcpy(&toffset, ((const void *) decoder) + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset += *(uint64_t *) (((const void *) decoder) + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return ((const void *) decoder) + offset;\n");
			}
			fprintf(fp, "}\n");
		}
		table_field_i += 1;
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_scalar(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_float(table_field->type)) {
			table_field_s += schema_inttype_size(table_field->type);
		} else if (schema_type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (schema_type_is_enum(schema, table_field->type)) {
			table_field_s += schema_inttype_size(schema_type_get_enum(schema, table_field->type)->type);
		} else if (schema_type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		}
	}

	return 0;
bail:	return -1;
}

int schema_generate_c_decoder (struct schema *schema, FILE *fp, int decoder_use_memcpy)
{
	int rc;

	struct namespace *namespace;

	struct schema_enum *anum;
	struct schema_table *table;

	namespace = NULL;

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

	fprintf(fp, "\n");
	fprintf(fp, "#if !defined(%s_VECTOR_API)\n", schema->NAMESPACE);
	fprintf(fp, "#define %s_VECTOR_API\n", schema->NAMESPACE);

	schema_generate_vector(schema, "int8", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int16", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int32", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "int64", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint8", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint16", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint32", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "uint64", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "float", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "double", decoder_use_memcpy, fp);
	schema_generate_vector(schema, "string", decoder_use_memcpy, fp);

	fprintf(fp, "\n");
	fprintf(fp, "#endif\n");

	TAILQ_FOREACH(anum, &schema->enums, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_VECTOR_API)\n", schema->NAMESPACE, anum->name);
		fprintf(fp, "#define %s_%s_VECTOR_API\n", schema->NAMESPACE, anum->name);

		schema_generate_vector(schema, anum->name, decoder_use_memcpy, fp);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_VECTOR_API)\n", schema->NAMESPACE, table->name);
		fprintf(fp, "#define %s_%s_VECTOR_API\n", schema->NAMESPACE, table->name);

		schema_generate_vector(schema, table->name, decoder_use_memcpy, fp);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_DECODER_API)\n", schema->NAMESPACE, table->name);
		fprintf(fp, "#define %s_%s_DECODER_API\n", schema->NAMESPACE, table->name);

		namespace = namespace_create();
		if (namespace == NULL) {
			linearbuffers_errorf("can not create namespace");
			goto bail;
		}
		rc = namespace_push(namespace, "%s_%s_", schema->namespace, table->name);
		if (rc != 0) {
			linearbuffers_errorf("can not build namespace");
			goto bail;
		}

		rc = schema_generate_decoder_table(schema, table, table, namespace, decoder_use_memcpy, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate decoder for table: %s", table->name);
			goto bail;
		}

		namespace_destroy(namespace);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	return 0;
bail:	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return -1;
}

static int schema_generate_jsonify_table (struct schema *schema, struct schema_table *head, struct schema_table *table, struct namespace *namespace, struct element *element, FILE *fp)
{
	int rc;
	char *prefix;
	uint64_t prefix_count;

	uint64_t table_field_i;
	struct schema_table_field *table_field;

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

	if (TAILQ_EMPTY(&element->entries)) {
		fprintf(fp, "static inline int %s_jsonify (const void *buffer, uint64_t length, int (*emitter) (void *context, const char *format, ...), void *context)\n", namespace_linearized(namespace));
		fprintf(fp, "{\n");
		fprintf(fp, "    int rc;\n");
		fprintf(fp, "    const struct %s *%s;\n", namespace_linearized(namespace), namespace_linearized(namespace));
		fprintf(fp, "    %s = %s_decode(buffer, length);\n", namespace_linearized(namespace), namespace_linearized(namespace));
		fprintf(fp, "    if (%s == NULL) {\n", namespace_linearized(namespace));
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    if (emitter == NULL) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    rc = emitter(context, \"{\\n\");\n");
		fprintf(fp, "    if (rc < 0) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
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

	table_field_i = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		fprintf(fp, "%sif (%s_%s_%s_present(%s)) {\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
		if (table_field->vector) {
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
			fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": [\\n\");\n", prefix, prefix, table_field->name);
			fprintf(fp, "%s    if (rc < 0) {\n", prefix);
			fprintf(fp, "%s        goto bail;\n", prefix);
			fprintf(fp, "%s    }\n", prefix);
			fprintf(fp, "%s    for (at_%" PRIu64 " = 0; at_%" PRIu64 " < count; at_%" PRIu64 "++) {\n", prefix, element->nentries, element->nentries, element->nentries);

			if (schema_type_is_scalar(table_field->type)) {
				fprintf(fp, "%s        %s_t value;\n", prefix, table_field->type);
				fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
				if (strncmp(table_field->type, "int", 3) == 0) {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIi64 \"%%s\\n\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				} else {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIu64 \"%%s\\n\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				}
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
				fprintf(fp, "%s        }\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
				fprintf(fp, "%s    rc = emitter(context, \"%s]%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			} else if (schema_type_is_float(table_field->type)) {
				fprintf(fp, "%s        %s value;\n", prefix, table_field->type);
				fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
				if (strncmp(table_field->type, "int", 3) == 0) {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIi64 \"%%s\\n\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				} else {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIu64 \"%%s\\n\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				}
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
				fprintf(fp, "%s        }\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
				fprintf(fp, "%s    rc = emitter(context, \"%s]%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			} else if (schema_type_is_enum(schema, table_field->type)) {
				fprintf(fp, "%s        %s_%s_enum_t value;\n", prefix, schema->namespace, table_field->type);
				fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
				if (strncmp(schema_type_get_enum(schema, table_field->type)->type, "int", 3) == 0) {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIi64 \"%%s\\n\", (int64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				} else {
					fprintf(fp, "%s        rc = emitter(context, \"%s    %%\" PRIu64 \"%%s\\n\", (uint64_t) value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				}
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
				fprintf(fp, "%s        }\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
				fprintf(fp, "%s    rc = emitter(context, \"%s]%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			} else if (schema_type_is_string(table_field->type)) {
				fprintf(fp, "%s        const char *value;\n", prefix);
				fprintf(fp, "%s        value = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
				fprintf(fp, "%s        rc = emitter(context, \"%s    \\\"%%s\\\"%%s\\n\", value, ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
				fprintf(fp, "%s        }\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
				fprintf(fp, "%s    rc = emitter(context, \"%s]%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			} else if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "%s        const struct %s_%s *%s_%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, table_field->type);
				fprintf(fp, "%s        %s_%s_%s = %s_%s_vector_get_at(%s_%s, at_%" PRIu64 ");\n", prefix, namespace_linearized(namespace), table_field->name, table_field->type, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name, element->nentries);
				fprintf(fp, "%s        rc = emitter(context, \"%s    {\\n\");\n", prefix, prefix);
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
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
				fprintf(fp, "%s        rc = emitter(context, \"%s    }%%s\\n\", ((at_%" PRIu64 " + 1) == count) ? \"\" : \",\");\n", prefix, prefix, element->nentries);
				fprintf(fp, "%s        if (rc < 0) {\n", prefix);
				fprintf(fp, "%s            goto bail;\n", prefix);
				fprintf(fp, "%s        }\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
				fprintf(fp, "%s    rc = emitter(context, \"%s]%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		} else {
			if (schema_type_is_scalar(table_field->type) ||
			    schema_type_is_float(table_field->type) ||
			    schema_type_is_string(table_field->type) ||
			    schema_type_is_enum(schema, table_field->type)) {
				if (schema_type_is_scalar(table_field->type)) {
					fprintf(fp, "%s    %s_t value;\n", prefix, table_field->type);
				} else if (schema_type_is_float(table_field->type)) {
					fprintf(fp, "%s    %s value;\n", prefix, table_field->type);
				} else if (schema_type_is_enum(schema, table_field->type)) {
					fprintf(fp, "%s    %s_%s_enum_t value;\n", prefix, schema->namespace, table_field->type);
				} else if (schema_type_is_string(table_field->type)) {
					fprintf(fp, "%s    const char *value;\n", prefix);
				}
				fprintf(fp, "%s    value = %s_%s_%s_get(%s);\n", prefix, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
				if (schema_type_is_scalar(table_field->type)) {
					if (strncmp(table_field->type, "int", 3) == 0) {
						fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": %%\" PRIi64 \"%s\\n\", (int64_t) value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
					} else {
						fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": %%\" PRIu64 \"%s\\n\", (uint64_t) value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
					}
				} else if (schema_type_is_float(table_field->type)) {
					fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": %%f%s\\n\", value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				} else if (schema_type_is_enum(schema, table_field->type)) {
					if (strncmp(schema_type_get_enum(schema, table_field->type)->type, "int", 3) == 0) {
						fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": %%\" PRIi64 \"%s\\n\", (int64_t) value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
					} else {
						fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": %%\" PRIu64 \"%s\\n\", (uint64_t) value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
					}
				} else if (schema_type_is_string(table_field->type)) {
					fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": \\\"%%s\\\"%s\\n\", value);\n", prefix, prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				}
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			}

			if (schema_type_is_table(schema, table_field->type)) {
				fprintf(fp, "%s    const struct %s_%s *%s_%s;\n", prefix, schema->namespace, table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "%s    (void) %s_%s;\n", prefix, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "%s    %s_%s = %s_%s_%s_get(%s);\n", prefix, namespace_linearized(namespace), table_field->name, schema->namespace, table->name, table_field->name, namespace_linearized(namespace));
				fprintf(fp, "%s    rc = emitter(context, \"%s\\\"%s\\\": {\\n\");\n", prefix, prefix, table_field->name);
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
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
				fprintf(fp, "%s    rc = emitter(context, \"%s}%s\\n\");\n", prefix, prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "%s    if (rc < 0) {\n", prefix);
				fprintf(fp, "%s        goto bail;\n", prefix);
				fprintf(fp, "%s    }\n", prefix);
			}
		}
		fprintf(fp, "%s}\n", prefix);
		table_field_i += 1;
	}

	if (TAILQ_EMPTY(&element->entries)) {
		fprintf(fp, "    rc = emitter(context, \"}\\n\");\n");
		fprintf(fp, "    if (rc != 0) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    return rc;\n");
		fprintf(fp, "bail:\n");
		fprintf(fp, "    return -1;\n");
		fprintf(fp, "}\n");
	}

	free(prefix);
	return 0;
bail:	return -1;
}

int schema_generate_c_jsonify (struct schema *schema, FILE *fp, int decoder_use_memcpy)
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

	rc = schema_generate_c_decoder(schema, fp, decoder_use_memcpy);
	if (rc != 0) {
		linearbuffers_errorf("can not generate decoder for schema");
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <inttypes.h>\n");

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
bail:	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	if (element != NULL) {
		element_destroy(element);
	}
	return -1;
}
