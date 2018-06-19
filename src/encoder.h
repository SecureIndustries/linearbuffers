
#if !defined(LINEARBUFFERS_ENCODER_H)
#define LINEARBUFFERS_ENCODER_H

struct linearbuffers_encoder;

struct linearbuffers_encoder_create_options {
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, uint64_t length);
		void *context;
	} emitter;
};

struct linearbuffers_encoder_reset_options {
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, uint64_t length);
		void *context;
	} emitter;
};

struct linearbuffers_encoder * linearbuffers_encoder_create (struct linearbuffers_encoder_create_options *options);
void linearbuffers_encoder_destroy (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder, struct linearbuffers_encoder_reset_options *options);

int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t elements, uint64_t size);
int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_table_set_int8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int8_t value);
int linearbuffers_encoder_table_set_int16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int16_t value);
int linearbuffers_encoder_table_set_int32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int32_t value);
int linearbuffers_encoder_table_set_int64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int64_t value);

int linearbuffers_encoder_table_set_uint8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint8_t value);
int linearbuffers_encoder_table_set_uint16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint16_t value);
int linearbuffers_encoder_table_set_uint32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint32_t value);
int linearbuffers_encoder_table_set_uint64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value);

int linearbuffers_encoder_table_set_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value);

int linearbuffers_encoder_table_set_vector_int8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const int8_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const int16_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const int32_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const int64_t *value, uint64_t count);

int linearbuffers_encoder_table_set_vector_uint8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const uint8_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const uint16_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const uint32_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const uint64_t *value, uint64_t count);

int linearbuffers_encoder_table_set_vector_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char **value, uint64_t count);

int linearbuffers_encoder_vector_start (struct linearbuffers_encoder *encoder, uint64_t element);
int linearbuffers_encoder_vector_end (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_set_int8 (struct linearbuffers_encoder *encoder, uint64_t at, int8_t value);
int linearbuffers_encoder_vector_set_int16 (struct linearbuffers_encoder *encoder, uint64_t at, int16_t value);
int linearbuffers_encoder_vector_set_int32 (struct linearbuffers_encoder *encoder, uint64_t at, int32_t value);
int linearbuffers_encoder_vector_set_int64 (struct linearbuffers_encoder *encoder, uint64_t at, int64_t value);

int linearbuffers_encoder_vector_set_uint8 (struct linearbuffers_encoder *encoder, uint64_t at, uint8_t value);
int linearbuffers_encoder_vector_set_uint16 (struct linearbuffers_encoder *encoder, uint64_t at, uint16_t value);
int linearbuffers_encoder_vector_set_uint32 (struct linearbuffers_encoder *encoder, uint64_t at, uint32_t value);
int linearbuffers_encoder_vector_set_uint64 (struct linearbuffers_encoder *encoder, uint64_t at, uint64_t value);

int linearbuffers_encoder_vector_set_string (struct linearbuffers_encoder *encoder, uint64_t at, const char *value);

int linearbuffers_encoder_vector_push_int8 (struct linearbuffers_encoder *encoder, int8_t value);
int linearbuffers_encoder_vector_push_int16 (struct linearbuffers_encoder *encoder, int16_t value);
int linearbuffers_encoder_vector_push_int32 (struct linearbuffers_encoder *encoder, int32_t value);
int linearbuffers_encoder_vector_push_int64 (struct linearbuffers_encoder *encoder, int64_t value);

int linearbuffers_encoder_vector_push_uint8 (struct linearbuffers_encoder *encoder, uint8_t value);
int linearbuffers_encoder_vector_push_uint16 (struct linearbuffers_encoder *encoder, uint16_t value);
int linearbuffers_encoder_vector_push_uint32 (struct linearbuffers_encoder *encoder, uint32_t value);
int linearbuffers_encoder_vector_push_uint64 (struct linearbuffers_encoder *encoder, uint64_t value);

int linearbuffers_encoder_vector_push_string (struct linearbuffers_encoder *encoder, const char *value);

const void * linearbuffers_encoder_linearized (struct linearbuffers_encoder *encoder, uint64_t *length);

#endif
