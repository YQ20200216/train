#pragma once
#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <sys/epoll.h>

class Epoller
{
public:
    static const int gsize = 128;
public:
    // 创建epoll
    static int CreateEpoller()
    {
        int epfd = epoll_create(gsize);// 创建对应size的epfd
        if (epfd < 0)
        {
            std::cout<<"epoll_create :" <<errno<< ':'<< strerror(errno)<<std::endl;
            exit(3);
        }
        return epfd;
    }
    // 给epoll添加事件
    static bool AddEvent(int epfd, int sock, uint32_t event)
    {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = sock;
        int n = epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);// 给对应的socket添加到epoll中
        return n == 0;
    }
    // 给epoll修改事件
    static bool ModEvent(int epfd, int sock, uint32_t event)
    {
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = sock;
        int n = epoll_ctl(epfd, EPOLL_CTL_MOD, sock, &ev);// 修改已有scoket的event
        return n == 0;
    }
    // 给epoll删除事件
    static bool DelEvent(int epfd, int sock)
    {
        int n = epoll_ctl(epfd, EPOLL_CTL_DEL, sock, nullptr);// 删除指定socket
        return n == 0;
    }
    // 从epoll中获取就绪事件
    static int LoopOnce(int epfd, struct epoll_event revs[], int num)
    {
        // 单次wait的调用，从数组里面取回就绪的文件描述符
        int n = epoll_wait(epfd, revs, num, -1);
        if(n == -1)
        {
            std::cout<<"epoll_wait : %d : %s" <<errno<< ':'<< strerror(errno)<<std::endl;
        }
        return n;
    }
};
