package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"net"
	"os"
	"time"
	"utils"
)

func handleConnection(conn net.Conn) {
	recoverTCP := func() {
		if r := recover(); r != nil {
			fmt.Println("recovered from ", r)
		}
	}
	defer recoverTCP()

	fmt.Printf("Serving %s\n", conn.RemoteAddr().String())

	var width, height, size int32
	var bufSize int32 = 65536
	counter := 1

	buf := new(bytes.Buffer)
	data := make([]byte, bufSize)

	for {
		fmt.Printf("Reading image %d is started\n", counter)

		err := conn.SetReadDeadline(time.Now().Add(time.Second))
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			break
		}

		if err := binary.Read(conn, binary.LittleEndian, &width); err != nil {
			fmt.Fprintln(os.Stderr, err)
			break
		}
		if err := binary.Read(conn, binary.LittleEndian, &height); err != nil {
			fmt.Fprintln(os.Stderr, err)
			break
		}

		size = width * height * 2

		fmt.Printf("Width=%d Height=%d Size=%d\n", width, height, size)

		buf.Reset()
		chunk := 1
		pos := 0

		for size > 0 {
			limit := bufSize
			if size < bufSize {
				limit = bufSize
			}

			err := conn.SetReadDeadline(time.Now().Add(time.Second))
			if err != nil {
				fmt.Fprintln(os.Stderr, err)
				goto exit // golang approach to break nested loops
			}

			n, err := conn.Read(data[:limit])
			size -= int32(n)
			pos += n
			if err != nil {
				fmt.Fprintln(os.Stderr, err)
				goto exit // golang approach to break nested loops
			}
			buf.Write(data[:n])
			fmt.Printf("Chunk %d size %d pos %d\n", chunk, n, pos)

			chunk++
		}

		fmt.Printf("Reading image %d is finished\n", counter)

		var enc []int16 = make([]int16, width*height)
		if err := binary.Read(buf, binary.LittleEndian, &enc); err != nil {
			fmt.Fprintln(os.Stderr, err)
			data[0] = 0
		} else {
			// remove a previous frame
			fn := fmt.Sprintf("lenna%d-%s.bmp", counter-4, conn.RemoteAddr().String())
			if _, err := os.Stat(fn); err == nil {
				_ = os.Remove(fn)
			}

			// a current frame
			fn = fmt.Sprintf("lenna%d-%s.bmp", counter, conn.RemoteAddr().String())

			fmt.Printf("Reading image %d is started\n", counter)

			var ih utils.BMPInfoHeader
			ih.HeaderSize = 40
			ih.Width = width
			ih.Height = height
			ih.NPlanes = 1
			ih.BitSpp = 8
			ih.BmpByteSize = uint32(width * height)
			ih.NColors = 256

			dec := utils.DecryptImage(enc, ih.Width, ih.Height, utils.N, utils.D)
			if utils.SaveBMP(fn, &ih, &dec) {
				fmt.Fprintln(os.Stderr, "%s is not written")
				data[0] = 0
			} else {
				data[0] = 1
			}

			fmt.Printf("Writing image %d is finished\n", counter)
		}

		err = conn.SetWriteDeadline(time.Now().Add(time.Second))
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			break
		}

		_, err = conn.Write(data[:1])
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			break
		}

		counter++
	}
exit:
	fmt.Printf("Served %s\n", conn.RemoteAddr().String())
	conn.Close()

	// remove a previous frame
	fn := fmt.Sprintf("lenna%d-%s.bmp", counter-4, conn.RemoteAddr().String())
	if _, err := os.Stat(fn); err == nil {
		_ = os.Remove(fn)
	}
}

func main() {
	fmt.Println("Starting 0.0.0.0:4040")
	server, err := net.Listen("tcp", "0.0.0.0:4040")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}
	defer server.Close()
	fmt.Println("Started 0.0.0.0:4040")

	for {
		fmt.Println("Waiting for a connection")
		conn, err := server.Accept()
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			return
		}
		fmt.Println("A connection is accepted")

		go handleConnection(conn)
	}
}
