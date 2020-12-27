package utils

import (
	"fmt"
	"os"
)

// RSA parameters
const (
	N uint64 = 3233
	E uint64 = 17
	D uint64 = 413
)

func modpow(base uint32, power uint64, mod uint64) uint32 {
	var result uint32 = 1
	for i := uint64(0); i < power; i++ {
		result = uint32(uint64(result*base) % mod)
	}
	return result
}

func convertImage(in []int16, nx int32, ny int32, n uint64, c uint64) []int16 {
	out := make([]int16, nx*ny)
	if out == nil {
		return nil
	}

	pixelTable := make(map[int16]int16)
	pixelVisited := make(map[int16]bool)

	if pixelTable == nil || pixelVisited == nil {
		return nil
	}

	var convPixel int16
	for y := int32(0); y < ny; y++ {
		for x := int32(0); x < nx; x++ {
			pos := y*nx + x
			pixel := in[pos]

			_, ok := pixelVisited[pixel]
			if !ok {
				convPixel = int16(modpow(uint32(pixel), c, n))
				pixelTable[pixel] = convPixel
				pixelVisited[pixel] = true
			} else {
				convPixel = pixelTable[pixel]
			}
			out[pos] = convPixel
		}
	}

	return out
}

// EncryptImage uses RSA algorithm
func EncryptImage(in []int16, nx int32, ny int32, n uint64, e uint64) []int16 {
	return convertImage(in, nx, ny, n, e)
}

// DecryptImage uses RSA algorithm
func DecryptImage(in []int16, nx int32, ny int32, n uint64, d uint64) []int16 {
	return convertImage(in, nx, ny, n, d)
}

// CheckEncryption uses RSA algorithm
func CheckEncryption(in []int16, nx int32, ny int32, n uint64, e uint64, d uint64) bool {
	err := false

	encrypted := EncryptImage(in, nx, ny, n, e)
	out := DecryptImage(encrypted, nx, ny, n, d)

	for y := int32(0); y < ny; y++ {
		for x := int32(0); x < nx; x++ {
			pos := y*nx + x

			if in[pos] != out[pos] {
				fmt.Fprintln(os.Stderr, "wrong pixel (x=%d, y=%d)\n", x, y)
				err = true
			}
		}
	}

	return err
}
