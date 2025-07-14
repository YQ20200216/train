#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

int main() {
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // 连接服务器
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return 2;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 3;
    }

    std::cout << "Connected to server. Type 'exit' to quit." << std::endl;

    // 交互循环
    while (true) {
        std::string input;
        std::cout << "Enter message: ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        // 发送数据
        send(sock, input.c_str(), input.size(), 0);

        // 接收响应
        char buffer[1024] = {0};
        int valread = read(sock, buffer, sizeof(buffer));
        std::cout << "Server response: " << buffer << std::endl;
    }

    close(sock);
    return 0;
}