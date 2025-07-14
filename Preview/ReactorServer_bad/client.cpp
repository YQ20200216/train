#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <ip> <port>" << endl;
        return 1;
    }

    // 1. 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 2;
    }

    // 2. 连接服务器
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sock);
        return 3;
    }

    cout << "Connected to server at " << argv[1] << ":" << argv[2] << endl;

    // 3. 交互循环
    while (true) {
        cout << "Please input expression (e.g. 1 + 1): ";
        string input;
        getline(cin, input);

        if (input == "quit" || input == "exit") {
            break;
        }

        // 发送请求
        string request = input + "X"; // 添加协议定义的分隔符
        if (send(sock, request.c_str(), request.size(), 0) < 0) {
            perror("send");
            break;
        }

        // 接收响应
        char buffer[1024];
        ssize_t s = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (s > 0) {
            buffer[s] = 0;
            string response(buffer);
            size_t end = response.find(" ");
            if (end != string::npos) {
                int code = stoi(response.substr(0, end));
                int result = stoi(response.substr(end+1));
                cout << "Server response - Code: " << code << ", Result: " << result << endl;
            } else {
                cout << "Invalid response format: " << response << endl;
            }
        } else if (s == 0) {
            cout << "Server closed connection" << endl;
            break;
        } else {
            perror("recv");
            break;
        }
    }

    close(sock);
    return 0;
}