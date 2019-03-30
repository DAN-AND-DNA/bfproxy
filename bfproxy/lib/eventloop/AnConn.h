#pragma once

#include <memory>
namespace dan {
using MSGHANDLE = std::function<void()>;
}

namespace dan { namespace eventloop {

class AnEvent;
class AnSocket;
class EventLoop;
class AnBuffer;
class TcpServer;
class AnMod;


class AnConn : public std::enable_shared_from_this<AnConn>
{
public:
    AnConn(AnMod* pstMod, int iClientFd, uint64_t ulUid, EventLoop* m_pstEventLoop, uint32_t dwAvgSeqSize = 40) noexcept;
    AnConn(AnMod* pstMod, uint64_t ulUid, EventLoop* m_pstEventLoop, uint32_t dwAvgSeqSize = 40) noexcept;
    ~AnConn();

    //int                         TryConnect(const char* szServerAddress, uint16_t iServerPort) noexcept;
    int                         TrySend(uint8_t* pstBytes, uint16_t size) noexcept;

    void                        SetRCallback(MSGHANDLE stFun);
    void                        SetWCallback(MSGHANDLE stFun);
    int                         GetConnType();                  
    //int                         PreRun() noexcept;
    int                         OnInit() noexcept;
private:
    void                        Close() noexcept;
private:
    uint64_t                    m_ulUid;
    std::unique_ptr<AnSocket>   m_pstClientSocket;
    std::unique_ptr<AnEvent>    m_pstClientEvent;
    std::unique_ptr<AnBuffer>   m_pstInBuffer;
    std::unique_ptr<AnBuffer>   m_pstOutBuffer;
    uint32_t                    m_dwAvgSeqSize;
    AnMod*                      m_pstMod;

    MSGHANDLE                   ReadCallback;
    MSGHANDLE                   WriteCallback;
};


}}
