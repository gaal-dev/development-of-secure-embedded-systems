#ifndef RSA_H
#define RSA_H

#include <FreeRTOS.h>
#include "canny.h"

pixel_t* encrypt_image(pixel_t* in, int32_t nx, int32_t ny, long int n, long int e);

pixel_t* decrypt_image(pixel_t* in, int32_t nx, int32_t ny, long int n, long int d);

bool check_encryption(pixel_t* in, int32_t nx, int32_t ny, long int n, long int e, long int d);

#endif
