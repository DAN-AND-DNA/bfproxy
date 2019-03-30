#pragma once

#include <memory>
#include <eventloop/Common.h>
#include <common/utils.hpp>
#include <vector>

namespace dan { namespace eventloop {
class EventLoop1;
class AnBuffer;

class AnEvent1 //: public std::enable_shared_from_this<AnEvent1>
{
public:
    AnEvent1() = delete;
    AnEvent1(int iFd, EventLoop1* pstEventLoop, int iEventType) noexcept;
    ~AnEvent1() noexcept;
    int         GetFd(){return m_iFd;}
    uint32_t    GetActions(){return m_dwActions;}
    int         GetEventType();
   
    void        FiredRead (CALLBACK stCallback){m_stReadCallback  = std::move(stCallback);}
    void        FiredWrite(CALLBACK stCallback){m_stWriteCallback = std::move(stCallback);}
    
    void        FiredRead() {if(likely(m_stReadCallback))  m_stReadCallback();}
    void        FiredWrite(){if(likely(m_stWriteCallback)) m_stWriteCallback();}
    
    int         EnableRead()    noexcept;        
    int         DisableRead()   noexcept;        
    int         EnableWrite()   noexcept;       
    int         DisableWrite()  noexcept;       
    int         DisableAll()    noexcept;
private:
    int                         m_iFd;
    EventLoop1*                 m_pstEventLoop;
    uint32_t                    m_dwActions;
    CALLBACK                    m_stReadCallback;
    CALLBACK                    m_stWriteCallback;
    std::unique_ptr<AnBuffer>   m_pstInBuffer;
    std::unique_ptr<AnBuffer>   m_pstOutBuffer;
    int                         m_iEventType;
};

}}
