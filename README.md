# echo

## Description

This project is a simple TCP echo server implemented in C++. The server listens for incoming connections, accepts a client connection, receives data from the client, and then sends the same data back to the client.

## Getting Started

#### Prerequisites
This project is built with C++, so you need a C++ compiler installed on your system. The code uses POSIX system calls, so it's best suited for a Unix-like operating system such as Linux or macOS.

#### build

To build the project, run the following command in the project root directory:

```bash
cmake -S . -B build
cmake --build build
```

#### run

To run the server, run the following command in the project root directory:

```bash
./build/server
```

To run the client, run the following command in the project root directory:

```bash
./build/client
```

