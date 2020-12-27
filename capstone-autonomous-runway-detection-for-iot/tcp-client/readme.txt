How-to run the TCP server and client:
1) https://golang.org/dl/ install Go
2) .envrc is for changing GOPATH environment variable on Unix-like operating system (Linux, macOS, …) and
   for a supported shell (bash, zsh, tcsh, fish, elvish)
3) GOPATH has to contain a root project directory to compile
   run `go build .` command from src/server/ or src/client/,
   `go install` for installing applications to bin/
   or just `go run .` from src/server/ or src/client/
3) use an IDE supported Go such as Visual Studio Code or GoLand if this way is convenient for you

Visual Studio Code:
a) install Go extension
b) set GOPATH or use yours: File->Preferences->Settings: Go: Infer GoPath
c) View->Command Pallette: Go Install/Update (do not forget about GOPATH)
d) run main.go from src/server/ or src/client/
