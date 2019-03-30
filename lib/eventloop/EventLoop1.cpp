#include <eventloop/EventLoop1.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <eventloop/AnEvent1.h>

namespace dan { namespace eventloop {

EventLoop1::EventLoop1() noexcept:
    m_iEpollFd(::epoll_create1(EPOLL_CLOEXEC)),
    m_stAllEvents(),
    m_stFiredEvents(32),
    m_bStart(true)
{
    if(unlikely(this->m_iEpollFd <= 0))
    {
        dan::log::BFLog::FATAL("epoll create failed:{}", ::strerror(errno));
        ::exit(-1);
    }
    dan::log::BFLog::INFO("epoll init success");
}

EventLoop1::~EventLoop1() noexcept
{
    if(unlikely(m_iEpollFd > 0))
    {
        ::close(m_iEpollFd);
    }

    dan::log::BFLog::WARN("epoll close success!");
}

int EventLoop1::Update(AnEvent1* pstNewEvent) noexcept
{
    if(unlikely(pstNewEvent == nullptr))
    {
        dan::log::BFLog::ERR("epoll subscribe failed: nullptr");
        return -1;
    }
    
    auto iFd = pstNewEvent->GetFd();
    
    if(unlikely(iFd < 0))
    {
        dan::log::BFLog::ERR("epoll subscribe failed: bad fd");
        return -1;
    }

    struct ::epoll_event stEpollEvent;
    ::bzero(&stEpollEvent, sizeof(struct ::epoll_event));
    stEpollEvent.data.ptr = pstNewEvent;
    stEpollEvent.events = pstNewEvent->GetActions();
    auto stEpollCtl = EPOLL_CTL_ADD;
    auto it = m_stAllEvents.find(iFd);  
    
    if(likely(it != m_stAllEvents.end()))
    {
        //change
        if(unlikely(stEpollEvent.events == 0))
        {
            stEpollCtl = EPOLL_CTL_DEL;
        }
        else
        {
            stEpollCtl = EPOLL_CTL_MOD;
        }
        
        if(unlikely(::epoll_ctl(m_iEpollFd, stEpollCtl, it->first, &stEpollEvent) == -1))
        {
            if(errno == ENOENT)
            {
                errno = 0;  //ingore no such fd error
            }
            else
            {
                dan::log::BFLog::ERR("epoll_ctl mod failed:{}", ::strerror(errno));
                return -1;
            }
        }
        else
        {
            if(stEpollCtl == EPOLL_CTL_DEL)
            {
                m_stAllEvents.erase(it);
            }
        }
    }
    else
    {
        // add
        if(unlikely(::epoll_ctl(m_iEpollFd, stEpollCtl, pstNewEvent->GetFd(), &stEpollEvent) == -1))
        {
            if(errno == EEXIST)
            {
                errno = 0;  //ingore error
            }
            else
            {
                dan::log::BFLog::ERR("epoll_ctl add failed:{}", ::strerror(errno));
                return -1;
            }
        }
        else
        {
            m_stAllEvents.insert({pstNewEvent->GetFd(), pstNewEvent});
        }
    }

    return 0;
}

void EventLoop1::Wait() noexcept
{
    int iFired = 0;
    while(iFired >= 0 || (iFired == -1 && errno == EINTR))
    {
        if(likely(m_bStart == true))
        {
            iFired = ::epoll_wait(m_iEpollFd, this->m_stFiredEvents.data(), static_cast<int>(this->m_stFiredEvents.size()), EPOLL_WAIT_TIME);
            Fire(iFired);
        }
        else
        {
            dan::log::BFLog::INFO("saving data...");
            dan::log::BFLog::WARN("epoll is stoping...");
            break;
        }
    }
    return;
}


void EventLoop1::Fire(int iFired) noexcept
{
    if(unlikely(iFired == 0))
        return;

    for(int i = 0; i < iFired; ++i)
    {
	// unix.EPOLLIN = unix.EPOLLIN | unix.EPOLLERR | unix.EPOLLHUP
        if(m_stFiredEvents[i].events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) reinterpret_cast<AnEvent1*>(m_stFiredEvents[i].data.ptr)->FiredRead();
        if(m_stFiredEvents[i].events & (EPOLLOUT)) reinterpret_cast<AnEvent1*>(m_stFiredEvents[i].data.ptr)->FiredWrite(); 
    }

    if(iFired == static_cast<int>(m_stFiredEvents.size()))
    {
        m_stFiredEvents.resize(m_stFiredEvents.size() * 2);
    }

}



}}
