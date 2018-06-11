
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "queue.h"
#include "encoder.h"

enum entry_scalar_type {
	entry_scalar_type_int8,
	entry_scalar_type_int16,
	entry_scalar_type_int32,
	entry_scalar_type_int64,
	entry_scalar_type_uint8,
	entry_scalar_type_uint16,
	entry_scalar_type_uint32,
	entry_scalar_type_uint64,
};

enum entry_vector_type {
	entry_vector_type_table,
	entry_vector_type_int8,
	entry_vector_type_int16,
	entry_vector_type_int32,
	entry_vector_type_int64,
	entry_vector_type_uint8,
	entry_vector_type_uint16,
	entry_vector_type_uint32,
	entry_vector_type_uint64,
};

enum entry_type {
	entry_type_unknown,
	entry_type_scalar,
	entry_type_enum,
	entry_type_table,
	entry_type_vector,
};

struct entry_scalar {
	enum entry_scalar_type type;
	union {
		int8_t int8;
		int16_t int16;
		int32_t int32;
		int64_t int64;
		uint8_t uint8;
		uint16_t uint16;
		uint32_t uint32;
		uint64_t uint64;
	} u;
};

struct entry_table {
	uint64_t nelements;
	uint64_t *offsets;
};

struct entry_enum {
};

struct entry_vector {
	enum entry_vector_type type;
	union {
		struct {
			int8_t *value;
			uint64_t length;
		} int8;
		struct {
			int16_t *value;
			uint64_t length;
		} int16;
		struct {
			int32_t *value;
			uint64_t length;
		} int32;
		struct {
			int64_t *value;
			uint64_t length;
		} int64;
		struct {
			uint8_t *value;
			uint64_t length;
		} uint8;
		struct {
			uint16_t *value;
			uint64_t length;
		} uint16;
		struct {
			uint32_t *value;
			uint64_t length;
		} uint32;
		struct {
			uint64_t *value;
			uint64_t length;
		} uint64;
		struct {
			uint64_t length;
		} table;
	} u;
};

TAILQ_HEAD(entries, entry);
struct entry {
	TAILQ_ENTRY(entry) child;
	TAILQ_ENTRY(entry) stack;
	enum entry_type type;
	union {
		struct entry_scalar scalar;
		struct entry_table table;
		struct entry_enum anum;
		struct entry_vector vector;
	} u;
	struct entries childs;
	struct entry *parent;
};

struct linearbuffers_encoder {
	struct entry *root;
	struct entries stack;
};

static const char * entry_type_string (enum entry_type type)
{
	switch (type) {
		case entry_type_scalar: return "scalar";
		case entry_type_table: return "table";
		case entry_type_enum: return "enum";
		case entry_type_vector: return "vector";
		default:
			break;
	}
	return "unknown";
}

static const char * entry_scalar_type_string (enum entry_scalar_type type)
{
	switch (type) {
		case entry_scalar_type_int8: return "int8";
		case entry_scalar_type_int16: return "int16";
		case entry_scalar_type_int32: return "int32";
		case entry_scalar_type_int64: return "int64";
		case entry_scalar_type_uint8: return "uint8";
		case entry_scalar_type_uint16: return "uint16";
		case entry_scalar_type_uint32: return "uint32";
		case entry_scalar_type_uint64: return "uint64";
		default:
			break;
	}
	return "unknown";
}

static const char * entry_vector_type_string (enum entry_vector_type type)
{
	switch (type) {
		case entry_vector_type_int8: return "int8";
		case entry_vector_type_int16: return "int16";
		case entry_vector_type_int32: return "int32";
		case entry_vector_type_int64: return "int64";
		case entry_vector_type_uint8: return "uint8";
		case entry_vector_type_uint16: return "uint16";
		case entry_vector_type_uint32: return "uint32";
		case entry_vector_type_uint64: return "uint64";
		case entry_vector_type_table: return "table";
		default:
			break;
	}
	return "unknown";
}

static void entry_dump (struct entry *entry, int prefix)
{
	char *pfx;
	struct entry *child;
	if (entry == NULL) {
		return;
	}
	pfx = malloc(prefix + 1 + 4);
	memset(pfx, ' ', prefix);
	pfx[prefix] = '\0';
	fprintf(stderr, "%s%s", pfx, entry_type_string(entry->type));
	if (entry->type == entry_type_scalar) {
		if (entry->u.scalar.type == entry_scalar_type_uint8) {
			fprintf(stderr, ", %s: %u", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.uint8);
		} else if (entry->u.scalar.type == entry_scalar_type_uint16) {
			fprintf(stderr, ", %s: %u", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.uint16);
		} else if (entry->u.scalar.type == entry_scalar_type_uint32) {
			fprintf(stderr, ", %s: %u", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.uint32);
		} else if (entry->u.scalar.type == entry_scalar_type_uint64) {
			fprintf(stderr, ", %s: %lu", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.uint64);
		} else if (entry->u.scalar.type == entry_scalar_type_int8) {
			fprintf(stderr, ", %s: %d", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.int8);
		} else if (entry->u.scalar.type == entry_scalar_type_int16) {
			fprintf(stderr, ", %s: %d", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.int16);
		} else if (entry->u.scalar.type == entry_scalar_type_int32) {
			fprintf(stderr, ", %s: %d", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.int32);
		} else if (entry->u.scalar.type == entry_scalar_type_int64) {
			fprintf(stderr, ", %s: %ld", entry_scalar_type_string(entry->u.scalar.type), entry->u.scalar.u.int64);
		}
	} if (entry->type == entry_type_vector) {
		if (entry->u.vector.type == entry_vector_type_uint8) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.uint8.length);
		} else if (entry->u.vector.type == entry_vector_type_uint16) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.uint16.length);
		} else if (entry->u.vector.type == entry_vector_type_uint32) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.uint32.length);
		} else if (entry->u.vector.type == entry_vector_type_uint64) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.uint64.length);
		} else if (entry->u.vector.type == entry_vector_type_int8) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.int8.length);
		} else if (entry->u.vector.type == entry_vector_type_int16) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.int16.length);
		} else if (entry->u.vector.type == entry_vector_type_int32) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.int32.length);
		} else if (entry->u.vector.type == entry_vector_type_int64) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.int64.length);
		} else if (entry->u.vector.type == entry_vector_type_table) {
			fprintf(stderr, ", %s: %lu", entry_vector_type_string(entry->u.vector.type), entry->u.vector.u.table.length);
		}
	}
	fprintf(stderr, "\n");
	TAILQ_FOREACH(child, &entry->childs, child) {
		entry_dump(child, prefix + 2);
	}
	free(pfx);
}

static void entry_destroy (struct entry *entry)
{
	struct entry *child;
	struct entry *nchild;
	if (entry == NULL) {
		return;
	}
	if (entry->type == entry_type_table) {
		if (entry->u.table.offsets != NULL) {
			free(entry->u.table.offsets);
		}
	}
	if (entry->type == entry_type_vector) {
		if (entry->u.vector.type == entry_vector_type_uint8) {
			if (entry->u.vector.u.uint8.value != NULL) {
				free(entry->u.vector.u.uint8.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_uint16) {
			if (entry->u.vector.u.uint16.value != NULL) {
				free(entry->u.vector.u.uint16.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_uint32) {
			if (entry->u.vector.u.uint32.value != NULL) {
				free(entry->u.vector.u.uint32.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_uint64) {
			if (entry->u.vector.u.uint64.value != NULL) {
				free(entry->u.vector.u.uint64.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_int8) {
			if (entry->u.vector.u.int8.value != NULL) {
				free(entry->u.vector.u.int8.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_int16) {
			if (entry->u.vector.u.int16.value != NULL) {
				free(entry->u.vector.u.int16.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_int32) {
			if (entry->u.vector.u.int32.value != NULL) {
				free(entry->u.vector.u.int32.value);
			}
		} else if (entry->u.vector.type == entry_vector_type_int64) {
			if (entry->u.vector.u.int64.value != NULL) {
				free(entry->u.vector.u.int64.value);
			}
		}
	}
	TAILQ_FOREACH_SAFE(child, &entry->childs, child, nchild) {
		TAILQ_REMOVE(&entry->childs, child, child);
		entry_destroy(child);
	}
	free(entry);
}

static struct entry * entry_table_create (struct entry *parent, uint64_t nelements)
{
	struct entry *entry;
	entry = malloc(sizeof(struct entry));
	if (entry == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_table;
	entry->parent = parent;
	entry->u.table.nelements = nelements;
	entry->u.table.offsets = malloc(sizeof(uint64_t) * nelements);
	if (entry->u.table.offsets == NULL) {
		fprintf(stderr, "can not allocate memory");
		goto bail;
	}
	memset(entry->u.table.offsets, 0, sizeof(uint64_t) * nelements);
	return entry;
bail:	if (entry != NULL) {
		entry_destroy(entry);
	}
	return NULL;
}

#define entry_scalar_type_create(__type__) \
	static struct entry * entry_scalar_ ## __type__ ## _create (struct entry *parent, __type__ ## _t value) \
	{ \
		struct entry *entry; \
		entry = malloc(sizeof(struct entry)); \
		if (entry == NULL) { \
			fprintf(stderr, "can not allocate memory\n"); \
			goto bail; \
		} \
		memset(entry, 0, sizeof(struct entry)); \
		TAILQ_INIT(&entry->childs); \
		entry->type = entry_type_scalar; \
		entry->parent = parent; \
		entry->u.scalar.type = entry_scalar_type_ ## __type__; \
		entry->u.scalar.u.__type__ = value; \
		return entry; \
	bail:	if (entry != NULL) { \
			entry_destroy(entry); \
		} \
		return NULL; \
	}

entry_scalar_type_create(int8);
entry_scalar_type_create(int16);
entry_scalar_type_create(int32);
entry_scalar_type_create(int64);
entry_scalar_type_create(uint8);
entry_scalar_type_create(uint16);
entry_scalar_type_create(uint32);
entry_scalar_type_create(uint64);

#define entry_vector_type_create(__type__) \
	static struct entry * entry_vector_ ## __type__ ## _create (struct entry *parent, __type__ ## _t *value, uint64_t length) \
	{ \
		struct entry *entry; \
		entry = malloc(sizeof(struct entry)); \
		if (entry == NULL) { \
			fprintf(stderr, "can not allocate memory\n"); \
			goto bail; \
		} \
		memset(entry, 0, sizeof(struct entry)); \
		TAILQ_INIT(&entry->childs); \
		entry->type = entry_type_vector; \
		entry->parent = parent; \
		entry->u.vector.type = entry_vector_type_ ## __type__; \
		entry->u.vector.u.__type__.value = malloc(sizeof(__type__ ## _t) * length); \
		if (entry->u.vector.u.__type__.value == NULL) { \
			fprintf(stderr, "can not allocate memory\n"); \
			goto bail; \
		} \
		memcpy(entry->u.vector.u.__type__.value, value, sizeof(__type__ ## _t) * length); \
		entry->u.vector.u.__type__.length = length; \
		return entry; \
	bail:	if (entry != NULL) { \
			entry_destroy(entry); \
		} \
		return NULL; \
	}

entry_vector_type_create(int8);
entry_vector_type_create(int16);
entry_vector_type_create(int32);
entry_vector_type_create(int64);
entry_vector_type_create(uint8);
entry_vector_type_create(uint16);
entry_vector_type_create(uint32);
entry_vector_type_create(uint64);

static struct entry * entry_vector_create (struct entry *parent)
{
	struct entry *entry;
	entry = malloc(sizeof(struct entry));
	if (entry == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_vector;
	entry->parent = parent;
	return entry;
bail:	if (entry != NULL) {
		entry_destroy(entry);
	}
	return NULL;
}

struct linearbuffers_encoder * linearbuffers_encoder_create (struct linearbuffers_encoder_create_options *options)
{
	struct linearbuffers_encoder *encoder;
	encoder = NULL;
	(void) options;
	encoder = malloc(sizeof(struct linearbuffers_encoder));
	if (encoder == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(encoder, 0, sizeof(struct linearbuffers_encoder));
	TAILQ_INIT(&encoder->stack);
	return encoder;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return NULL;
}

void linearbuffers_encoder_destroy (struct linearbuffers_encoder *encoder)
{
	if (encoder == NULL) {
		return;
	}
	if (encoder->root != NULL) {
		entry_destroy(encoder->root);
	}
	free(encoder);
}

int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder)
{
	if (encoder == NULL) {
		fprintf(stderr, "encoder is invalid\n");
		goto bail;
	}
	return 0;
bail:	return -1;
}

int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t nelements)
{
	struct entry *entry;
	struct entry *parent;
	if (encoder == NULL) {
		fprintf(stderr, "encoder is invalid\n");
		goto bail;
	}
	if (nelements == 0) {
		fprintf(stderr, "nelements is invalid\n");
		goto bail;
	}
	if (encoder->root == NULL) {
		if (element != UINT64_MAX) {
			fprintf(stderr, "logic error: element is invalid\n");
			goto bail;
		}
		parent = NULL;
	} else {
		if (TAILQ_EMPTY(&encoder->stack)) {
			fprintf(stderr, "logic error: stack is empty\n");
			goto bail;
		}
		parent = TAILQ_LAST(&encoder->stack, entries);
		if (parent == NULL) {
			fprintf(stderr, "logic error: parent is invalid\n");
			goto bail;
		}
		if (parent->type == entry_type_table) {
			if (element >= parent->u.table.nelements) {
				fprintf(stderr, "logic error: element is invalid\n");
				goto bail;
			}
		} else if (parent->type == entry_type_vector) {
			if (element != UINT64_MAX) {
				fprintf(stderr, "logic error: element is invalid\n");
				goto bail;
			}
		} else {
			fprintf(stderr, "logic error: parent type: %s is invalid\n", entry_type_string(parent->type));
			goto bail;
		}
	}
	entry = entry_table_create(parent, nelements);
	if (entry == NULL) {
		fprintf(stderr, "can not create entry table\n");
		goto bail;
	}
	if (parent == NULL) {
		encoder->root = entry;
	} else {
		TAILQ_INSERT_TAIL(&parent->childs, entry, child);
		if (parent->type == entry_type_vector) {
			parent->u.vector.u.table.length += 1;
		}
	}
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_table_set_type(__type__) \
	int linearbuffers_encoder_table_set_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, __type__ ## _t value) \
	{ \
		struct entry *entry; \
		struct entry *parent; \
		if (encoder == NULL) { \
			fprintf(stderr, "encoder is invalid\n"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			fprintf(stderr, "logic error: stack is empty\n"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			fprintf(stderr, "logic error: parent is invalid\n"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			fprintf(stderr, "logic error: parent is invalid\n"); \
			goto bail; \
		} \
		if (element >= parent->u.table.nelements) { \
			fprintf(stderr, "logic error: element is invalid\n"); \
			goto bail; \
		} \
		entry = entry_scalar_ ## __type__ ## _create(parent, value); \
		if (entry == NULL) { \
			fprintf(stderr, "can not create entry scalar\n"); \
			goto bail; \
		} \
		TAILQ_INSERT_TAIL(&parent->childs, entry, child); \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_table_set_type(int8);
linearbuffers_encoder_table_set_type(int16);
linearbuffers_encoder_table_set_type(int32);
linearbuffers_encoder_table_set_type(int64);

linearbuffers_encoder_table_set_type(uint8);
linearbuffers_encoder_table_set_type(uint16);
linearbuffers_encoder_table_set_type(uint32);
linearbuffers_encoder_table_set_type(uint64);

#define linearbuffers_encoder_table_set_vector_type(__type__) \
	int linearbuffers_encoder_table_set_vector_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, __type__ ## _t *value, uint64_t length) \
	{ \
		struct entry *entry; \
		struct entry *parent; \
		if (encoder == NULL) { \
			fprintf(stderr, "encoder is invalid\n"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			fprintf(stderr, "logic error: stack is empty\n"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			fprintf(stderr, "logic error: parent is invalid\n"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			fprintf(stderr, "logic error: parent is invalid\n"); \
			goto bail; \
		} \
		if (element >= parent->u.table.nelements) { \
			fprintf(stderr, "logic error: element is invalid\n"); \
			goto bail; \
		} \
		entry = entry_vector_ ## __type__ ## _create(parent, value, length); \
		if (entry == NULL) { \
			fprintf(stderr, "can not create entry vector\n"); \
			goto bail; \
		} \
		TAILQ_INSERT_TAIL(&parent->childs, entry, child); \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_table_set_vector_type(int8);
linearbuffers_encoder_table_set_vector_type(int16);
linearbuffers_encoder_table_set_vector_type(int32);
linearbuffers_encoder_table_set_vector_type(int64);

linearbuffers_encoder_table_set_vector_type(uint8);
linearbuffers_encoder_table_set_vector_type(uint16);
linearbuffers_encoder_table_set_vector_type(uint32);
linearbuffers_encoder_table_set_vector_type(uint64);

int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		fprintf(stderr, "encoder is invalid\n");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		fprintf(stderr, "logic error: stack is empty\n");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		fprintf(stderr, "logic error: entry is invalid\n");
		goto bail;
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (TAILQ_EMPTY(&encoder->stack)) {
		fprintf(stderr, "linearbuffers dump:\n");
		entry_dump(encoder->root, 2);
	}
	return 0;
bail:	return -1;
}

int linearbuffers_encoder_vector_start (struct linearbuffers_encoder *encoder, uint64_t element)
{
	struct entry *entry;
	struct entry *parent;
	if (encoder == NULL) {
		fprintf(stderr, "encoder is invalid\n");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		fprintf(stderr, "logic error: stack is empty\n");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		fprintf(stderr, "logic error: parent is invalid\n");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		fprintf(stderr, "logic error: parent is invalid\n");
		goto bail;
	}
	if (element >= parent->u.table.nelements) {
		fprintf(stderr, "logic error: element is invalid\n");
		goto bail;
	}
	entry = entry_vector_create(parent);
	if (entry == NULL) {
		fprintf(stderr, "can not create entry vector\n");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	return -1;
}

int linearbuffers_encoder_vector_end (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		fprintf(stderr, "encoder is invalid\n");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		fprintf(stderr, "logic error: stack is empty\n");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		fprintf(stderr, "logic error: entry is invalid\n");
		goto bail;
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	return 0;
bail:	return -1;
}
