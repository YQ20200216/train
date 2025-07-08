#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENTS 10

int main() {

    // 创建socket文件描述符
    int server_fd  = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd== 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    // 绑定socket到端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 监听socket
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    // 创建epoll实例
    int epoll_fd = epoll_create(1024);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }
    std::ofstream outfile("file.txt", std::ios::binary | std::ios::app);
    
    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                std::memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_addr_len = sizeof(client_addr); 

                int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if (new_socket < 0) {
                    perror("accept");
                    continue;
                }
                
                // 设置非阻塞模式
                // int flags = fcntl(new_socket, F_GETFL, 0);
                // fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);
                
                ev.events = EPOLLIN;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("epoll_ctl: new_socket");
                    close(new_socket);
                }
            } else {
                char buffer[1024];
                int valread = read(events[n].data.fd, buffer, sizeof(buffer)-1);
                if (valread <= 0) {
                    close(events[n].data.fd);
                } else {
                    buffer[valread] = 0;
                    std::cout << "Received: " << buffer << std::endl;
                    outfile << buffer << std::endl;
                    outfile.flush();
                    write(events[n].data.fd, buffer, strlen(buffer));
                }
            }
        }
    }
    
    return 0;
}