#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring> // 添加memset的头文件

int main() {
    
    // 创建socket文件描述符
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    // 绑定socket到端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // 监听连接
    if (listen(server_fd, 2) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    std::ofstream outfile("file.txt", std::ios::app);
    
    while (true) {
        // 接受新连接
        struct sockaddr_in client_addr;
        std::memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr); // 修正socklent_t为socklen_t
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_socket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        while (true) {
            char buffer[1024] = {0};
            int bytes_read = read(new_socket, buffer, 1024);
            if (bytes_read <= 0) {
                break;
            }
            std::cout << "Received: " << buffer << std::endl;
            outfile << buffer << std::endl;
            outfile.flush();
        }
        close(new_socket);
    }
    
    return 0;
}