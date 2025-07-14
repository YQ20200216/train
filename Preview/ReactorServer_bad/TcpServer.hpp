#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <unordered_map>
#include <functional>
#include <fcntl.h>
#include "Sock.hpp"
#include "Epoller.hpp"
#include "Protocol.hpp"

class Connection;
class TcpServer;

using func_t = std::function<int(Connection *)>;
typedef int (*callback_t)(Connection *, std::string &);

//class Connection
class Connection
{
public:
    int sock_;              // I/O 文件描述符
    TcpServer *R_;          // 主服务器的类指针
    std::string inbuffer_;  // 接收缓冲区
    std::string outbuffer_; // 发送缓冲区
    func_t recver_;         // 读事件回调函数
    func_t sender_;         // 写事件回调函数
    func_t excepter_;       // 异常事件回调函数

public:
    Connection(int sock, TcpServer *r) : sock_(sock), R_(r)
    {
    }
    void SetRecver(func_t recver) { recver_ = recver; }
    void SetSender(func_t sender) { sender_ = sender; }
    void SetExcepter(func_t excepter) { excepter_ = excepter; }
    ~Connection() {}
};
//class TcpServer
static void SetNonBlock(int fd){}
class TcpServer
{
public:
    TcpServer(callback_t cb, int port = 8080) : cb_(cb)
    {
        // 为保存就绪事件的数组申请空间
        revs_ = new struct epoll_event[revs_num];
        // 获取 listensock
        listensock_ = Sock::SocketInit();
        SetNonBlock(listensock_);
        Sock::Bind(listensock_, port);
        Sock::Listen(listensock_, 20);

        // 多路转接
        epfd_ = Epoller::CreateEpoller();

        // 添加 listensock 到服务器中
        AddConnection(listensock_, EPOLLIN | EPOLLET,
                      std::bind(&TcpServer::Accepter, this, std::placeholders::_1), nullptr, nullptr);
    }

    void AddConnection(int sockfd, uint32_t event, func_t recver, func_t sender, func_t excepter)
    {
        //设置 sock 为非阻塞
        if (event & EPOLLET)
            SetNonBlock(sockfd);
        // 添加sockfd到epoll
        Epoller::AddEvent(epfd_, sockfd, event);
        // 将sockfd匹配的Connection也添加到当前的unordered_map中
        Connection *conn = new Connection(sockfd, this);
        conn->SetRecver(recver);
        conn->SetSender(sender);
        conn->SetExcepter(excepter);
        //将 Connection 对象的地址插入到哈希表
        connections_.insert(std::make_pair(sockfd, conn));
        std::cout << "添加新链接到connections成功: " << sockfd << std::endl;
    }

    static void SetNonBlock(int fd)
    {
        int fl = fcntl(fd, F_GETFL);
        fcntl(fd, F_SETFL, fl | O_NONBLOCK);
    }

    int Accepter(Connection *conn)
    {
        while (true)
        {
            std::string clientip;
            uint16_t clientport = 0;
            int sockfd = Sock::Accept(conn->sock_, &clientip, &clientport);
            if (sockfd < 0)
            {
                // 接收函数被事件打断了
                if (errno == EINTR)
                    continue;
                // 取完了
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else
                {
                    std::cout << "accept error" << std::endl;
                    return -1;
                }
            }
            std::cout << "get a new link: " << sockfd << std::endl;
            //将 sock 交给 TcpServer 监视，并注册回调函数
            AddConnection(sockfd, EPOLLIN | EPOLLET,
                          std::bind(&TcpServer::TcpRecver, this, std::placeholders::_1),
                          std::bind(&TcpServer::TcpSender, this, std::placeholders::_1),
                          std::bind(&TcpServer::TcpExcepter, this, std::placeholders::_1));
        }
        return 0;
    }


    int TcpRecver(Connection *conn)
    {
        while (true)
        {
            char buffer[1024];
            ssize_t s = recv(conn->sock_, buffer, sizeof(buffer) - 1, 0);
            if (s > 0)
            {
                buffer[s] = 0;
                conn->inbuffer_ += buffer;
            }
            else if (s == 0)
            {
                std::cout << "client quit" << std::endl;
                conn->excepter_(conn);
                break;
            }
            else
            {
                // 接收事件被打断
                if (errno == EINTR)
                    continue;
                // 接收缓冲区空了
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else
                {
                    // 出错了
                    std::cout << "recv error: " << errno << std::endl;
                    conn->excepter_(conn);
                    break;
                }
            }
        }
        // 将本轮全部读取完毕
        std::vector<std::string> result;
        PackageSplit(conn->inbuffer_, &result);
        for (auto &message : result)
        {
            cb_(conn, message);
        }
        return 0;
    }

    int TcpSender(Connection *conn)
    {
        while (true)
        {
            ssize_t n = send(conn->sock_, conn->outbuffer_.c_str(), conn->outbuffer_.size(), 0);
            if (n > 0)
            {
                // 去除已经成功发送的数据
                conn->outbuffer_.erase(0, n);
            }
            else
            {
                // 写入操作被打断
                if (errno == EINTR)
                    continue;
                // 写入缓冲区满了，没办法继续写
                else if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else
                {
                    conn->excepter_(conn);
                    std::cout << "send error: " << errno << std::endl;
                    break;
                }
            }
        }
        return 0;
    }

    int TcpExcepter(Connection *conn)
    {
        // 0.判断有效性
        if (!IsExists(conn->sock_))
            return -1;
        // 1.删除epoll的监看
        Epoller::DelEvent(epfd_, conn->sock_);
        std::cout << "remove epoll event!" << std::endl;
        // 2.close
        close(conn->sock_);
        std::cout << "close fd: " << conn->sock_ << std::endl;
        // 3. delete conn
        delete connections_[conn->sock_];
        std::cout << "delete connection object done" << std::endl;
        // 4.erase conn
        connections_.erase(conn->sock_);
        std::cout << "erase connection from connections" << std::endl;
        return 0;
    }

    bool IsExists(int sock)
    {
        auto iter = connections_.find(sock);
        if (iter == connections_.end())
            return false;
        else
            return true;
    }

    // 打开或者关闭对于特定socket是否要关心读或者写
    // EnableReadWrite(sock, true, false);
    // EnableReadWrite(sock, true, true);
    void EnableReadWrite(int sock, bool readable, bool writeable)
    {
        uint32_t event = 0;
        event |= (readable ? EPOLLIN : 0);
        event |= (writeable ? EPOLLOUT : 0);
        Epoller::ModEvent(epfd_, sock, event);
    }

    // 根据就绪事件，将事件进行事件派发
    void Dispatcher()
    {
        int n = Epoller::LoopOnce(epfd_, revs_, revs_num);
        for (int i = 0; i < n; i++)
        {
            int sock = revs_[i].data.fd;
            uint32_t revent = revs_[i].events;
            // 判断是否出现错误，如果出现了错误，那就把EPOLLIN和OUT都加上
            // 这样这个链接会进入下面的处理函数，并在处理函数中出现异常
            // 处理函数中出现异常回统一调用TcpExcpter函数
            if (revent & EPOLLHUP)
                revent |= (EPOLLIN | EPOLLOUT);
            if (revent & EPOLLERR)
                revent |= (EPOLLIN | EPOLLOUT);

            if (revent & EPOLLIN)
            {
                if (IsExists(sock) && connections_[sock]->recver_)
                    connections_[sock]->recver_(connections_[sock]);
            }
            // 当链接的写事件被激活的时候，在这里就会触发写事件的处理
            // 所以并不需要在recv里面主动调用写事件处理函数
            // 只需要告诉epoll让它帮我们监控写事件，那么就会在这里触发写操作
            if (revent & EPOLLOUT)
            {
                if (IsExists(sock) && connections_[sock]->sender_)
                    connections_[sock]->sender_(connections_[sock]);
            }
        }
    }
    void Run()
    {
        while (true)
        {
            Dispatcher();
        }
    }
    ~TcpServer()
    {
        if (listensock_ != -1)
            close(listensock_);
        if (epfd_ != -1)
            close(epfd_);
        delete[] revs_;
    }

private:
    // 接收队列的长度
    static const int revs_num = 64;
    // 1. 网络socket
    int listensock_;
    // 2. epoll
    int epfd_;
    // 3. 用哈希表保存连接
    std::unordered_map<int, Connection *> connections_;
    // 4. 就绪事件的件描述符的数组
    struct epoll_event *revs_;
    // 5. 设置完整报文的处理方法
    callback_t cb_;
};

