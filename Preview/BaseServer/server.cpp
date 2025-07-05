#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    // 创建套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    // 配置服务器地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // 绑定套接字和地址
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        return -1;
    }

    // 监听连接
    if (listen(server_fd, 5) == -1) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on port 8080..." << std::endl;

    while(true) {
        // 接受连接
        struct sockaddr_in client_addr;
        std::memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

        // 接收和打印消息
        char buffer[1024];
        while(true) {
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
            if(bytes_read <= 0) {
                break;
            }
            buffer[bytes_read] = '\0';
            std::cout << "Received: " << buffer << std::endl;
        }

        // 关闭连接
        close(client_fd);
    }

    close(server_fd);
    return 0;
}