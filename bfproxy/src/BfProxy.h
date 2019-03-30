#pragma once

#include <common/utils.hpp>
#include <thread>
#include <memory>
#include <eventloop/AnMod.h>
#include <threadmq/message_queue.h>


namespace dan { namespace log {
class BfLog;
class BfThread;
}}

namespace dan { namespace db {
//class AnMysql;
}}

namespace dan { namespace eventloop {
class AnConn;
}}

namespace dan { namespace pool {
template<typename T>
class LuPool;
}}


namespace dan { namespace bf {
class  BfThread;


class BfProxy : public dan::eventloop::AnMod
{
public:
    BfProxy(const char* szServerAddress, uint16_t dwServerPort) noexcept;
    virtual ~BfProxy() noexcept;

    void Run();
    virtual int KickConn(uint64_t ulUid);
    virtual int HandleC2SMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulConnId);
    virtual int HandleS2CMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulRobotId);
    virtual int BuildC2SMsg(uint64_t ulRobotId);
    
private:
    int PreRun() noexcept;
private:
    std::unique_ptr<dan::pool::LuPool<dan::bf::BfThread>>       m_pstThreadPool;
    std::map<uint64_t, std::shared_ptr<dan::eventloop::AnConn>> m_stConns;
    uint64_t                                                    m_ulUidGen;
    //std::unique_ptr<dan::log::BfLog>                            m_pstLog;
};


}}
