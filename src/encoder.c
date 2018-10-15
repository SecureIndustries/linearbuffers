
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
	const char *name;
	uint64_t selements;
	uint64_t nelements;
	uint64_t uelements;
	struct pool_element *felements;
	struct pool_block *cblock;
	struct pool_block *blocks;
};

static int pool_init (struct pool *pool, const char *name, uint64_t selements, uint64_t nelements);
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

static int pool_init (struct pool *pool, const char *name, uint64_t selements, uint64_t nelements)
{
	memset(pool, 0, sizeof(struct pool));
	linearbuffers_debugf("init name: %s, selements: %llu, nelements: %llu", name, selements, nelements);
	pool->name = name;
	pool->selements = selements;
	pool->nelements = nelements;
	pool->felements = NULL;
	pool->cblock = NULL;
	pool->blocks = NULL;
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
	linearbuffers_debugf("uelements: %lld, nelements: %lld", pool->uelements, pool->nelements);
	if (pool->uelements + 1 > pool->nelements ||
	    pool->cblock == NULL) {
		struct pool_block *block;
		struct pool_block *blocks;
		linearbuffers_debugf("pool(%s): creating new block", pool->name);
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

#define OFFSET_BUFFER_64_COUNT	(64)
#define OFFSET_BUFFER_32_COUNT	(OFFSET_BUFFER_64_COUNT * (sizeof(uint64_t) / sizeof(uint32_t)))
#define OFFSET_BUFFER_16_COUNT	(OFFSET_BUFFER_32_COUNT * (sizeof(uint32_t) / sizeof(uint16_t)))
#define OFFSET_BUFFER_8_COUNT	(OFFSET_BUFFER_16_COUNT * (sizeof(uint16_t) / sizeof(uint8_t)))
struct offset_buffer {
	struct offset_buffer *next;
	union {
		uint64_t _64[OFFSET_BUFFER_64_COUNT];
		uint32_t _32[OFFSET_BUFFER_32_COUNT];
		uint16_t _16[OFFSET_BUFFER_16_COUNT];
		uint8_t   _8[OFFSET_BUFFER_8_COUNT];
	} u;
};

struct offset_table {
	uint64_t count;
	struct offset_buffer *cbuffer;
	struct offset_buffer *buffers;
	int (*push) (struct offset_table *offset, uint64_t value, struct pool *pool);
	int (*emit) (struct offset_table *table, int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t *offset, uint64_t diff);
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
	vector_type_float,
	vector_type_double,
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
	TAILQ_ENTRY(entry) stack;
	enum entry_type type;
	union {
		struct entry_table table;
		struct entry_vector vector;
	} u;
	uint64_t count_size;
	int (*count_emitter) (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value);
	uint64_t offset_size;
	int (*offset_emitter) (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value);
	uint64_t offset;
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

static int encoder_uint8_emitter (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value)
{
	uint8_t uint8;
	uint8 = value;
	return function(context, offset, &uint8, sizeof(uint8));
}

static int encoder_uint16_emitter (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value)
{
	uint16_t uint16;
	uint16 = value;
	return function(context, offset, &uint16, sizeof(uint16));
}

static int encoder_uint32_emitter (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value)
{
	uint32_t uint32;
	uint32 = value;
	return function(context, offset, &uint32, sizeof(uint32));
}

static int encoder_uint64_emitter (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value)
{
	return function(context, offset, &value, sizeof(value));
}

#define offset_table_push_type(__type__) \
	static int offset_table_push_ ## __type__  (struct offset_table *offset, uint64_t value, struct pool *pool) \
	{ \
		if (offset->count + 1 > OFFSET_BUFFER_ ## __type__ ## _COUNT || \
		    offset->cbuffer == NULL) { \
			struct offset_buffer *buffer; \
			buffer = pool_malloc(pool); \
			if (buffer == NULL) { \
				linearbuffers_errorf("can not allocate memory"); \
				goto bail; \
			} \
			memset(buffer, 0, sizeof(struct offset_buffer)); \
			if (offset->cbuffer == NULL) { \
				offset->buffers = buffer; \
			} else { \
				offset->cbuffer->next = buffer; \
			} \
			offset->cbuffer = buffer; \
			offset->count = 0; \
		} \
		offset->cbuffer->u._ ## __type__[offset->count] = value; \
		offset->count += 1; \
		return 0; \
	bail:	return -1; \
	} \
	\
	static int offset_table_emit_ ## __type__ (struct offset_table *table, int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t *offset, uint64_t diff) \
	{ \
		int rc; \
		uint64_t i; \
		uint64_t c; \
		uint64_t o; \
		struct offset_buffer *buffer; \
		struct offset_buffer *nbuffer; \
		o = *offset; \
		for (buffer = table->buffers; \
		     buffer && (nbuffer = buffer->next, 1); \
		     buffer = buffer->next) { \
			c = (nbuffer == NULL) ? table->count : OFFSET_BUFFER_ ## __type__ ## _COUNT; \
			for (i = 0; i < c; i++) { \
				buffer->u._ ## __type__ [i] -= (uint ## __type__ ## _t) diff; \
			} \
			rc = function(context, o, buffer->u._ ## __type__, c * sizeof(uint ## __type__ ## _t)); \
			if (rc != 0) { \
				return -1; \
			} \
			o += c * sizeof(uint ## __type__ ## _t); \
		} \
		*offset = o; \
		return 0; \
	}

offset_table_push_type(64)
offset_table_push_type(32)
offset_table_push_type(16)
offset_table_push_type(8)

static const struct {
	const char *name;
	uint64_t value;
	uint64_t size;
	int (*emitter) (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value);
} linearbuffers_encoder_count_types[] = {
	[linearbuffers_encoder_count_type_uint8]   = { "uint8" , linearbuffers_encoder_count_type_uint8 , sizeof(uint8_t) , encoder_uint8_emitter  },
	[linearbuffers_encoder_count_type_uint16]  = { "uint16", linearbuffers_encoder_count_type_uint16, sizeof(uint16_t), encoder_uint16_emitter },
	[linearbuffers_encoder_count_type_uint32]  = { "uint32", linearbuffers_encoder_count_type_uint32, sizeof(uint32_t), encoder_uint32_emitter },
	[linearbuffers_encoder_count_type_uint64]  = { "uint64", linearbuffers_encoder_count_type_uint64, sizeof(uint64_t), encoder_uint64_emitter }
};

static const struct {
	const char *name;
	uint64_t value;
	uint64_t size;
	int (*emitter) (int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t offset, uint64_t value);
	int (*offset_table_push) (struct offset_table *offset, uint64_t value, struct pool *pool);
	int (*offset_table_emit) (struct offset_table *table, int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t *offset, uint64_t diff);
} linearbuffers_encoder_offset_types[] = {
	[linearbuffers_encoder_offset_type_uint8]   = { "uint8" , linearbuffers_encoder_offset_type_uint8 , sizeof(uint8_t) , encoder_uint8_emitter , offset_table_push_8 , offset_table_emit_8  },
	[linearbuffers_encoder_offset_type_uint16]  = { "uint16", linearbuffers_encoder_offset_type_uint16, sizeof(uint16_t), encoder_uint16_emitter, offset_table_push_16, offset_table_emit_16 },
	[linearbuffers_encoder_offset_type_uint32]  = { "uint32", linearbuffers_encoder_offset_type_uint32, sizeof(uint32_t), encoder_uint32_emitter, offset_table_push_32, offset_table_emit_32 },
	[linearbuffers_encoder_offset_type_uint64]  = { "uint64", linearbuffers_encoder_offset_type_uint64, sizeof(uint64_t), encoder_uint64_emitter, offset_table_push_64, offset_table_emit_64 },
};

static int offset_table_push (struct offset_table *table, uint64_t value, struct pool *pool)
{
	return table->push(table, value, pool);
}

static uint64_t offset_table_emit (struct offset_table *table, int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length), void *context, uint64_t *offset, uint64_t diff)
{
	return table->emit(table, function, context, offset, diff);
}

static void offset_table_uninit (struct pool *pool, struct offset_table *table)
{
	struct offset_buffer *buffer;
	struct offset_buffer *nbuffer;
	for (buffer = table->buffers; buffer && ((nbuffer = buffer->next), 1); buffer = nbuffer) {
		pool_free(pool, buffer);
	}
	memset(table, 0, sizeof(struct offset_table));
}

static int offset_table_init (struct pool *pool, struct offset_table *table, enum linearbuffers_encoder_offset_type type)
{
	(void) pool;
	memset(table, 0, sizeof(struct offset_table));
	table->push = linearbuffers_encoder_offset_types[type].offset_table_push;
	table->emit = linearbuffers_encoder_offset_types[type].offset_table_emit;
	return 0;
}

static int present_table_mark (struct present_table *table, uint64_t element)
{
	uint64_t byte;
	uint64_t buffer;
	struct present_buffer *present_buffer;
	byte = element / 8;
	buffer = byte / PRESENT_BUFFER_COUNT;
	present_buffer = table->buffers;
	while (buffer--) {
		present_buffer = present_buffer->next;
	}
	present_buffer->buffer[byte % PRESENT_BUFFER_COUNT] |= (1 << (element % 8));
	return 0;
}

static void present_table_uninit (struct pool *pool, struct present_table *table)
{
	struct present_buffer *buffer;
	struct present_buffer *nbuffer;
	for (buffer = table->buffers; buffer && ((nbuffer = buffer->next), 1); buffer = nbuffer) {
		pool_free(pool, buffer);
	}
	memset(table, 0, sizeof(struct present_table));
}

static int present_table_init (struct pool *pool, struct present_table *table, uint64_t elements)
{
	uint64_t buffers;
	struct present_buffer *present_buffer;
	struct present_buffer *present_buffers;
	memset(table, 0, sizeof(struct present_table));
	table->bytes = sizeof(uint8_t) * ((elements + 7) / 8);
	buffers = (table->bytes + PRESENT_BUFFER_COUNT - 1) / PRESENT_BUFFER_COUNT;
	while (buffers--) {
		present_buffer = pool_malloc(pool);
		if (present_buffer == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
		memset(present_buffer->buffer, 0, PRESENT_BUFFER_COUNT);
		present_buffers = table->buffers;
		table->buffers = present_buffer;
		table->buffers->next = present_buffers;
	}
	return 0;
bail:	present_table_uninit(pool, table);
	return -1;
}

static void entry_destroy (struct pool *epool, struct pool *ppool, struct pool *opool, struct entry *entry)
{
	if (entry == NULL) {
		return;
	}
	if (entry->type == entry_type_table) {
		present_table_uninit(ppool, &entry->u.table.present);
	} else if (entry->type == entry_type_vector) {
		offset_table_uninit(opool, &entry->u.vector.offset);
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
	pool_init(&encoder->pool.entry, "entry", sizeof(struct entry), 8);
	pool_init(&encoder->pool.present, "present", sizeof(struct present_buffer), 8);
	pool_init(&encoder->pool.offset, "offset", sizeof(struct offset_buffer), 8);
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
	struct entry *entry;
	struct entry *nentry;
	if (encoder == NULL) {
		return;
	}
	TAILQ_FOREACH_REVERSE_SAFE(entry, &encoder->stack, entries, stack, nentry) {
		TAILQ_REMOVE(&encoder->stack, entry, stack);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, encoder->root);
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
	struct entry *entry;
	struct entry *nentry;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	TAILQ_FOREACH_REVERSE_SAFE(entry, &encoder->stack, entries, stack, nentry) {
		TAILQ_REMOVE(&encoder->stack, entry, stack);
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, encoder->root);
	}
	encoder->emitter.offset = 0;
	encoder->emitter.function = encoder_default_emitter;
	encoder->emitter.context = encoder;
	encoder->output.length = 0;
	if (options != NULL) {
		if (options->emitter.function != NULL) {
			encoder->emitter.function = options->emitter.function;
			encoder->emitter.context = options->emitter.context;
		}
	}
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type, enum linearbuffers_encoder_offset_type offset_type, uint64_t elements, uint64_t size)
{
	int rc;
	struct entry *entry;
	entry = NULL;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (elements == 0) {
		linearbuffers_errorf("elements is invalid");
		goto bail;
	}
	entry = pool_malloc(&encoder->pool.entry);
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct entry));
	entry->type = entry_type_table;
	entry->count_size = linearbuffers_encoder_count_types[count_type].size;
	entry->count_emitter = linearbuffers_encoder_count_types[count_type].emitter;
	entry->offset_size = linearbuffers_encoder_offset_types[offset_type].size;
	entry->offset_emitter = linearbuffers_encoder_offset_types[offset_type].emitter;
	entry->u.table.elements = elements;
	entry->offset = encoder->emitter.offset;
	rc = present_table_init(&encoder->pool.present, &entry->u.table.present, elements);
	if (rc != 0) {
		linearbuffers_errorf("can not init table present");
		goto bail;
	}
	rc = encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, entry->count_size + entry->u.table.present.bytes + size);
	if (rc != 0) {
		linearbuffers_errorf("can not emit table space");
		goto bail;
	}
	encoder->emitter.offset += entry->count_size + entry->u.table.present.bytes + size;
	TAILQ_INSERT_TAIL(&encoder->stack, entry, stack);
	return 0;
bail:	if (entry != NULL) {
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry);
	}
	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder, uint64_t *offset)
{
	int rc;
	struct entry *entry;
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
	rc = entry->count_emitter(encoder->emitter.function, encoder->emitter.context, entry->offset, entry->u.table.elements);
	if (rc != 0) {
		linearbuffers_errorf("can not emit table count");
		goto bail;
	}
	for (present_bufferi = 0 , present_bytes = entry->u.table.present.bytes, present_buffer = entry->u.table.present.buffers;
	     present_buffer;
	     present_bufferi += 1, present_bytes -= PRESENT_BUFFER_COUNT       , present_buffer = present_buffer->next) {
		rc = encoder->emitter.function(encoder->emitter.context, entry->offset + entry->count_size + present_bufferi * PRESENT_BUFFER_COUNT, present_buffer->buffer, MIN(present_bytes, PRESENT_BUFFER_COUNT));
		if (rc != 0) {
			linearbuffers_errorf("can not emit table present");
			goto bail;
		}
	}
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry);
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_cancel (struct linearbuffers_encoder *encoder)
{
	int rc;
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
	rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset);
	if (rc != 0) {
		linearbuffers_errorf("can not emit table cancel");
		goto bail;
	}
	encoder->emitter.offset = entry->offset;
	TAILQ_REMOVE(&encoder->stack, entry, stack);
	entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry);
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_table_set_scalar_type(__type__, __type_t__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, __type_t__ value) \
	{ \
		int rc; \
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
		rc = encoder->emitter.function(encoder->emitter.context, parent->offset + parent->count_size + parent->u.table.present.bytes + offset, &value, sizeof(__type_t__)); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit table element"); \
			goto bail; \
		} \
		rc = present_table_mark(&parent->u.table.present, element); \
		if (rc != 0) { \
			linearbuffers_errorf("can not mark table element"); \
			goto bail; \
		} \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_table_set_scalar_type(int8, int8_t);
linearbuffers_encoder_table_set_scalar_type(int16, int16_t);
linearbuffers_encoder_table_set_scalar_type(int32, int32_t);
linearbuffers_encoder_table_set_scalar_type(int64, int64_t);

linearbuffers_encoder_table_set_scalar_type(uint8, uint8_t);
linearbuffers_encoder_table_set_scalar_type(uint16, uint16_t);
linearbuffers_encoder_table_set_scalar_type(uint32, uint32_t);
linearbuffers_encoder_table_set_scalar_type(uint64, uint64_t);

linearbuffers_encoder_table_set_scalar_type(float, float);
linearbuffers_encoder_table_set_scalar_type(double, double);

#define linearbuffers_encoder_table_set_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_table_set_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value) \
	{ \
		int rc; \
		uint64_t eoffset; \
		struct entry *parent; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (value <= 0) { \
			linearbuffers_errorf("value is invalid"); \
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
		eoffset = value - parent->offset; \
		rc = parent->offset_emitter(encoder->emitter.function, encoder->emitter.context, parent->offset + parent->count_size + parent->u.table.present.bytes + offset, eoffset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit table element offset"); \
			goto bail; \
		} \
		rc = present_table_mark(&parent->u.table.present, element); \
		if (rc != 0) { \
			linearbuffers_errorf("can not mark table element"); \
			goto bail; \
		} \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_table_set_type(string);
linearbuffers_encoder_table_set_type(table);
linearbuffers_encoder_table_set_type(vector);

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_string_create (struct linearbuffers_encoder *encoder, uint64_t *offset, const char *value)
{
	int rc;
	uint64_t length;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (offset == NULL) {
		linearbuffers_errorf("offset is invalid");
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
	*offset = encoder->emitter.offset;
	length = strlen(value) + 1;
	rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, length);
	if (rc != 0) {
		linearbuffers_errorf("can not emit element");
		goto bail;
	}
	encoder->emitter.offset += length;
	return 0;
bail:	return -1;
}

__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_string_ncreate (struct linearbuffers_encoder *encoder, uint64_t *offset, uint64_t n, const char *value)
{
	const char _null = 0;
	int rc;
	if (encoder == NULL) {
		linearbuffers_errorf("encoder is invalid");
		goto bail;
	}
	if (offset == NULL) {
		linearbuffers_errorf("offset is invalid");
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
	*offset = encoder->emitter.offset;
	rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, value, n);
	if (rc != 0) {
		linearbuffers_errorf("can not emit element");
		goto bail;
	}
	rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset + n, &_null, 1);
	if (rc != 0) {
		linearbuffers_errorf("can not emit element");
		goto bail;
	}
	encoder->emitter.offset += n + 1;
	return 0;
bail:	return -1;
}

#define linearbuffers_encoder_vector_start_scalar_type(__type__, __type_t__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_create_ ## __type__ (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type, enum linearbuffers_encoder_offset_type offset_type, uint64_t *offset, const __type_t__ *value, uint64_t count) \
	{ \
		int rc; \
		(void) offset_type; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (offset == NULL) { \
			linearbuffers_errorf("offset is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		*offset = encoder->emitter.offset; \
		rc = linearbuffers_encoder_count_types[count_type].emitter(encoder->emitter.function, encoder->emitter.context, encoder->emitter.offset, count); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector count"); \
			goto bail; \
		} \
		rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset + linearbuffers_encoder_count_types[count_type].size, value, count * sizeof(__type_t__)); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector values"); \
			goto bail; \
		} \
		encoder->emitter.offset += linearbuffers_encoder_count_types[count_type].size; \
		encoder->emitter.offset += count * sizeof(__type_t__); \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start_ ## __type__ (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type, enum linearbuffers_encoder_offset_type offset_type) \
	{ \
		int rc; \
		struct entry *entry; \
		entry = NULL; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		entry = pool_malloc(&encoder->pool.entry); \
		if (entry == NULL) { \
			linearbuffers_errorf("can not allocate memory"); \
			goto bail; \
		} \
		memset(entry, 0, sizeof(struct entry)); \
		entry->type = entry_type_vector; \
		entry->u.vector.type = vector_type_ ## __type__; \
		entry->u.vector.elements = 0; \
		entry->count_size = linearbuffers_encoder_count_types[count_type].size; \
		entry->count_emitter = linearbuffers_encoder_count_types[count_type].emitter; \
		entry->offset_size = linearbuffers_encoder_offset_types[offset_type].size; \
		entry->offset_emitter = linearbuffers_encoder_offset_types[offset_type].emitter; \
		rc = offset_table_init(&encoder->pool.offset, &entry->u.vector.offset, offset_type); \
		if (rc != 0) { \
			linearbuffers_errorf("can not init table present"); \
			goto bail; \
		} \
		entry->offset = encoder->emitter.offset; \
		rc = encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, entry->count_size); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector place"); \
			goto bail; \
		} \
		encoder->emitter.offset += entry->count_size; \
		TAILQ_INSERT_TAIL(&encoder->stack, entry, stack); \
		return 0; \
	bail:	if (entry != NULL) { \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		} \
		return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t *offset) \
	{ \
		int rc; \
		struct entry *entry; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (offset == NULL) { \
			linearbuffers_errorf("offset is invalid"); \
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
		*offset = entry->offset; \
		rc = entry->count_emitter(encoder->emitter.function, encoder->emitter.context, entry->offset, entry->u.vector.elements); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector count"); \
			goto bail; \
		} \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_cancel_ ## __type__ (struct linearbuffers_encoder *encoder) \
	{ \
		int rc; \
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
		rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector cancel"); \
			goto bail; \
		} \
		encoder->emitter.offset = entry->offset; \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_push_ ## __type__ (struct linearbuffers_encoder *encoder, __type_t__ value) \
	{ \
		int rc; \
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
		rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, &value, sizeof(__type_t__)); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector element"); \
			goto bail; \
		} \
		entry->u.vector.elements += 1; \
		encoder->emitter.offset += sizeof(__type_t__); \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_vector_start_scalar_type(int8, int8_t);
linearbuffers_encoder_vector_start_scalar_type(int16, int16_t);
linearbuffers_encoder_vector_start_scalar_type(int32, int32_t);
linearbuffers_encoder_vector_start_scalar_type(int64, int64_t);

linearbuffers_encoder_vector_start_scalar_type(uint8, uint8_t);
linearbuffers_encoder_vector_start_scalar_type(uint16, uint16_t);
linearbuffers_encoder_vector_start_scalar_type(uint32, uint32_t);
linearbuffers_encoder_vector_start_scalar_type(uint64, uint64_t);

linearbuffers_encoder_vector_start_scalar_type(float, float);
linearbuffers_encoder_vector_start_scalar_type(double, double);

#define linearbuffers_encoder_vector_start_type(__type__) \
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_start_ ## __type__ (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type, enum linearbuffers_encoder_offset_type offset_type) \
	{ \
		int rc; \
		struct entry *entry; \
		entry = NULL; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (TAILQ_EMPTY(&encoder->stack)) { \
			linearbuffers_errorf("logic error: stack is empty"); \
			goto bail; \
		} \
		entry = pool_malloc(&encoder->pool.entry); \
		if (entry == NULL) { \
			linearbuffers_errorf("can not allocate memory"); \
			goto bail; \
		} \
		memset(entry, 0, sizeof(struct entry)); \
		entry->type = entry_type_vector; \
		entry->u.vector.type = vector_type_ ## __type__; \
		entry->u.vector.elements = 0; \
		entry->count_size = linearbuffers_encoder_count_types[count_type].size; \
		entry->count_emitter = linearbuffers_encoder_count_types[count_type].emitter; \
		entry->offset_size = linearbuffers_encoder_offset_types[offset_type].size; \
		entry->offset_emitter = linearbuffers_encoder_offset_types[offset_type].emitter; \
		rc = offset_table_init(&encoder->pool.offset, &entry->u.vector.offset, offset_type); \
		if (rc != 0) { \
			linearbuffers_errorf("can not init table present"); \
			goto bail; \
		} \
		entry->offset = encoder->emitter.offset; \
		rc = encoder->emitter.function(encoder->emitter.context, entry->offset, NULL, entry->count_size + entry->offset_size); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector place"); \
			goto bail; \
		} \
		encoder->emitter.offset += entry->count_size + entry->offset_size; \
		TAILQ_INSERT_TAIL(&encoder->stack, entry, stack); \
		return 0; \
	bail:	if (entry != NULL) { \
			entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		} \
		return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_end_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t *offset) \
	{ \
		int rc; \
		struct entry *entry; \
		uint64_t offset_table; \
		if (encoder == NULL) { \
			linearbuffers_errorf("encoder is invalid"); \
			goto bail; \
		} \
		if (offset == NULL) { \
			linearbuffers_errorf("offset is invalid"); \
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
		*offset = entry->offset; \
		offset_table = encoder->emitter.offset - entry->offset; \
		rc = entry->count_emitter(encoder->emitter.function, encoder->emitter.context, entry->offset, entry->u.vector.elements); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector count"); \
			goto bail; \
		} \
		rc = entry->offset_emitter(encoder->emitter.function, encoder->emitter.context, entry->offset + entry->count_size, offset_table); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector offset"); \
			goto bail; \
		} \
		rc = offset_table_emit(&entry->u.vector.offset, encoder->emitter.function, encoder->emitter.context, &encoder->emitter.offset, offset_table + entry->offset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit offset table"); \
			goto bail; \
		} \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_cancel_ ## __type__ (struct linearbuffers_encoder *encoder) \
	{ \
		int rc; \
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
		rc = encoder->emitter.function(encoder->emitter.context, encoder->emitter.offset, NULL, entry->offset - encoder->emitter.offset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not emit vector cancel"); \
			goto bail; \
		} \
		encoder->emitter.offset = entry->offset; \
		TAILQ_REMOVE(&encoder->stack, entry, stack); \
		entry_destroy(&encoder->pool.entry, &encoder->pool.present, &encoder->pool.offset, entry); \
		return 0; \
	bail:	return -1; \
	} \
	\
	__attribute__ ((__visibility__("default"))) int linearbuffers_encoder_vector_push_ ## __type__ (struct linearbuffers_encoder *encoder, uint64_t value) \
	{ \
		int rc; \
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
		rc = offset_table_push(&entry->u.vector.offset, value, &encoder->pool.offset); \
		if (rc != 0) { \
			linearbuffers_errorf("can not push element offset"); \
			goto bail; \
		} \
		entry->u.vector.elements += 1; \
		return 0; \
	bail:	return -1; \
	}

linearbuffers_encoder_vector_start_type(string);
linearbuffers_encoder_vector_start_type(table);
