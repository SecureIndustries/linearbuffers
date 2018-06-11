
#if !defined(LINEARBUFFERS_DECODER_H)
#define LINEARBUFFERS_DECODER_H

struct linearbuffers_decoder {
	const void *buffer;
	uint64_t length;
};

static inline int  linearbuffers_decoder_init (struct linearbuffers_decoder *decoder, const void *buffer, uint64_t length)
{
	decoder->buffer = buffer;
	decoder->length = length;
	return 0;
}

#endif
