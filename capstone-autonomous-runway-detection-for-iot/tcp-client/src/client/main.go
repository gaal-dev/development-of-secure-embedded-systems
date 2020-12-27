package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"net"
	"os"
	"utils"
)

func main() {
	fmt.Println("Connecting to 0.0.0.0:4040")
	conn, err := net.Dial("tcp", "0.0.0.0:4040")
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}
	defer conn.Close()
	fmt.Println("Connected to 0.0.0.0:4040")

	fmt.Println("Loading BMP image")
	var ih utils.BMPInfoHeader
	out := utils.LoadBMP("lenna.bmp", &ih)
	fmt.Println("Loaded BMP image")

	fmt.Println("Encrypting BMP image")
	enc := utils.EncryptImage(out, ih.Width, ih.Height, utils.N, utils.E)
	//utils.CheckEncryption(out, ih.Width, ih.Height, utils.N, utils.E, utils.D)
	if out == nil || enc == nil {
		return
	}
	fmt.Println("Encrypted BMP image")

	sendImage(conn, &enc, &ih)
}

func sendImage(conn net.Conn, data *[]int16, ih *utils.BMPInfoHeader) {
	fmt.Println("Transmitting BMP image")

	buf := new(bytes.Buffer)
	if err := binary.Write(buf, binary.LittleEndian, ih.Width); err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	if err := binary.Write(buf, binary.LittleEndian, ih.Height); err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	if err := binary.Write(buf, binary.LittleEndian, *data); err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	_, err := conn.Write(buf.Bytes())
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	res := make([]byte, 1)
	_, err = conn.Read(res)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	if res[0] != 0 {
		fmt.Printf("The image has been transmitted\n")
	} else {
		fmt.Printf("The image has been lost\n")
	}
}
