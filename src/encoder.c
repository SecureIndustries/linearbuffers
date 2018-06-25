
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

#define PRESENT_BUFFER_COUNT	(64 / 8)
struct present_buffer {
	struct present_buffer *next;
	uint8_t buffer[PRESENT_BUFFER_COUNT];
};

struct present_table {
	uint64_t bytes;
	struct present_buffer *buffers;
};

#define OFFSET_BUFFER_COUNT	(64)
struct offset_buffer {
	struct offset_buffer *next;
	uint64_t buffer[OFFSET_BUFFER_COUNT];
};

struct offset_table {
	uint64_t index;
	struct offset_buffer *cbuffer;
	struct offset_buffer *buffers;
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
	struct present_table present;
};

enum vector_type {
	vector_type_int8,
	vector_type_int16,
	vector_type_int32,
	vector_type_int64,
	vector_type_uint8,
	vector_type_uint16,
	vector_type_uint32,
	vector_type_uint64,
	vector_type_string,
	vector_type_table,
};

struct entry_vector {
	enum vector_type type;
	uint64_t elements;
	struct offset_table offset;
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
	uint64_t parent_element;
	uint64_t parent_offset;
};

struct linearbuffers_encoder {
	struct entry *root;
	struct entries stack;
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length);
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
		struct pool offset;
	} pool;
};

static int encoder_default_emitter (void *context, uint64_t offset, const void *buffer, int64_t length)
{
	struct linearbuffers_encoder *encoder = context;
	linearbuffers_debugf("emitter offset: %08" PRIu64 ", buffer: %11p, length: %08" PRIi64 "", offset, buffer, length);
	if (length < 0) {
		encoder->output.length = offset + length;
		return 0;
	}
	if (encoder->output.size < offset + length) {
		void *tmp;
		uint64_t size;
		size = (((offset + length) + 4095) / 4096) * 4096;
		tmp = realloc(encoder->output.buffer, size);
		if (tmp == NULL) {
			tmp = malloc(size);
			if (tmp == NULL) {
				linearbuffers_errorf("can not allocate memory");
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

static int present_table_mark (struct present_table *present, uint64_t element)
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

static void present_table_uninit (struct pool *pool, struct present_table *present)
{
	struct present_buffer *buffer;
	struct present_buffer *nbuffer;
	for (buffer = present->buffers; buffer && ((nbuffer = buffer->next), 1); buffer = nbuffer) {
		pool_free(pool, buffer);
	}
	memset(present, 0, sizeof(struct present_table));
}

static int present_table_init (struct pool *pool, struct present_table *present, uint64_t elements)
{
	uint64_t buffers;
	struct present_buffer *present_buffer;
	struct present_buffer *present_buffers;
	memset(present, 0, sizeof(struct present_table));
	present->bytes = sizeof(uint8_t) * ((elements + 7) / 8);
	buffers = (present->bytes + PRESENT_BUFFER_COUNT - 1) / PRESENT_BUFFER_COUNT;
	while (buffers--) {
		present_buffer = pool_malloc(pool);
		if (present_buffer == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
		memset(present_buffer->buffer, 0, PRESENT_BUFFER_COUNT);
		present_buffers = present->buffers;
		present->buffers = present_buffer;
		present->buffers->next = present_buffers;
	}
	return 0;
bail:	present_table_uninit(pool, present);
	return -1;
}

static int offset_table_push (struct offset_table *offset, uint64_t value, struct pool *pool)
{
	if (offset->index + 1 > OFFSET_BUFFER_COUNT ||
	    offset->cbuffer == NULL) {
		struct offset_buffer *buffer;
		buffer = pool_malloc(pool);
		if (buffer == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
		memset(buffer, 0, sizeof(struct offset_buffer));
		if (offset->cbuffer == NULL) {
			offset->buffers = buffer;
		} else {
			offset->cbuffer->next = buffer;
		}
		offset->cbuffer = buffer;
		offset->index = 0;
	}
	offset->cbuffer->buffer[offset->index] = value;
	offset->index += 1;
	return 0;
bail:	return -1;
}

static void offset_table_uninit (struct pool *pool, struct offset_table *offset)
{
	struct offset_buffer *buffer;
	struct offset_buffer *nbuffer;
	for (buffer = offset->buffers; buffer && ((nbuffer = buffer->next), 1); buffer = nbuffer) {
		pool_free(pool, buffer);
	}
	memset(offset, 0, sizeof(struct offset_table));
}

static int offset_table_init (struct pool *pool, struct offset_table *offset)
{
	(void) pool;
	memset(offset, 0, sizeof(struct offset_table));
	return 0;
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
		present_table_uninit(ppool, &entry->u.table.present);
	} else if (entry->type == entry_type_vector) {
		offset_table_uninit(ppool, &entry->u.vector.offset);
	}
	pool_free(epool, entry);
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
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(encoder, 0, sizeof(struct linearbuffers_encoder));
	TAILQ_INIT(&encoder->stack);
	pool_init(&encoder->pool.entry, sizeof(struct entry), 8);
	pool_init(&encoder->pool.present, sizeof(struct present_buffer), 8);
	pool_init(&encoder->pool.offset, sizeof(struct offset_buffer), 8);
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
	pool_uninit(&encoder->pool.offset);
	free(encoder);
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder, struct linearbuffers_encoder_reset_options *options)
{
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
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

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, int headless, uint64_t element, uint64_t offset, uint64_t elements, uint64_t size)
{
	int rc;
	struct entry *entry;
	struct entry *parent;
	entry = NULL;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (elements == 0) {
		linearbuffers_errorf("elements is invalid");
		goto bail;
	}
	if (encoder->root == NULL) {
		if (element != UINT64_MAX ||
		    offset != UINT64_MAX) {
			linearbuffers_errorf("logic error: element is invalid");
			goto bail;
		}
		parent = NULL;
	} else if (headless) {
		parent = NULL;
	} else {
		if (TAILQ_EMPTY(&encoder->stack)) {
			linearbuffers_errorf("logic error: stack is empty");
			goto bail;
		}
		parent = TAILQ_LAST(&encoder->stack, entries);
		if (parent == NULL) {
			linearbuffers_errorf("logic error: parent is invalid");
			goto bail;
		}
		if (parent->type == entry_type_table) {
			if (element >= parent->u.table.elements) {
				linearbuffers_errorf("logic error: element is invalid");
				goto bail;
			}
		} else if (parent->type == entry_type_vector) {
			if (element != UINT64_MAX ||
			    offset != UINT64_MAX) {
				linearbuffers_errorf("logic error: element is invalid");
				goto bail;
			}
		} else {
			linearbuffers_errorf("logic error: parent type: %s is invalid", entry_type_string(parent->type));
			goto bail;
		}
	}
	entry = pool_malloc(&encoder->pool.entry);
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_table;
	entry->parent = parent;
	entry->parent_element = element;
	entry->parent_offset = offset;
	entry->u.table.elements = elements;
	rc = present_table_init(&encoder->pool.present, &entry->u.table.present, elements);
	if (rc != 0) {
		linearbuffers_errorf("can not init table present");
		goto bail;
	}
	if (encoder->root == NULL) {
		encoder->root = entry;
	}
	if (parent != NULL) {
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
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			linearbuffers_errorf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			linearbuffers_errorf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (element >= parent->u.table.elements) { \
			linearbuffers_errorf("logic error: element is invalid"); \
			goto bail; \
		} \
		present_table_mark(&parent->u.table.present, element); \
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

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_table (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value)
{
	uint64_t eoffset;
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_errorf("logic error: element is invalid");
		goto bail;
	}
	present_table_mark(&parent->u.table.present, element);
	eoffset = value - parent->offset;
	encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &eoffset, sizeof(uint64_t));
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value)
{
	uint64_t eoffset;
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (value == NULL) {
		linearbuffers_errorf("value is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_errorf("logic error: element is invalid");
		goto bail;
	}
	present_table_mark(&parent->u.table.present, element);
	eoffset = encoder->emitter.offset - parent->offset;
	encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &eoffset, sizeof(uint64_t));
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, strlen(value) + sizeof(char));
	encoder->emitter.offset += strlen(value) + sizeof(char);
	return 0;
bail:	return -1;
}

int linearbuffers_encoder_table_nset_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value, uint64_t n)
{
	const char _null = 0;
	uint64_t eoffset;
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (value == NULL) {
		linearbuffers_errorf("value is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_errorf("logic error: element is invalid");
		goto bail;
	}
	present_table_mark(&parent->u.table.present, element);
	eoffset = encoder->emitter.offset - parent->offset;
	encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &eoffset, sizeof(uint64_t));
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, n);
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset + n, &_null, 1);
	encoder->emitter.offset += n + 1;
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_table_set_vector_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_vector_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const __type__ ## _t *value, uint64_t count) \
	{ \
		uint64_t eoffset; \
		struct entry *parent; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			linearbuffers_errorf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			linearbuffers_errorf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (element >= parent->u.table.elements) { \
			linearbuffers_errorf("logic error: element is invalid"); \
			goto bail; \
		} \
		present_table_mark(&parent->u.table.present, element); \
		eoffset = encoder->emitter.offset - parent->offset; \
		encoder->emitter.function(encoder->emitter.context, parent->offset + parent->u.table.present.bytes + offset, &eoffset, sizeof(uint64_t)); \
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

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder, uint64_t *offset)
{
	struct entry *entry;
	uint64_t eoffset;
	uint64_t present_bytes;
	uint64_t present_bufferi;
	struct present_buffer *present_buffer;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_table) {
		linearbuffers_errorf("logic error: entry type is not table");
		goto bail;
	}
	if (offset != NULL) {
		*offset = entry->offset;
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	for (present_bufferi = 0 , present_bytes = entry->u.table.present.bytes, present_buffer = entry->u.table.present.buffers;
	     present_buffer;
	     present_bufferi += 1, present_bytes -= PRESENT_BUFFER_COUNT       , present_buffer = present_buffer->next) {
		encoder->emitter.function(encoder->emitter.context, entry->offset + present_bufferi * PRESENT_BUFFER_COUNT, present_buffer->buffer, MIN(present_bytes, PRESENT_BUFFER_COUNT));
	}
	if (entry->parent != NULL) {
		if (entry->parent->type == entry_type_table) {
			present_table_mark(&entry->parent->u.table.present, entry->parent_element);
			eoffset = entry->offset - entry->parent->offset;
			encoder->emitter.function(encoder->emitter.context, entry->parent->offset + entry->parent->u.table.present.bytes + entry->parent_offset, &eoffset, sizeof(uint64_t));
		} else if (entry->parent->type == entry_type_vector) {
			entry->parent->u.vector.elements += 1;
			offset_table_push(&entry->parent->u.vector.offset, entry->offset, &encoder->pool.offset);
		}
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_cancel (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_table) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset);
	encoder->emitter.offset = entry->offset;
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		if (entry->parent->type == entry_type_table) {
		} else if (entry->parent->type == entry_type_vector) {
		}
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_vector_start_scalar_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset) \
	{ \
		int rc; \
		struct entry *entry; \
		struct entry *parent; \
		entry = NULL; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		parent = TAILQ_LAST(&encoder->stack, entries); \
		if (parent == NULL) { \
			linearbuffers_errorf("logic error: parent is invalid"); \
			goto bail; \
		} \
		if (parent->type != entry_type_table) { \
			linearbuffers_errorf("logic error: parent type: %s is invalid", entry_type_string(parent->type)); \
			goto bail; \
		} \
		if (element >= parent->u.table.elements) { \
			linearbuffers_errorf("logic error: element is invalid"); \
			goto bail; \
		} \
		entry = pool_malloc(&encoder->pool.entry); \
		if (entry == NULL) { \
			linearbuffers_errorf("can not allocate memory"); \
			goto bail; \
		} \
		memset(entry, 0, sizeof(struct entry)); \
		TAILQ_INIT(&entry->childs); \
		entry->type = entry_type_vector; \
		entry->parent = parent; \
		entry->parent_element = element; \
		entry->parent_offset = offset; \
		entry->u.vector.type = vector_type_ ## __type__; \
		entry->u.vector.elements = 0; \
		rc = offset_table_init(&encoder->pool.offset, &entry->u.vector.offset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not init table present"); \
			goto bail; \
		} \
		entry->offset = encoder->emitter.offset; \
		encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, sizeof(uint64_t)); \
		encoder->emitter.offset += sizeof(uint64_t); \
		TAILQ_INSERT_TAIL(&parent->childs, entry, child); \
		TAILQ_INSERT_TAIL(&encoder->stack, entry, stack); \
		return 0; \
	bail:	if (entry != NULL) { \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry); \
		} \
		return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end_ ## __type__ (struct linearbuffers_encoder *encoder) \
	{ \
		struct entry *entry; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		entry = TAILQ_LAST(&encoder->stack, entries); \
		if (entry == NULL) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->type != entry_type_vector) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->u.vector.type != vector_type_ ## __type__) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		encoder->emitter.function(encoder->emitter.context, entry->offset, &entry->u.vector.elements, sizeof(uint64_t)); \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		if (entry->parent != NULL) { \
			present_table_mark(&entry->parent->u.table.present, entry->parent_element); \
			encoder->emitter.function(encoder->emitter.context, entry->parent->offset + entry->parent->u.table.present.bytes + entry->parent_offset, &entry->offset, sizeof(uint64_t)); \
			TAILQ_REMOVE(&entry->parent->childs, entry, child); \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry); \
		} else { \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry); \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			encoder->root = NULL; \
		} \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_cancel_ ## __type__ (struct linearbuffers_encoder *encoder) \
	{ \
		struct entry *entry; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		entry = TAILQ_LAST(&encoder->stack, entries); \
		if (entry == NULL) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->type != entry_type_vector) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->u.vector.type != vector_type_ ## __type__) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset); \
		encoder->emitter.offset = entry->offset; \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		if (entry->parent != NULL) { \
			TAILQ_REMOVE(&entry->parent->childs, entry, child); \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry); \
		} else { \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry); \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			encoder->root = NULL; \
		} \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_push_ ## __type__ (struct linearbuffers_encoder *encoder, __type__ ## _t value) \
	{ \
		struct entry *entry; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		entry = TAILQ_LAST(&encoder->stack, entries); \
		if (entry == NULL) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->type != entry_type_vector) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		if (entry->u.vector.type != vector_type_ ## __type__) { \
			linearbuffers_errorf("logic error: entry is invalid"); \
			goto bail; \
		} \
		entry->u.vector.elements += 1; \
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, &value, sizeof(__type__ ## _t)); \
		encoder->emitter.offset += sizeof(__type__ ## _t); \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_vector_start_scalar_type(int8);
linearbuffers_encoder_vector_start_scalar_type(int16);
linearbuffers_encoder_vector_start_scalar_type(int32);
linearbuffers_encoder_vector_start_scalar_type(int64);
linearbuffers_encoder_vector_start_scalar_type(uint8);
linearbuffers_encoder_vector_start_scalar_type(uint16);
linearbuffers_encoder_vector_start_scalar_type(uint32);
linearbuffers_encoder_vector_start_scalar_type(uint64);

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset)
{
	int rc;
	struct entry *entry;
	struct entry *parent;
	entry = NULL;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_errorf("logic error: element is invalid");
		goto bail;
	}
	entry = pool_malloc(&encoder->pool.entry);
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_vector;
	entry->parent = parent;
	entry->parent_element = element;
	entry->parent_offset = offset;
	entry->u.vector.type = vector_type_string;
	entry->u.vector.elements = 0;
	rc = offset_table_init(&encoder->pool.offset, &entry->u.vector.offset);
	if (rc != 0) {
		linearbuffers_errorf("can not init table present");
		goto bail;
	}
	entry->offset = encoder->emitter.offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, sizeof(uint64_t) + sizeof(uint64_t));
	encoder->emitter.offset += sizeof(uint64_t) + sizeof(uint64_t);
	TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	if (entry != NULL) {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end_string (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	uint64_t i;
	uint64_t eoffset;
	uint64_t offset_bytes;
	uint64_t offset_emitter;
	struct offset_buffer *offset_buffer;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_vector) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->u.vector.type != vector_type_string) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	encoder->emitter.function(encoder->emitter.context, entry->offset, &entry->u.vector.elements, sizeof(uint64_t));
	offset_emitter = encoder->emitter.offset - entry->offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset + sizeof(uint64_t), &offset_emitter, sizeof(uint64_t));
	for (offset_bytes = entry->u.vector.elements, offset_buffer = entry->u.vector.offset.buffers;
	     offset_buffer;
	     offset_bytes -= OFFSET_BUFFER_COUNT    , offset_buffer = offset_buffer->next) {
		for (i = 0; i < MIN(offset_bytes, OFFSET_BUFFER_COUNT); i++) {
			offset_buffer->buffer[i] = offset_buffer->buffer[i] - (offset_emitter + entry->offset);
		}
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, offset_buffer->buffer, MIN(offset_bytes, OFFSET_BUFFER_COUNT) * sizeof(uint64_t));
		encoder->emitter.offset += MIN(offset_bytes, OFFSET_BUFFER_COUNT) * sizeof(uint64_t);
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		present_table_mark(&entry->parent->u.table.present, entry->parent_element);
		eoffset = entry->offset - entry->parent->offset;
		encoder->emitter.function(encoder->emitter.context, entry->parent->offset + entry->parent->u.table.present.bytes + entry->parent_offset, &eoffset, sizeof(uint64_t));
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_cancel_string (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_vector) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->u.vector.type != vector_type_string) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset);
	encoder->emitter.offset = entry->offset;
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_push_string (struct linearbuffers_encoder *encoder, const char *value)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_vector) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->u.vector.type != vector_type_string) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	entry->u.vector.elements += 1;
	offset_table_push(&entry->u.vector.offset, encoder->emitter.offset, &encoder->pool.offset);
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, strlen(value) + 1);
	encoder->emitter.offset += strlen(value) + 1;
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start_table (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset)
{
	int rc;
	struct entry *entry;
	struct entry *parent;
	entry = NULL;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (element >= parent->u.table.elements) {
		linearbuffers_errorf("logic error: element is invalid");
		goto bail;
	}
	entry = pool_malloc(&encoder->pool.entry);
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	TAILQ_INIT(&entry->childs);
	entry->type = entry_type_vector;
	entry->parent = parent;
	entry->parent_element = element;
	entry->parent_offset = offset;
	entry->u.vector.type = vector_type_table;
	entry->u.vector.elements = 0;
	rc = offset_table_init(&encoder->pool.offset, &entry->u.vector.offset);
	if (rc != 0) {
		linearbuffers_errorf("can not init table present");
		goto bail;
	}
	entry->offset = encoder->emitter.offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, sizeof(uint64_t) + sizeof(uint64_t));
	encoder->emitter.offset += sizeof(uint64_t) + sizeof(uint64_t);
	TAILQ_INSERT_TAIL(&parent->childs, entry, child);
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	if (entry != NULL) {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end_table (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	uint64_t i;
	uint64_t eoffset;
	uint64_t offset_bytes;
	uint64_t offset_emitter;
	struct offset_buffer *offset_buffer;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_vector) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->u.vector.type != vector_type_table) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	encoder->emitter.function(encoder->emitter.context, entry->offset, &entry->u.vector.elements, sizeof(uint64_t));
	offset_emitter = encoder->emitter.offset - entry->offset;
	encoder->emitter.function(encoder->emitter.context, entry->offset + sizeof(uint64_t), &offset_emitter, sizeof(uint64_t));
	for (offset_bytes = entry->u.vector.elements, offset_buffer = entry->u.vector.offset.buffers;
	     offset_buffer;
	     offset_bytes -= OFFSET_BUFFER_COUNT    , offset_buffer = offset_buffer->next) {
		for (i = 0; i < MIN(offset_bytes, OFFSET_BUFFER_COUNT); i++) {
			offset_buffer->buffer[i] = offset_buffer->buffer[i] - (offset_emitter + entry->offset);
		}
		encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, offset_buffer->buffer, MIN(offset_bytes, OFFSET_BUFFER_COUNT) * sizeof(uint64_t));
		encoder->emitter.offset += MIN(offset_bytes, OFFSET_BUFFER_COUNT) * sizeof(uint64_t);
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		present_table_mark(&entry->parent->u.table.present, entry->parent_element);
		eoffset = entry->offset - entry->parent->offset;
		encoder->emitter.function(encoder->emitter.context, entry->parent->offset + entry->parent->u.table.present.bytes + entry->parent_offset, &eoffset, sizeof(uint64_t));
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_cancel_table (struct linearbuffers_encoder *encoder)
{
	struct entry *entry;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&encoder->stack, entries);
	if (entry == NULL) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->type != entry_type_vector) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	if (entry->u.vector.type != vector_type_table) {
		linearbuffers_errorf("logic error: entry is invalid");
		goto bail;
	}
	encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset);
	encoder->emitter.offset = entry->offset;
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	if (entry->parent != NULL) {
		TAILQ_REMOVE(&entry->parent->childs, entry, child);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	} else {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, entry);
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		encoder->root = NULL;
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_push_table (struct linearbuffers_encoder *encoder, uint64_t value)
{
	struct entry *parent;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&encoder->stack)) {
		linearbuffers_errorf("logic error: stack is empty");
		goto bail;
	}
	parent = TAILQ_LAST(&encoder->stack, entries);
	if (parent == NULL) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->type != entry_type_vector) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	if (parent->u.vector.type != vector_type_table) {
		linearbuffers_errorf("logic error: parent is invalid");
		goto bail;
	}
	parent->u.vector.elements += 1;
	offset_table_push(&parent->u.vector.offset, value, &encoder->pool.offset);
	return 0;
bail:	return -1;
}
