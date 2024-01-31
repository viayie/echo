#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_EVENTS = 1024;
constexpr int MAX_PENDING_CONNECTIONS = 1024;

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9527);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_PENDING_CONNECTIONS) < 0) {
        perror("Failed to listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Create an epoll instance
    // 0: use the default size
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("Failed to create epoll instance");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Add the server socket to the event pool
    // EPOLLIN: the server socket is ready to receive data
    // EPOLLET: use edge-triggered mode
    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("Failed to add server socket to epoll instance");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // Main loop
    std::vector<epoll_event> events(MAX_EVENTS);
    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (event_count < 0) {
            perror("Failed to wait for events");
            close(server_fd);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == server_fd) {
                sockaddr_in client_addr{};
                socklen_t client_addr_len = sizeof(client_addr);

                int client_fd = accept(server_fd, (sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
                if (client_fd < 0) {
                    perror("Failed to accept connection");
                    close(server_fd);
                    close(epoll_fd);
                    exit(EXIT_FAILURE);
                }

                std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
                          << ntohs(client_addr.sin_port) << std::endl;

                // Add the client socket to the event pool
                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                    perror("Failed to add client socket to epoll instance");
                    close(client_fd);
                    close(server_fd);
                    close(epoll_fd);
                    exit(EXIT_FAILURE);
                }
            } else {
                // Receive data from the client
                std::vector<char> buffer(BUFFER_SIZE);
                ssize_t bytes_received = recv(events[i].data.fd, buffer.data(), buffer.size() - 1, 0);
                if (bytes_received < 0) {
                    perror("Failed to receive data");
                    close(events[i].data.fd);
                    close(server_fd);
                    close(epoll_fd);
                    exit(EXIT_FAILURE);
                } else if (bytes_received == 0) {
                    std::cout << "Connection closed" << std::endl;
                    close(events[i].data.fd);
                    continue;
                }

                buffer[bytes_received] = '\0';
                std::cout << "Received " << bytes_received << " bytes: " << buffer.data() << std::endl;

                // Send the received data back to the client
                ssize_t bytes_sent = send(events[i].data.fd, buffer.data(), bytes_received, 0);
                if (bytes_sent < 0) {
                    perror("Failed to send data");
                    close(events[i].data.fd);
                    close(server_fd);
                    close(epoll_fd);
                    exit(EXIT_FAILURE);
                }

                std::cout << "Sent " << bytes_sent << " bytes: " << buffer.data() << std::endl;
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    return 0;
}
