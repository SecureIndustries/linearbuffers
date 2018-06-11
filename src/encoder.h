
#if !defined(LINEARBUFFERS_ENCODER_H)
#define LINEARBUFFERS_ENCODER_H

struct linearbuffers_encoder;

struct linearbuffers_encoder_create_options {
	int _unused;
};

struct linearbuffers_encoder * linearbuffers_encoder_create (struct linearbuffers_encoder_create_options *options);
void linearbuffers_encoder_destroy (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_reset (struct linearbuffers_encoder *encoder);

int linearbuffers_encoder_table_start (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t nelements);
int linearbuffers_encoder_table_end (struct linearbuffers_encoder *encoder);

int linearbuffer_encoder_table_set_uint8 (struct linearbuffers_encoder *encoder, uint64_t element, uint8_t value);
int linearbuffer_encoder_table_set_uint16 (struct linearbuffers_encoder *encoder, uint64_t element, uint16_t value);
int linearbuffer_encoder_table_set_uint32 (struct linearbuffers_encoder *encoder, uint64_t element, uint32_t value);
int linearbuffer_encoder_table_set_uint64 (struct linearbuffers_encoder *encoder, uint64_t element, uint64_t value);

int linearbuffer_encoder_table_set_int8 (struct linearbuffers_encoder *encoder, uint64_t element, int8_t value);
int linearbuffer_encoder_table_set_int16 (struct linearbuffers_encoder *encoder, uint64_t element, int16_t value);
int linearbuffer_encoder_table_set_int32 (struct linearbuffers_encoder *encoder, uint64_t element, int32_t value);
int linearbuffer_encoder_table_set_int64 (struct linearbuffers_encoder *encoder, uint64_t element, int64_t value);

#endif
