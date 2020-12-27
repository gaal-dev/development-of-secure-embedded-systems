#ifndef CANNY_H
#define CANNY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// Use short int instead `unsigned char' so that we can
// store negative values.
typedef short int pixel_t;

typedef struct {
    uint32_t header_sz;
    int32_t  width;
    int32_t  height;
    uint16_t nplanes;
    uint16_t bitspp;
    uint32_t compress_type;
    uint32_t bmp_bytesz;
    int32_t  hres;
    int32_t  vres;
    uint32_t ncolors;
    uint32_t nimpcolors;
} bitmap_info_header_t;

pixel_t *load_bmp(const char *filename, bitmap_info_header_t *bitmapInfoHeader);

bool save_bmp(const char *filename, const bitmap_info_header_t *bmp_ih, const pixel_t *data);

pixel_t *canny_edge_detection(const pixel_t *in, const bitmap_info_header_t *bmp_ih, const int tmin, const int tmax, const float sigma);

bool pack_pixels(int width, int height, const pixel_t* data, uint8_t** ppDataOut, uint32_t* len);

#ifdef __cplusplus
}
#endif
#endif