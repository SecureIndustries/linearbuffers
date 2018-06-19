
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define LINEARBUFFERS_DEBUG_NAME "encoder"

#include "debug.h"
#include "queue.h"
#include "encoder.h"

#if !defined(MAX)
#define MAX(a, b)			((a) < (b) ? (b) : (a))
#endif

#define POOL_ENABLE			1
#define POOL_DEFAULT_BLOCKS_COUNT	1

struct pool_element {
	struct pool_element *next;
};

struct pool {
	uint64_t element_size;
	uint64_t block_size;
	uint64_t used;
	uint64_t block;
	struct pool_element *freed;
	uint64_t blocks_used;
	uint8_t **blocks;
};

static int pool_init (struct pool *p, const uint64_t element_size, const uint64_t block_size);
static void pool_uninit (struct pool *p);

#if defined(POOL_ENABLE) && (POOL_ENABLE == 1)

static void * pool_malloc (struct pool *p);
static void pool_free (struct pool *p, void *ptr);

#else

#define pool_malloc(p) malloc((p)->element_size)
#define pool_free(p, d) free(d)

#endif

static void pool_uninit (struct pool *p)
{
	uint64_t i;
	for(i = 0; i < p->blocks_used; i++) {
		if (p->blocks[i] == NULL) {
			break;
		} else {
			free(p->blocks[i]);
		}
	}
	free(p->blocks);
	memset(p, 0, sizeof(struct pool));
}

static int pool_init (struct pool *p, const uint64_t element_size, const uint64_t block_size)
{
	uint64_t i;
	memset(p, 0, sizeof(struct pool));
	linearbuffers_debugf("init element_size: %lu, block_size: %lu", element_size, block_size);
	p->element_size = sizeof(struct pool_element) + element_size;
	p->block_size = block_size;
	p->blocks_used = POOL_DEFAULT_BLOCKS_COUNT;
	p->block = 0;
	p->blocks = malloc(sizeof(uint8_t *) *  p->blocks_used);
	if (p->blocks == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	for (i = 0; i < p->blocks_used; ++i) {
		p->blocks[i] = malloc(p->element_size * p->block_size);
		if (p->blocks[i] == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	pool_uninit(p);
	return -1;
}

#if defined(POOL_ENABLE) && (POOL_ENABLE == 1)

static void * pool_malloc (struct pool *p)
{
	struct pool_element *element;
	if (p->freed != NULL) {
		element = p->freed;
		p->freed = p->freed->next;
		return ((uint8_t *) element) + sizeof(struct pool_element);
	}
	if (p->used + 1 > p->block_size) {
		p->used = 0;
		if (p->block + 1 >= p->blocks_used) {
			uint64_t i;
			void *blocks;
			blocks = realloc(p->blocks, sizeof(uint8_t *) *  (p->blocks_used + 1));
			if (blocks == NULL) {
				linearbuffers_errorf("can not allocate memory");
				return NULL;
			} else {
				p->blocks = blocks;
			}
			for (i = p->blocks_used; i < p->blocks_used + 1; i++) {
				p->blocks[i] = NULL;
			}
			p->blocks_used += 1;
		}
		if (p->blocks[p->block + 1] == NULL) {
			p->blocks[p->block + 1] = malloc(p->element_size * p->block_size);
			if (p->blocks[p->block + 1] == NULL) {
				linearbuffers_errorf("can not allocate memory");
				return NULL;
			}
		}
		p->block += 1;
	}
	element = (struct pool_element *) (p->blocks[p->block] + p->used * p->element_size);
	p->used += 1;
	return ((uint8_t *) element) + sizeof(struct pool_element);
}

static void pool_free (struct pool *p, void *ptr)
{
	struct pool_element *freed = p->freed;
	p->freed = (struct pool_element *) (((uint8_t *) ptr) - sizeof(struct pool_element));
	p->freed->next = freed;
}

#endif

enum entry_type {
	entry_type_unknown,
	entry_type_scalar,
	entry_type_enum,
	entry_type_table,
	entry_type_vector,
};

struct entry_table {
	uint64_t elements;
	uint8_t present[32];
};

struct entry_vector {
};

TAILQ_HEAD(entries, entry);
struct entry {
	TAILQ_ENTRY(entry) child;
	TAILQ_ENTRY(entry) stack;
	enum entry_type type;
	union {
		struct entry_table table;
		struct entry_vector vector;
	} u;
	uint64_t offset;
	struct entries childs;
	struct entry *parent;
};

struct linearbuffers_encoder {
	struct entry *root;
	struct entries stack;
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, uint64_t length);
		void *context;
		uint64_t offset;
	} emitter;
	struct {
		void *buffer;
		uint64_t length;
		uint64_t size;
	} output;
	struct {
		struct pool entry;
	} pool;
};

static int encoder_default_emitter (void *context, uint64_t offset, const void *buffer, uint64_t length)
{
	struct linearbuffers_encoder *encoder = context;
	linearbuffers_debugf("emitter offset: %08" PRIu64 ", buffer: %11p, length: %08" PRIu64 "", offset, buffer, length);
	if (encoder->output.size < offset + length) {
		void *tmp;
		uint64_t size;
		size = (((offset + length) + 4095) / 4096) * 4096;
		tmp = realloc(encoder->output.buffer, size);
		if (tmp == NULL) {
			tmp = malloc(size);
			if (tmp == NULL) {
				linearbuffers_debugf("can not allocate memory");
				goto bail;
			}
			memcpy(tmp, encoder->output.buffer, encoder->output.length);
			encoder->output.buffer = tmp;
			free(tmp);
		} else {
			encoder->output.buffer = tmp;
		}
		encoder->output.size = size;
	}
	if (buffer == NULL) {
		memset(encoder->output.buffer + offset, 0, length);
	} else {
		memcpy(encoder->output.buffer + offset, buffer, length);
	}
	encoder->output.length = MAX(encoder->output.length, offset + length);
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) const void * linearbuffers_encoder_linearized (struct linearbuffers_encoder *encoder, uint64_t *length)
{
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		return NULL;
	}
	if (length != NULL) {
		*length = encoder->output.length;
	}
	return encoder->output.buffer;
}

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

static void entry_destroy (struct pool *pool, struct entry *entry)
{
	struct entry *child;
	struct entry *nchild;
	if (entry == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(child, &entry->childs, child, nchild) {
		TAILQ_REMOVE(&entry->childs, child, child);
		entry_destroy(pool, child);
	}
#if 0
	if (entry->u.table.present != NULL) {
		free(entry->u.table.present);
	}
#endif
	pool_free(pool, entry);
}

static struct entry * entry_vector_create (struct pool *pool, struct entry *parent)
{
	struct entry *entry;
	entry = pool_malloc(pool);
	if (entry == NULL) {
		linearbuffers_debugf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_vector;
	entry->parent = parent;
	return entry;
bail:	if (entry != NULL) {
		entry_destroy(pool, entry);
	}
	return NULL;
}

__attribute__ ((__visibility__("default"))) struct linearbuffers_encoder * linearbuffers_encoder_create (struct linearbuffers_encoder_create_options *options)
{
	struct linearbuffers_encoder *encoder;
	encoder = NULL;
	encoder = malloc(sizeof(struct linearbuffers_encoder));
	if (encoder == NULL) {
		linearbuffers_debugf("can not allocate memory");
		goto bail;
	}
	memset(encoder, 0, sizeof(struct linearbuffers_encoder));
	TAILQ_INIT(&encoder->stack);
	pool_init(&encoder->pool.entry, sizeof(struct entry), 32);
	encoder->emitter.function = encoder_default_emitter;
	encoder->emitter.context = encoder;
	if (options != NULL) {
		if (options->emitter.function != NULL) {
			encoder->emitter.function = options->emitter.function;
			encoder->emitter.context = options->emitter.context;
		}
	}
	return encoder;
bail:	if (encoder != NULL) {
		linearbuffers_encoder_destroy(encoder);
	}
	return NULL;
}

__attribute__ ((__visibility__("default"))) void linearbuffers_encoder_destroy (struct linearbuffers_encoder *encoder)
{
	if (encoder == NULL) {
		return;
	}
	if (encoder->root != NULL) {
		entry_destroy(&encoder->pool.entry, encoder->root);
	}
	if (encoder->output.buffer != NULL) {
		free(encoder->output.buffer);
	}
	pool_uninit(&encoder->pool.entry);
	free(encoder);
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder, struct linearbuffers_encoder_reset_options *options)
{
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	entry_destroy(&encoder->pool.entry, encoder->root);
	encoder->root = NULL;
	encoder->emitter.offset = 0;
	TAILQ_INIT(&encoder->stack);
	encoder->output.length = 0;
	encoder->emitter.function = encoder_default_emitter;
	encoder->emitter.context = encoder;
	if (options != NULL) {
		if (options->emitter.function != NULL) {
			encoder->emitter.function = options->emitter.function;
			encoder->emitter.context = options->emitter.context;
		}
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t elements, uint64_t size)
{
	struct entry *entry;
	struct entry *parent;
	entry = NULL;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (elements == 0) {
		linearbuffers_debugf("elements is invalid");
		goto bail;
	}
	if (encoder->root == NULL) {
		if (element != UINT64_MAX) {
			linearbuffers_debugf("logic error: element is invalid");
			goto bail;
		}
		parent = NULL;
	} else {
		if (TAILQ_EMPTY(&encoder->stack)) {
			linearbuffers_debugf("logic error: stack is empty");
			goto bail;
		}
		parent = TAILQ_LAST(&encoder->stack, entries);
		if (parent == NULL) {
			linearbuffers_debugf("logic error: parent is invalid");
			goto bail;
		}
		if (parent->type == entry_type_table) {
			if (element >= parent->u.table.elements) {
				linearbuffers_debugf("logic error: element is invalid");
				goto bail;
			}
			parent->u.table.present[element / 8] |= (1 << (element % 8));
			encoder->emitter.function(encoder->emitter.context, parent->offset + (sizeof(uint8_t) * ((parent->u.table.elements + 7) / 8)) + offset, &encoder->emitter.offset, sizeof(uint64_t));
		} else if (parent->type == entry_type_vector) {
			if (element != UINT64_MAX) {
				linearbuffers_debugf("logic error: element is invalid");
				goto bail;
			}
		} else {
			linearbuffers_debugf("logic error: parent type: %s is invalid", entry_type_string(parent->type));
			goto bail;
		}
	}
	entry = pool_malloc(&encoder->pool.entry);
	if (entry == NULL) {
		linearbuffers_debugf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_table;
	entry->parent = parent;
	entry->u.table.elements = elements;
#if 0
	entry->u.table.present = malloc((sizeof(uint8_t) * ((elements + 7) / 8)));
	if (entry->u.table.present == NULL) {
		linearbuffers_debugf("can not allocate memory");
		goto bail;
	}
#else
	if (sizeof(entry->u.table) < (sizeof(uint8_t) * ((elements + 7) / 8))) {
		linearbuffers_debugf("can not allocate memory");
		goto bail;
	}
#endif
	memset(entry->u.table.present, 0, (sizeof(uint8_t) * ((elements + 7) / 8)));
	if (parent == NULL) {
		encoder->root = entry;
	} else {
		TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	}
	entry->offset = encoder->emitter.offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, (sizeof(uint8_t) * ((elements + 7) / 8)) + size);
	encoder->emitter.offset += (sizeof(uint8_t) * ((elements + 7) / 8)) + size;
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	if (entry != NULL) {
		entry_destroy(&encoder->pool.entry, entry);
	}
	return -1;
}

#define linearbuffers_encoder_table_set_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, __type__ ## _t value) \
	{ \
		struct entry *parent; \
		if (encoder == NULL) { \
			linearbuffers_debugf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_debugf("logic error: stack is empty"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			linearbuffers_debugf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			linearbuffers_debugf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (element >= parent->u.table.elements) { \
			linearbuffers_debugf("logic error: element is invalid"); \
			goto bail; \
		} \
		parent->u.table.present[element / 8] |= (1 << (element % 8)); \
		encoder->emitter.function(encoder->emitter.context, parent->offset + (sizeof(uint8_t) * ((parent->u.table.elements + 7) / 8)) + offset, &value, sizeof(__type__ ## _t)); \
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

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value)
{
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (value == NULL) {
		linearbuffers_debugf("value is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_debugf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_debugf("logic error: element is invalid");
		goto bail;
	}
	parent->u.table.present[element / 8] |= (1 << (element % 8));
	encoder->emitter.function(encoder->emitter.context, parent->offset + (sizeof(uint8_t) * ((parent->u.table.elements + 7) / 8)) + offset, &encoder->emitter.offset, sizeof(uint64_t));
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, strlen(value) + sizeof(char));
	encoder->emitter.offset += strlen(value) + sizeof(char);
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_table_set_vector_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_vector_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const __type__ ## _t *value, uint64_t count) \
	{ \
		struct entry *parent; \
		if (encoder == NULL) { \
			linearbuffers_debugf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_debugf("logic error: stack is empty"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			linearbuffers_debugf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			linearbuffers_debugf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (element >= parent->u.table.elements) { \
			linearbuffers_debugf("logic error: element is invalid"); \
			goto bail; \
		} \
		parent->u.table.present[element / 8] |= (1 << (element % 8)); \
		encoder->emitter.function(encoder->emitter.context, parent->offset + (sizeof(uint8_t) * ((parent->u.table.elements + 7) / 8)) + offset, &encoder->emitter.offset, sizeof(uint64_t)); \
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, &count, sizeof(uint64_t)); \
		encoder->emitter.offset += sizeof(uint64_t); \
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, count * sizeof(__type__ ## _t)); \
		encoder->emitter.offset += count * sizeof(__type__ ## _t); \
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

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_vector_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char **value, uint64_t count)
{
	uint64_t i;
	uint64_t counts;
	uint64_t offsets;
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_debugf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_debugf("logic error: element is invalid");
		goto bail;
	}
	parent->u.table.present[element / 8] |= (1 << (element % 8));
	encoder->emitter.function(encoder->emitter.context, parent->offset + (sizeof(uint8_t) * ((parent->u.table.elements + 7) / 8)) + offset, &encoder->emitter.offset, sizeof(uint64_t));
	counts = encoder->emitter.offset;
	encoder->emitter.offset += sizeof(uint64_t);
	offsets = encoder->emitter.offset;
	encoder->emitter.offset += sizeof(uint64_t) * count;
	for (i = 0; i < count; i++) {
		encoder->emitter.function(encoder->emitter.context, offsets, &encoder->emitter.offset, sizeof(uint64_t));
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value[i], strlen(value[i]) + 1);
		offsets += sizeof(uint64_t);
		encoder->emitter.offset += strlen(value[i]) + 1;
	}
	encoder->emitter.function(encoder->emitter.context, counts, &count, sizeof(uint64_t));
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_debugf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_debugf("logic error: entry is invalid");
		goto bail;
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	encoder->emitter.function(encoder->emitter.context, entry->offset, entry->u.table.present, (sizeof(uint8_t) * ((entry->u.table.elements + 7) / 8)));
	if (entry->parent != NULL) {
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, entry);
	} else {
		entry_destroy(&encoder->pool.entry, entry);
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start (struct linearbuffers_encoder *encoder, uint64_t element)
{
	struct entry *entry;
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_debugf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_debugf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_debugf("logic error: element is invalid");
		goto bail;
	}
	entry = entry_vector_create(&encoder->pool.entry, parent);
	if (entry == NULL) {
		linearbuffers_debugf("can not create entry vector");
		goto bail;
	}
	entry->offset = encoder->emitter.offset;
	TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_debugf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_debugf("logic error: entry is invalid");
		goto bail;
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, entry);
	} else {
		entry_destroy(&encoder->pool.entry, entry);
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}
