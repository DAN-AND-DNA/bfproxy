#pragma once

#include <eventloop/AnMod1.h>
#include <map>

namespace dan { 

namespace eventloop {
class AnConn1;
}

namespace pool {
template<typename T>
class LuPool;
}

namespace bf {
class BfThread;
}

}

namespace dan { namespace bf {

class BfDBServer : public dan::eventloop::AnMod1
{
public:
    BfDBServer(const char* szServerAddress = "127.0.0.1", uint16_t dwServerPort = 3737) noexcept;
    virtual ~BfDBServer();
    virtual void KickConn (uint64_t ulSessionId);
    virtual int HandleC2S(uint8_t* pstBytes, uint64_t dwSize, uint64_t ulSessionId, int iConnFd);
    void Run();
private:
    int MakeSure();
private:
    uint64_t                                                       m_ulSessionIdGen;
    std::map<uint64_t, std::shared_ptr<dan::eventloop::AnConn1>>   m_stConns;
    std::unique_ptr<dan::pool::LuPool<dan::bf::BfThread>>          m_pstThreadPool;
};

}}
