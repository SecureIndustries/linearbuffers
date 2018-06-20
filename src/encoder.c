
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define LINEARBUFFERS_DEBUG_NAME "encoder"

#include "debug.h"
#include "queue.h"
#include "encoder.h"

#if !defined(MIN)
#define MIN(a, b)			((a) < (b) ? (a) : (b))
#endif
#if !defined(MAX)
#define MAX(a, b)			((a) > (b) ? (a) : (b))
#endif

#define POOL_ENABLE			1

struct pool_element {
	struct pool_element *next;
};

struct pool_block {
	struct pool_block *next;
	uint8_t buffer[0];
};

struct pool {
	uint64_t selements;
	uint64_t nelements;
	uint64_t uelements;
	struct pool_element *felements;
	struct pool_block *cblock;
	struct pool_block *blocks;
};

static int pool_init (struct pool *pool, const uint64_t selements, const uint64_t nelements);
static void pool_uninit (struct pool *pool);

#if defined(POOL_ENABLE) && (POOL_ENABLE == 1)

static void * pool_malloc (struct pool *pool);
static void pool_free (struct pool *pool, void *ptr);

#else

#define pool_malloc(p) malloc((p)->selements)
#define pool_free(p, d) free(d)

#endif

static void pool_uninit (struct pool *pool)
{
	struct pool_block *block;
	struct pool_block *nblock;
	for (block = pool->blocks; block && ((nblock = block->next), 1); block = nblock) {
		free(block);
	}
	memset(pool, 0, sizeof(struct pool));
}

static int pool_init (struct pool *p, const uint64_t selements, const uint64_t nelements)
{
	memset(p, 0, sizeof(struct pool));
	linearbuffers_debugf("init selements: %lu, nelements: %lu", selements, nelements);
	p->selements = selements;
	p->nelements = nelements;
	p->felements = NULL;
	p->cblock = NULL;
	p->blocks = NULL;
	return 0;
}

#if defined(POOL_ENABLE) && (POOL_ENABLE == 1)

static void * pool_malloc (struct pool *pool)
{
	struct pool_element *element;
	if (pool->felements != NULL) {
		element = pool->felements;
		pool->felements = pool->felements->next;
		return ((uint8_t *) element) + sizeof(struct pool_element);
	}
	linearbuffers_debugf("uelements: %ld, nelements: %ld", pool->uelements, pool->nelements);
	if (pool->uelements + 1 > pool->nelements ||
	    pool->cblock == NULL) {
		struct pool_block *block;
		struct pool_block *blocks;
		linearbuffers_debugf("  creating new block");
		block = malloc(sizeof(struct pool_block) + ((sizeof(struct pool_element) + pool->selements) * pool->nelements));
		if (block == NULL) {
			linearbuffers_errorf("can not allocate memory");
			return NULL;
		}
		blocks = pool->blocks;
		pool->blocks = block;
		pool->blocks->next = blocks;
		pool->cblock = block;
		pool->uelements = 0;
	}
	element = (struct pool_element *) (pool->cblock->buffer + ((sizeof(struct pool_element) + pool->selements) * pool->uelements));
	pool->uelements += 1;
	return ((uint8_t *) element) + sizeof(struct pool_element);
}

static void pool_free (struct pool *p, void *ptr)
{
	struct pool_element *felements = p->felements;
	p->felements = (struct pool_element *) (((uint8_t *) ptr) - sizeof(struct pool_element));
	p->felements->next = felements;
}

#endif

#define PRESENT_BUFFER_COUNT	32
struct present_buffer {
	struct present_buffer *next;
	uint8_t buffer[PRESENT_BUFFER_COUNT];
};

struct present {
	uint64_t bytes;
	struct present_buffer *buffers;
};

enum entry_type {
	entry_type_unknown,
	entry_type_scalar,
	entry_type_enum,
	entry_type_table,
	entry_type_vector,
};

struct entry_table {
	uint64_t elements;
	struct present present;
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
		struct pool present;
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

static int present_mark (struct present *present, uint64_t element)
{
	uint64_t byte;
	uint64_t buffer;
	struct present_buffer *present_buffer;
	byte = element / 8;
	buffer = byte / PRESENT_BUFFER_COUNT;
	present_buffer = present->buffers;
	while (buffer--) {
		present_buffer = present_buffer->next;
	}
	present_buffer->buffer[byte % PRESENT_BUFFER_COUNT] |= (1 << (element % 8));
	return 0;
}

static void present_uninit (struct pool *pool, struct present *present)
{
	struct present_buffer *buffer;
	struct present_buffer *nbuffer;
	for (buffer = present->buffers; buffer && ((nbuffer = buffer->next), 1); buffer = nbuffer) {
		pool_free(pool, buffer);
	}
	memset(present, 0, sizeof(struct present));
}

static int present_init (struct pool *pool, struct present *present, uint64_t elements)
{
	uint64_t buffers;
	struct present_buffer *present_buffer;
	struct present_buffer *present_buffers;
	memset(present, 0, sizeof(struct present));
	present->bytes = sizeof(uint8_t) * ((elements + 7) / 8);
	buffers = (present->bytes + PRESENT_BUFFER_COUNT - 1) / PRESENT_BUFFER_COUNT;
	fprintf(stderr, "buffers: %ld\n", buffers);
	while (buffers--) {
		present_buffer = pool_malloc(pool);
		if (present_buffer == NULL) {
			linearbuffers_debugf("can not allocate memory");
			goto bail;
		}
		memset(present_buffer->buffer, 0, PRESENT_BUFFER_COUNT);
		present_buffers = present->buffers;
		present->buffers = present_buffer;
		present->buffers->next = present_buffers;
	}
	return 0;
bail:	present_uninit(pool, present);
	return -1;
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

static void entry_destroy (struct pool *epool, struct pool *ppool, struct entry *entry)
{
	struct entry *child;
	struct entry *nchild;
	if (entry == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(child, &entry->childs, child, nchild) {
		TAILQ_REMOVE(&entry->childs, child, child);
		entry_destroy(epool, ppool, child);
	}
	if (entry->type == entry_type_table) {
		present_uninit(ppool, &entry->u.table.present);
	}
	pool_free(epool, entry);
}

static struct entry * entry_vector_create (struct pool *epool, struct pool *ppool, struct entry *parent)
{
	struct entry *entry;
	entry = pool_malloc(epool);
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
		entry_destroy(epool, ppool, entry);
	}
	return NULL;
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
	pool_init(&encoder->pool.present, sizeof(struct present_buffer), 32);
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
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, encoder->root);
	}
	if (encoder->output.buffer != NULL) {
		free(encoder->output.buffer);
	}
	pool_uninit(&encoder->pool.entry);
	pool_uninit(&encoder->pool.present);
	free(encoder);
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder, struct linearbuffers_encoder_reset_options *options)
{
	if (encoder == NULL) {
		linearbuffers_debugf("encoder is invalid");
		goto bail;
	}
	entry_destroy(&encoder->pool.entry, &encoder->pool.present, encoder->root);
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
	int rc;
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
			present_mark(&parent->u.table.present, element);
			encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &encoder->emitter.offset, sizeof(uint64_t));
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
	rc = present_init(&encoder->pool.present, &entry->u.table.present, elements);
	if (rc != 0) {
		linearbuffers_debugf("can not init table present");
		goto bail;
	}
	if (parent == NULL) {
		encoder->root = entry;
	} else {
		TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	}
	entry->offset = encoder->emitter.offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, entry->u.table.present.bytes + size);
	encoder->emitter.offset += entry->u.table.present.bytes + size;
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	if (entry != NULL) {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
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
		present_mark(&parent->u.table.present, element); \
		encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &value, sizeof(__type__ ## _t)); \
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
	present_mark(&parent->u.table.present, element); \
	encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &encoder->emitter.offset, sizeof(uint64_t));
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
		present_mark(&parent->u.table.present, element); \
		encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &encoder->emitter.offset, sizeof(uint64_t)); \
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
	present_mark(&parent->u.table.present, element); \
	encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &encoder->emitter.offset, sizeof(uint64_t));
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
	uint64_t present_bytes;
	uint64_t present_bufferi;
	struct present_buffer *present_buffer;
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
	for (present_bufferi = 0 , present_bytes = entry->u.table.present.bytes   , present_buffer = entry->u.table.present.buffers;
	     present_buffer;
	     present_bufferi += 1, present_bytes -= PRESENT_BUFFER_COUNT, present_buffer = present_buffer->next) {
		encoder->emitter.function(encoder->emitter.context, entry->offset + present_bufferi * PRESENT_BUFFER_COUNT, present_buffer->buffer, MIN(present_bytes, PRESENT_BUFFER_COUNT));
	}
	if (entry->parent != NULL) {
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
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
	entry = entry_vector_create(&encoder->pool.entry, &encoder->pool.present, parent);
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
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}
