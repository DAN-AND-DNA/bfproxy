#pragma once

#include <common/utils.hpp>
#include <memory>

namespace dan { namespace eventloop {

class EventLoop1;
class AnEvent1;
class AnConn1;

class AnMod1 : common::NonCopyable
{
public:
    AnMod1(const char* szAddress, uint16_t dwPort) noexcept;
    virtual                         ~AnMod1();
   
    virtual void KickConn(uint64_t ulUid) = 0;
    virtual int HandleC2S(uint8_t* pstBytes, uint64_t dwSize, uint64_t ulSessionId, int iConnFd) = 0;
    void                            Run();
    void                            Stop();
protected:
    std::shared_ptr<AnConn1>        Accept(uint64_t ulUid);
    int                             GetEpollFd();
    static int                      CreateEventfd();
protected:
    std::string                                 m_strAddress;
    uint16_t                                    m_dwPort;
    std::unique_ptr<dan::eventloop::EventLoop1> m_pstEventLoop;
    std::unique_ptr<dan::eventloop::AnEvent1>   m_pstServerEvent;
    std::unique_ptr<dan::eventloop::AnEvent1>   m_pstWorkerEvent;
};

}}
