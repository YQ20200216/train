#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return 1;
    }

    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return 1;
    }

    std::cout << "Connected to server at " << argv[1] << ":" << argv[2] << std::endl;
    std::cout << "Enter messages (type 'quit' or 'exit' to end):" << std::endl;

    while (true) {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "quit" || input == "exit") {
            break;
        }

        // 添加X作为消息结束符
        input += "X";
        
        if (send(client_fd, input.c_str(), input.size(), 0) < 0) {
            std::cerr << "Send failed" << std::endl;
            break;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received < 0) {
            std::cerr << "Receive failed" << std::endl;
            break;
        } else if (bytes_received == 0) {
            std::cout << "Server disconnected" << std::endl;
            break;
        } else {
            buffer[bytes_received] = '\0';
            std::cout << "Server response: " << buffer << std::endl;
        }
    }

    close(client_fd);
    return 0;
}