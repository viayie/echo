#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING_CONNECTIONS = 1024;

int main() {
    // Create a new socket
    // AF_INET: use IPv4
    // SOCK_STREAM: use TCP
    // 0: use default protocol (TCP in this case)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Set socket options
    // server_fd: the file descriptor of the socket to set options on
    // SOL_SOCKET: the level at which the option is defined (socket level)
    // SO_REUSEADDR | SO_REUSEPORT: the option to set (allow reuse of address and port)
    // &opt: pointer to a buffer containing the new value for the option
    // sizeof(opt): size of the buffer
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to a specific address and port
    // server_fd: the file descriptor of the socket to bind
    // (sockaddr *)&server_addr: pointer to a sockaddr_in structure containing the address to bind to
    // sizeof(server_addr): size of the address structure
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9527);

    if (bind(server_fd, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    // server_fd: the file descriptor of the socket to listen on
    // n: the maximum length of the queue of pending connections
    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Accept a client connection
    // server_fd: the file descriptor of the socket to accept the connection on
    // (sockaddr *)&client_addr: pointer to a sockaddr_in structure that will contain the address of the client on return
    // (socklen_t *)&client_addr_len: pointer to a socklen_t variable that should contain the size of the address structure on call, and will contain the size of the actual address on return
    sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd < 0) {
        std::cerr << "Failed to accept" << std::endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port)
              << std::endl;

    while (true) {
        // Receive data from the client
        // client_fd: the file descriptor of the client socket to receive data from
        // buffer.data(): pointer to the buffer to receive the data into
        // buffer.size() - 1: size of the buffer (subtract 1 to leave space for the string null terminator)
        // 0: flags specifying the behavior of the receive operation (0 means use default behavior)
        std::vector<char> buffer(BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer.data(), buffer.size() - 1, 0);
        if (bytes_received < 0) {
            std::cerr << "Failed to receive data" << std::endl;
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            std::cout << "Connection closed" << std::endl;
            break;
        }

        buffer[bytes_received] = '\0';
        std::cout << "Received " << bytes_received << " bytes: " << buffer.data() << std::endl;

        // Send the received data back to the client
        // client_fd: the file descriptor of the client socket to send data to
        // buffer.data(): pointer to the buffer containing the data to send
        // bytes_received: number of bytes to send
        // 0: flags specifying the behavior of the send operation (0 means use default behavior)
        ssize_t bytes_sent = send(client_fd, buffer.data(), bytes_received, 0);
        if (bytes_sent < 0) {
            std::cerr << "Failed to send data" << std::endl;
            close(client_fd);
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        std::cout << "Sent " << bytes_sent << " bytes: " << buffer.data() << std::endl;
    }

    // Close the connections
    close(client_fd);
    close(server_fd);

    return 0;
}