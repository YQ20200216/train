#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <vector>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind" << std::endl;
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        std::cerr << "Failed to create epoll" << std::endl;
        return 1;
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET; // 设置为边缘触发模式
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        std::cerr << "Failed to add server fd to epoll" << std::endl;
        return 1;
    }

    std::vector<epoll_event> events(MAX_EVENTS);
    std::cout << "Server started on port 8080 (ET mode)" << std::endl;

    while (true) {
        int num_events = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (num_events < 0) {
            std::cerr << "Epoll wait error" << std::endl;
            continue;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    std::cerr << "Failed to accept client" << std::endl;
                    continue;
                }

                setNonBlocking(client_fd);
                event.events = EPOLLIN | EPOLLET; // 设置为边缘触发模式
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                    std::cerr << "Failed to add client fd to epoll" << std::endl;
                    close(client_fd);
                }

                std::cout << "New client connected: " 
                          << inet_ntoa(client_addr.sin_addr) << ":" 
                          << ntohs(client_addr.sin_port) << std::endl;
            } else {
                if (events[i].events & EPOLLRDHUP) {
                    std::cout << "Client disconnected" << std::endl;
                    close(events[i].data.fd);
                    continue;
                }

                if (events[i].events & EPOLLIN) {
                    char buffer[BUFFER_SIZE];
                    while (true) { // 必须循环读取直到EAGAIN
                        ssize_t bytes_read = recv(events[i].data.fd, buffer, BUFFER_SIZE, 0);
                        if (bytes_read < 0) {
                            // 这里要判断是否是EAGAIN或EWOULDBLOCK
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break; // 数据已读完
                            }
                            std::cerr << "Read error: " << strerror(errno) << std::endl;
                            close(events[i].data.fd);
                            break;
                        } else if (bytes_read == 0) {
                            std::cout << "Client disconnected" << std::endl;
                            close(events[i].data.fd);
                            break;
                        } else {
                            buffer[bytes_read] = '\0';
                            std::cout << "Received: " << buffer << std::endl;
                            send(events[i].data.fd, buffer, bytes_read, 0);
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}