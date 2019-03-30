#include <eventloop/AnMod.h>
#include <eventloop/AnSocket.h>
#include <eventloop/EventLoop.h>
#include <eventloop/AnEvent.h>
#include <eventloop/AnConn.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

namespace 
{

int CreateEventfd()
{
    int iFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(iFd < 0)
    {
        ::exit(-1);
    }
    return iFd;
}


}

namespace dan { namespace eventloop {

AnMod::AnMod(){}

AnMod::AnMod(const char* szAddress, uint16_t dwPort) noexcept:
    m_strAddress(szAddress),
    m_dwPort(dwPort),
    m_pstServerSocket(new AnSocket(true, dwPort, szAddress)),
    m_pstDBMsgSocket(new AnSocket(::CreateEventfd())),
    m_pstEventLoop(new EventLoop()),
    m_pstServerEvent(new AnEvent(m_pstServerSocket->Fd(), m_pstEventLoop.get())),
    m_pstDBMsgEvent(new AnEvent(m_pstDBMsgSocket->Fd(), m_pstEventLoop.get()))
{
    //printf("ss:%d  ms:%d\n", m_pstServerSocket->Fd(), m_pstDBMsgSocket->Fd());
}

AnMod::~AnMod()
{}

int AnMod::EpollFd()
{
    return m_pstEventLoop->EpollFd();
}



std::shared_ptr<AnConn> AnMod::NewConn(uint64_t ulUid)
{
    auto fd = m_pstServerSocket->Accept();

    printf("fd:%d\n", fd);
    if (fd != -1)
    {
         return std::make_shared<AnConn>(this, fd, ulUid, m_pstEventLoop.get());
    
    }
    else
    {
        return nullptr;
    }
}


void AnMod::GetDBMsg()
{
    printf("getdbmsg\n");
    uint64_t i = 1;
    auto n = m_pstDBMsgSocket->Read(&i, sizeof i);

    if(n != sizeof i)
    {
        printf("error\n");
    }
}

int AnMod::Efd()
{

    return m_pstDBMsgSocket->Fd();
}

}}
