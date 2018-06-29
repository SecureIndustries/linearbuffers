
#if !defined(LINEARBUFFERS_ENCODER_H)
#define LINEARBUFFERS_ENCODER_H

struct linearbuffers_encoder;

enum linearbuffers_encoder_count_type {
	linearbuffers_encoder_count_type_uint8,
	linearbuffers_encoder_count_type_uint16,
	linearbuffers_encoder_count_type_uint32,
	linearbuffers_encoder_count_type_uint64
};

enum linearbuffers_encoder_offset_type {
	linearbuffers_encoder_offset_type_uint8,
	linearbuffers_encoder_offset_type_uint16,
	linearbuffers_encoder_offset_type_uint32,
	linearbuffers_encoder_offset_type_uint64
};

struct linearbuffers_encoder_create_options {
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length);
		void *context;
	} emitter;
};

struct linearbuffers_encoder_reset_options {
	struct {
		int (*function) (void *context, uint64_t offset, const void *buffer, int64_t length);
		void *context;
	} emitter;
};

struct linearbuffers_encoder * linearbuffers_encoder_create (struct linearbuffers_encoder_create_options *options);
void linearbuffers_encoder_destroy (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder, struct linearbuffers_encoder_reset_options *options);

int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type, enum linearbuffers_encoder_offset_type offset_type, uint64_t elements, uint64_t size);
int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_table_cancel (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_table_set_int8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int8_t value);
int linearbuffers_encoder_table_set_int16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int16_t value);
int linearbuffers_encoder_table_set_int32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int32_t value);
int linearbuffers_encoder_table_set_int64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, int64_t value);

int linearbuffers_encoder_table_set_uint8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint8_t value);
int linearbuffers_encoder_table_set_uint16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint16_t value);
int linearbuffers_encoder_table_set_uint32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint32_t value);
int linearbuffers_encoder_table_set_uint64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value);

int linearbuffers_encoder_table_set_float (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, float value);
int linearbuffers_encoder_table_set_double (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, double value);

int linearbuffers_encoder_table_set_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value);
int linearbuffers_encoder_table_nset_string (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, const char *value, uint64_t n);

int linearbuffers_encoder_table_set_table (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value);
int linearbuffers_encoder_table_set_vector (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, uint64_t value);

int linearbuffers_encoder_table_set_vector_int8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const int8_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const int16_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const int32_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_int64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const int64_t *value, uint64_t count);

int linearbuffers_encoder_table_set_vector_uint8 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const uint8_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint16 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const uint16_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint32 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const uint32_t *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_uint64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const uint64_t *value, uint64_t count);

int linearbuffers_encoder_table_set_vector_float (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const float *value, uint64_t count);
int linearbuffers_encoder_table_set_vector_double (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t offset, enum linearbuffers_encoder_count_type count_type, const double *value, uint64_t count);

int linearbuffers_encoder_vector_start_int8 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_int8 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_int8 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_int16 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_int16 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_int16 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_int32 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_int32 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_int32 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_int64 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_int64 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_int64 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_uint8 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_uint8 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_uint8 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_uint16 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_uint16 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_uint16 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_uint32 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_uint32 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_uint32 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_uint64 (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_uint64 (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_uint64 (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_float (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_float (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_float (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_double (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_double (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_double (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_string (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_string (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_string (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_start_table (struct linearbuffers_encoder *encoder, enum linearbuffers_encoder_count_type count_type);
int linearbuffers_encoder_vector_end_table (struct linearbuffers_encoder *encoder, uint64_t *offset);
int linearbuffers_encoder_vector_cancel_table (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_vector_push_int8 (struct linearbuffers_encoder *encoder, int8_t value);
int linearbuffers_encoder_vector_push_int16 (struct linearbuffers_encoder *encoder, int16_t value);
int linearbuffers_encoder_vector_push_int32 (struct linearbuffers_encoder *encoder, int32_t value);
int linearbuffers_encoder_vector_push_int64 (struct linearbuffers_encoder *encoder, int64_t value);

int linearbuffers_encoder_vector_push_uint8 (struct linearbuffers_encoder *encoder, uint8_t value);
int linearbuffers_encoder_vector_push_uint16 (struct linearbuffers_encoder *encoder, uint16_t value);
int linearbuffers_encoder_vector_push_uint32 (struct linearbuffers_encoder *encoder, uint32_t value);
int linearbuffers_encoder_vector_push_uint64 (struct linearbuffers_encoder *encoder, uint64_t value);

int linearbuffers_encoder_vector_push_float (struct linearbuffers_encoder *encoder, float value);
int linearbuffers_encoder_vector_push_double (struct linearbuffers_encoder *encoder, double value);

int linearbuffers_encoder_vector_push_string (struct linearbuffers_encoder *encoder, const char *value);
int linearbuffers_encoder_vector_push_table (struct linearbuffers_encoder *encoder, uint64_t value);

const void * linearbuffers_encoder_linearized (struct linearbuffers_encoder *encoder, uint64_t *length);

#endif
