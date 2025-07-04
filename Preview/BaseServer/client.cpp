// client.cpp ，test2测试
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void start()
{
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket: " << std::strerror(errno) << std::endl;
        return;
    }

    // 设置服务器地址
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << std::strerror(errno) << std::endl;
        close(sockfd);
        return;
    }

    // 连接到服务器
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect: " << std::strerror(errno) << std::endl;
        close(sockfd);
        return;
    }

    std::cout << "Connected to server." << std::endl;

    std::string msg;
    while (true) {
        std::cout << "Please Enter# ";
        std::getline(std::cin, msg);

        if (write(sockfd, msg.c_str(), msg.size()) == -1) {
            std::cerr << "Failed to write: " << std::strerror(errno) << std::endl;
            break;
        }

        if(msg == "quit") {
            break;
        }
    }

    close(sockfd);
}

int main()
{
    start();
    return 0;
}
