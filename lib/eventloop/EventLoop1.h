#pragma once

#include <common/utils.hpp>
#include <map>
#include <sys/epoll.h>

namespace dan { namespace eventloop {

class AnEvent1;

class EventLoop1 : common::NonCopyable
{
public:
    EventLoop1() noexcept;
    ~EventLoop1() noexcept;
    int Update(AnEvent1* pstNewEvent) noexcept;
    void Wait() noexcept;
    void Stop() {m_bStart = false;}
    int GetEpollFd(){return m_iEpollFd;}
private:
    void Fire(int iFired) noexcept;
private:
    int                                 m_iEpollFd;
    std::map<int, AnEvent1*>            m_stAllEvents;      // fd : event
    std::vector<struct ::epoll_event>   m_stFiredEvents;    
    bool                                m_bStart;
};

}}
