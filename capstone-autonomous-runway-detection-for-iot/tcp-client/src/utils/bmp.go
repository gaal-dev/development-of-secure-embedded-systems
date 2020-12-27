package utils

import (
	"encoding/binary"
	"fmt"
	"math"
	"os"
	"unsafe"
)

// BMPFileMagic is a magic number
type BMPFileMagic struct {
	Magic [2]uint8
}

// BMPFileHeader is a BMP file header
type BMPFileHeader struct {
	FileSize  uint32
	Creator1  uint16
	Creator2  uint16
	BmpOffset uint32
}

// BMPInfoHeader is a BMP info header
type BMPInfoHeader struct {
	HeaderSize   uint32
	Width        int32
	Height       int32
	NPlanes      uint16
	BitSpp       uint16
	CompressType uint32
	BmpByteSize  uint32
	HRes         int32
	VRes         int32
	NColors      uint32
	HImpColors   uint32
}

// RGBA is red, green, blue, alpha
type RGBA struct {
	R uint8
	G uint8
	B uint8
	A uint8
}

// Pixel is an alias
type Pixel = int16

// LoadBMP loads a BMP file from a file
func LoadBMP(filename string, bitmapInfoHeader *BMPInfoHeader) []Pixel {
	f, err := os.Open(filename)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return nil
	}
	defer f.Close()

	var mag BMPFileMagic
	err = binary.Read(f, binary.LittleEndian, &mag)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return nil
	}

	// verify that this is a bmp file by check bitmap id
	if mag.Magic[0] != 0x42 || mag.Magic[1] != 0x4D {
		fmt.Fprintf(os.Stderr, "Not a BMP file: magic=%X%X\n", mag.Magic[0], mag.Magic[1])
		return nil
	}

	// read the bitmap file header
	var bitmapFileHeader BMPFileHeader
	err = binary.Read(f, binary.LittleEndian, &bitmapFileHeader)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return nil
	}

	// read the bitmap info header
	err = binary.Read(f, binary.LittleEndian, bitmapInfoHeader)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return nil
	}

	if bitmapInfoHeader.CompressType != 0 {
		fmt.Fprintln(os.Stderr, "Warning, compression is not supported.\n")
	}

	// Palette is not parsed

	// move file point to the beginning of bitmap data
	_, err = f.Seek(int64(bitmapFileHeader.BmpOffset), 0)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return nil
	}

	// allocate enough memory for the bitmap image data
	bitmapImage := make([]Pixel, bitmapInfoHeader.BmpByteSize)
	if bitmapImage == nil {
		return nil
	}

	// read in the bitmap image data
	count := 0
	pad := 4*int32(math.Ceil(float64(int32(bitmapInfoHeader.BitSpp)*bitmapInfoHeader.Width)/32.0)) - bitmapInfoHeader.Width

	bytes := make([]uint8, bitmapInfoHeader.Width)
	for i := int32(0); i < bitmapInfoHeader.Height; i++ {
		_, err = f.Read(bytes)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			return nil
		}
		f.Seek(int64(pad), 1)

		for j := int32(0); j < bitmapInfoHeader.Width; j++ {
			bitmapImage[count] = Pixel(bytes[j])
			count++
		}
	}

	return bitmapImage
}

// SaveBMP save a BMP file
func SaveBMP(filename string, bitmapInfoHeader *BMPInfoHeader, data *[]Pixel) bool {
	f, err := os.Create(filename)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return true
	}
	defer f.Close()

	mag := BMPFileMagic{Magic: [2]uint8{0x42, 0x4D}}
	err = binary.Write(f, binary.LittleEndian, &mag)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return true
	}

	var bitmapFileHeader BMPFileHeader
	offset := unsafe.Sizeof(mag) +
		unsafe.Sizeof(bitmapFileHeader) +
		unsafe.Sizeof(*bitmapInfoHeader) +
		(1<<bitmapInfoHeader.BitSpp)*4

	bitmapFileHeader.FileSize = uint32(offset) + bitmapInfoHeader.BmpByteSize
	bitmapFileHeader.BmpOffset = uint32(offset)

	err = binary.Write(f, binary.LittleEndian, &bitmapFileHeader)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return true
	}

	err = binary.Write(f, binary.LittleEndian, bitmapInfoHeader)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return true
	}

	// Palette
	for i := 0; i < 1<<bitmapInfoHeader.BitSpp; i++ {
		color := RGBA{R: uint8(i), G: uint8(i), B: uint8(i)}
		err = binary.Write(f, binary.LittleEndian, &color)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			return true
		}
	}

	pad := 4*int32(math.Ceil(float64(int32(bitmapInfoHeader.BitSpp)*bitmapInfoHeader.Width)/32.0)) - bitmapInfoHeader.Width

	bytes := make([]uint8, bitmapInfoHeader.Width+pad)
	for i := int32(0); i < bitmapInfoHeader.Height; i++ {
		for j := int32(0); j < bitmapInfoHeader.Width; j++ {
			bytes[j] = uint8((*data)[j+bitmapInfoHeader.Width*i])
		}
		_, err = f.Write(bytes)
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			return true
		}
	}

	return false
}
