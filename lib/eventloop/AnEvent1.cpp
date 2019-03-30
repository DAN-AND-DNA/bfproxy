#include <eventloop/AnEvent1.h>
#include <eventloop/EventLoop1.h>
#include <eventloop/AnBuffer.h>
#include <sys/epoll.h>

namespace dan { namespace eventloop {

AnEvent1::AnEvent1(int iFd, EventLoop1* pstEventLoop, int iEventType) noexcept:
    m_iFd(iFd),
    m_pstEventLoop(pstEventLoop),
    m_dwActions(0),
    m_stReadCallback(),
    m_stWriteCallback(),
    m_iEventType(iEventType)
{

}

AnEvent1::~AnEvent1() noexcept
{
    if(unlikely(m_iFd != -1))
    {
        this->DisableAll();
        ::close(m_iFd);
    }

}
int AnEvent1::GetEventType()
{
    return m_iEventType;
}

int AnEvent1::EnableRead() noexcept
{
    if(likely(m_pstEventLoop != nullptr))
    {
        m_dwActions |= EPOLLIN;     // EPOLLIN =  EPOLLIN | EPOLLERR | EPOLLHUP
        m_dwActions |= EPOLLPRI;
        return m_pstEventLoop->Update(this);
    }
    return -1;
}

int AnEvent1::DisableRead() noexcept
{
    if(likely(m_pstEventLoop != nullptr))
    {
        m_dwActions &= ~EPOLLIN;     
        m_dwActions &= ~EPOLLPRI;
        return m_pstEventLoop->Update(this);
    }
    return -1;
}

int AnEvent1::EnableWrite() noexcept
{
    if(likely(m_pstEventLoop != nullptr))
    {
        m_dwActions |= EPOLLOUT;
        return m_pstEventLoop->Update(this);
    }
    return -1;
}

int AnEvent1::DisableWrite() noexcept
{
    if(likely(m_pstEventLoop != nullptr))
    {
        m_dwActions &= ~EPOLLOUT; 
        return m_pstEventLoop->Update(this);
    }
    return -1;
}

int AnEvent1::DisableAll() noexcept
{
    if(likely(m_pstEventLoop != nullptr))
    {
        m_dwActions = 0;
        return m_pstEventLoop->Update(this);
    }
    return -1;
}



}}
