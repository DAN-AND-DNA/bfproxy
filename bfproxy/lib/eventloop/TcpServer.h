#pragma once

#include <memory>
#include <eventloop/AnMod.h>
#include <unordered_map>
#include <thread>

namespace dan { namespace eventloop {

class EventLoop;
class AnEvent;
class AnSocket;
class AnConn;

class TcpServer : public AnMod
{
public:
    TcpServer(int iPort, const char* szAddress = "127.0.0.1") noexcept;
    TcpServer() noexcept;
    ~TcpServer();
    std::thread::id ThreadID(){return m_stThreadID;}
    void Run() noexcept;
    virtual int KickConn(uint64_t ulUid){m_stConns.erase(ulUid); return true;} 
    virtual int HandleMsg(uint8_t* pstBytes, uint64_t ulSize){return true;}
private:
    void        PreRun() noexcept;
    uint64_t    GenUid();
private:
    std::unique_ptr<AnSocket>                                m_stServerSocket;
    std::unique_ptr<EventLoop>                               m_pstEventLoop;
    std::unique_ptr<AnEvent>                                 m_pstServerEvent;
    std::thread::id                                          m_stThreadID;     //线程唯一id
    std::unordered_map<uint64_t, std::shared_ptr<AnConn>>    m_stConns;
};

}}

