#pragma once

#include <map>
#include <memory>

namespace dan { namespace eventloop {
class AnMod1;
class AnEvent1;
class AnBuffer;

class AnConn1
{
public:
    AnConn1(dan::eventloop::AnMod1* pstMod, std::shared_ptr<dan::eventloop::AnEvent1>& pstEvent, uint64_t ulSessionId);
    ~AnConn1();
    int  TrySend(uint8_t* pstBytes, uint64_t dwSize);
    void Close();
private:
    dan::eventloop::AnMod1*                   m_pstMod;
    std::shared_ptr<dan::eventloop::AnEvent1> m_pstEvent;
    std::unique_ptr<dan::eventloop::AnBuffer> m_pstInBuffer;
    std::unique_ptr<dan::eventloop::AnBuffer> m_pstOutBuffer;
    int                                       m_iFd;
    uint64_t                                  m_ulSessionId;
};

}}
