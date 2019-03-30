#pragma once

#include <stdint.h>
#include <common/utils.hpp>
#include <memory>
#include <map>

namespace dan { namespace eventloop {

class AnSocket;
class EventLoop;
class AnEvent;
class AnConn;

// interface
class AnMod: common::NonCopyable
{
public:
    AnMod();
    AnMod(const char* szServerAddress, uint16_t dwServerPort) noexcept;
    virtual ~AnMod();

    virtual int KickConn(uint64_t ulId) = 0;
    virtual int HandleC2SMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulConnId) = 0;
    virtual int  HandleS2CMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulRobotId) = 0;
    virtual int  BuildC2SMsg(uint64_t ulRobotId) = 0;

    std::string&                                Address(){return m_strAddress;}
    uint16_t                                    Port(){return m_dwPort;}
    int                                         EpollFd();
protected:
    std::shared_ptr<dan::eventloop::AnConn>     NewConn(uint64_t ulUid);
    void                                        GetDBMsg();
    int                                         Efd();
protected:     
    std::string                                 m_strAddress;
    uint16_t                                    m_dwPort;
    std::unique_ptr<dan::eventloop::AnSocket>   m_pstServerSocket;
    std::unique_ptr<dan::eventloop::AnSocket>   m_pstDBMsgSocket;
    std::unique_ptr<dan::eventloop::EventLoop>  m_pstEventLoop; 
    std::unique_ptr<dan::eventloop::AnEvent>    m_pstServerEvent;
    std::unique_ptr<dan::eventloop::AnEvent>    m_pstDBMsgEvent;
};

}
}
