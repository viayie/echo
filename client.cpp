#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

constexpr int BUFFER_SIZE = 1024;

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9527);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    while (true) {
        std::vector<char> buffer(BUFFER_SIZE);
        std::string message;
        std::cout << "Enter message to send to server (or 'exit' to quit): ";
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }

        if (send(client_fd, message.c_str(), message.size(), 0) < 0) {
            std::cerr << "Failed to send data to server" << std::endl;
            break;
        }

        ssize_t bytes_received = recv(client_fd, buffer.data(), buffer.size() - 1, 0);
        if (bytes_received < 0) {
            std::cerr << "Failed to receive data from server" << std::endl;
            break;
        }

        buffer[bytes_received] = '\0';
        std::cout << "Received from server: " << buffer.data() << std::endl;
    }

    close(client_fd);

    return 0;
}