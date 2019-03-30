#include <eventloop/AnMod1.h>
#include <eventloop/Common.h>
#include <eventloop/AnEvent1.h>
#include <eventloop/AnConn1.h>
#include <eventloop/AnSocket1.h>
#include <eventloop/EventLoop1.h>
#include <log/BFLog.hpp>
#include <sys/eventfd.h>

namespace dan { namespace eventloop{

AnMod1::AnMod1(const char* szAddress, uint16_t dwPort) noexcept:
    m_strAddress(szAddress),
    m_dwPort(dwPort),
    m_pstEventLoop(new EventLoop1()),
    m_pstServerEvent(new AnEvent1(AnSocket1::MakeSocket(SOCKET_SERVER, szAddress, dwPort), m_pstEventLoop.get(), EVENT_SERVER)),
    m_pstWorkerEvent(new AnEvent1(AnMod1::CreateEventfd(), m_pstEventLoop.get(), EVENT_CLIENT))
{
    //check everything
    
    if(m_pstEventLoop == nullptr)
    {
        dan::log::BFLog::ERR("event loop init failed: nullptr");
        ::exit(-1);
    }
  
    if(m_pstServerEvent == nullptr)
    {
        dan::log::BFLog::ERR("server event init failed: nullptr");
        ::exit(-1);
    }
 
    if(m_pstWorkerEvent == nullptr)
    {
        dan::log::BFLog::ERR("worker event init failed: nullptr");
        ::exit(-1);
    }

    if(m_pstServerEvent->GetFd() <= 0) 
    {
        dan::log::BFLog::ERR("server event init failed: fd < 0");
        ::exit(-1);
    }
        
    if(m_pstWorkerEvent->GetFd() <= 0)
    {
        dan::log::BFLog::ERR("worker event init failed: fd < 0");
        ::exit(-1);
    }
}

AnMod1::~AnMod1()
{}

std::shared_ptr<AnConn1> AnMod1::Accept(uint64_t ulUid)
{
    auto fd = AnSocket1::Accept(m_pstServerEvent->GetFd());
    if(fd > 0)
    {
        auto e = std::make_shared<AnEvent1>(fd, m_pstEventLoop.get(), EVENT_CLIENT);
        return std::make_shared<AnConn1>(this, e, ulUid);
    }
    else
    {
        return nullptr;
    }
}

int AnMod1::GetEpollFd()
{
    return m_pstEventLoop->GetEpollFd();
}


int AnMod1::CreateEventfd()
{
    int iFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return iFd;
}

void AnMod1::Run()
{
    dan::log::BFLog::INFO("server is listening at {}:{}...", m_strAddress, m_dwPort); 
    m_pstEventLoop->Wait();
}

void AnMod1::Stop()
{
    m_pstEventLoop->Stop();
}


}}
