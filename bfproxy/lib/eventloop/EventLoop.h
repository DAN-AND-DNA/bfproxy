#pragma once

#include <common/utils.hpp>
#include <map>
#include <vector>
#include <sys/epoll.h>

namespace dan { namespace eventloop {

class AnEvent;


class EventLoop : common::NonCopyable
{
public:
    EventLoop() noexcept;
    ~EventLoop();

    bool Subscribe(AnEvent* pstEvent) noexcept;
    void Loop() noexcept;
    int  EpollFd() {return m_iEpollFd;}
private:
    void Fire(int iReady) noexcept;
private:
    int                                 m_iEpollFd;         // epoll fd
    std::map<int, AnEvent*>             m_stEvents;
    std::vector<struct epoll_event>     m_stActiveEvents;
};

}}
