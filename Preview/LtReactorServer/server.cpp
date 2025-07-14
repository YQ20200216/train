#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define PORT 8080

class Reactor {
private:
    int epoll_fd;
    int server_fd;
    
public:
    Reactor() : epoll_fd(-1), server_fd(-1) {}
    
    ~Reactor() {
        if (epoll_fd != -1) close(epoll_fd);
        if (server_fd != -1) close(server_fd);
    }
    
    void start() {
        // 创建server socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        
        // 设置socket选项
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        
        // 绑定地址和端口
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        
        bind(server_fd, (struct sockaddr*)&address, sizeof(address));
        
        // 开始监听
        listen(server_fd, 5);
        
        // 创建epoll实例
        epoll_fd = epoll_create1(0);
        
        // 添加server socket到epoll
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = server_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
        
        // 事件循环
        struct epoll_event events[MAX_EVENTS];
        while (true) {
            int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            
            for (int i = 0; i < num_events; i++) {
                if (events[i].data.fd == server_fd) {
                    // 新连接
                    handle_new_connection();
                } else {
                    // 客户端数据
                    handle_client_data(events[i].data.fd);
                }
            }
        }
    }
    
private:
    void handle_new_connection() {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        // 设置非阻塞
        fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
        
        // 添加到epoll
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = client_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
    }
    
    void handle_client_data(int fd) {
        char buffer[1024];
        int bytes_read = read(fd, buffer, sizeof(buffer));
        
        if (bytes_read <= 0) {
            // 连接关闭或错误
            close(fd);
            return;
        }
        
        // 处理数据
        buffer[bytes_read] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        
        // 回显数据
        write(fd, buffer, bytes_read);
    }
};

int main() {
    Reactor reactor;
    reactor.start();
    return 0;
}