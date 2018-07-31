
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "14-encoder.h"
#include "14-decoder.h"
#include "14-jsonify.h"

int main (int argc, char *argv[])
{
        int rc;

        size_t i;
        uint8_t data[10];

        uint64_t linearized_length;
        const char *linearized_buffer;

        struct linearbuffers_encoder *encoder;
        const struct linearbuffers_output *output;

        (void) argc;
        (void) argv;

        srand(time(NULL));
        for (i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
                data[i] = rand();
        }

        encoder = linearbuffers_encoder_create(NULL);
        if (encoder == NULL) {
                fprintf(stderr, "can not create linearbuffers encoder\n");
                goto bail;
        }

        rc  = linearbuffers_output_start(encoder);
        rc |= linearbuffers_timeval_start(encoder);
        rc |= linearbuffers_timeval_seconds_set(encoder, 2);
        rc |= linearbuffers_timeval_useconds_set(encoder, 3);
        rc |= linearbuffers_output_timeval_set(encoder, linearbuffers_timeval_end(encoder));
        rc |= linearbuffers_output_length_set(encoder, sizeof(data) / sizeof(data[0]));
        rc |= linearbuffers_output_data_set(encoder, (uintptr_t) data);
        rc |= linearbuffers_output_finish(encoder);
        if (rc != 0) {
                fprintf(stderr, "can not encode output\n");
                goto bail;
        }

        linearized_buffer = linearbuffers_encoder_linearized(encoder, &linearized_length);
        if (linearized_buffer == NULL) {
                fprintf(stderr, "can not get linearized buffer\n");
                goto bail;
        }
        fprintf(stderr, "linearized: %p, length: %ld\n", linearized_buffer, linearized_length);

        linearbuffers_output_jsonify(linearized_buffer, linearized_length, LINEARBUFFERS_JSONIFY_FLAG_DEFAULT, (int (*) (void *context, const char *fmt, ...)) fprintf, stderr);

        output = linearbuffers_output_decode(linearized_buffer, linearized_length);
        if (output == NULL) {
                fprintf(stderr, "decoder failed: linearbuffers_output_decode\n");
                goto bail;
        }
        if (linearbuffers_timeval_seconds_get(linearbuffers_output_timeval_get(output)) != 2) {
                fprintf(stderr, "decoder failed: linearbuffers_timeval_seconds_get\n");
                goto bail;
        }
        if (linearbuffers_timeval_useconds_get(linearbuffers_output_timeval_get(output)) != 3) {
                fprintf(stderr, "decoder failed: linearbuffers_timeval_useconds_get\n");
                goto bail;
        }
        if (linearbuffers_output_length_get(output) != sizeof(data) / sizeof(data[0])) {
                fprintf(stderr, "decoder failed: linearbuffers_output_length_get\n");
                goto bail;
        }
        if ((uint8_t *) linearbuffers_output_data_get(output) != data) {
                fprintf(stderr, "decoder linearbuffers_output_data_get: linearbuffers_output_length_get\n");
                goto bail;
        }
        for (i = 0; i < linearbuffers_output_length_get(output); i++) {
                if (data[i] != ((uint8_t *) linearbuffers_output_data_get(output))[i]) {
                        fprintf(stderr, "decoder failed: linearbuffers_uint8_vector_get_at\n");
                        goto bail;
                }
        }

        linearbuffers_encoder_destroy(encoder);

        return 0;
bail:   if (encoder != NULL) {
                linearbuffers_encoder_destroy(encoder);
        }
        return -1;
}
