#include "rsa.h"

unsigned long long int modpow(int base, int power, int mod)
{
	int i;
	unsigned long long int result = 1;
	for (i = 0; i < power; ++i)
	{
		result = (result * base) % mod;
	}
	return result;
}

pixel_t* convert_image(pixel_t* in, int32_t nx, int32_t ny, long int n, long int c) {
	pixel_t* out = malloc(nx * ny * sizeof(pixel_t));

	// memoization - 65536 bytes are fine for small numbers. hashing can be used for big numbers.
    // canny edge MAX_BRIGHTNESS is 255, but modpow returns bigger numbers

	pixel_t* pixel_table = malloc(SHRT_MAX * sizeof(pixel_t));
	bool* pixel_visited = calloc(SHRT_MAX, sizeof(bool));

	if (out == NULL || pixel_table == NULL || pixel_visited == NULL) {
		if (out != NULL) free(out);
		if (pixel_table != NULL) free(pixel_table);
		if (pixel_visited != NULL) free(pixel_visited);
		return NULL;
	}

	int32_t y, x;
	for (y = 0; y < ny; ++y) {
		for (x = 0; x < nx; ++x) {
			int pos = y * nx + x;
			pixel_t pixel = in[pos];
			pixel_t conv_pixel;
			if (!pixel_visited[pixel]) {
				conv_pixel = modpow(pixel, c, n);

				pixel_table[pixel] = conv_pixel;
				pixel_visited[pixel] = true;
			} else {
				conv_pixel = pixel_table[pixel];
			}

			out[pos] = conv_pixel;
		}
	}

	free(pixel_table);
	free(pixel_visited);

	return out;
}

pixel_t* encrypt_image(pixel_t* in, int32_t nx, int32_t ny, long int n, long int e) {
	return convert_image(in, nx, ny, n, e);
}

pixel_t* decrypt_image(pixel_t* in, int32_t nx, int32_t ny, long int n, long int d) {
	return convert_image(in, nx, ny, n, d);
}

bool check_encryption(pixel_t* in, int32_t nx, int32_t ny, long int n, long int e, long int d) {
	int32_t y, x;

	pixel_t* encrypted = encrypt_image(in, nx, ny, n, e);
	if (encrypted == NULL) {
		return true;
	}

	pixel_t* out = decrypt_image(encrypted, nx, ny, n, d);
	free(encrypted);

	if (out == NULL) {
		return true;
	}

	bool error = true;
	for (y = 0; y < ny; ++y) {
		for (x = 0; x < nx; ++x) {
			int pos = y * nx + x;

			// this is too slow for big powers
			//pixel_t _enc_pixel = modpow(pixel, e, n);
			//pixel_t _dec_pixel = modpow(enc_pixel, d, n);

			if (in[pos] != out[pos]) {
				printf("wrong pixel (x=%d, y=%d)\n", x, y);
				error = true;
			}
		}
	}

	free(out);

	return error;
}