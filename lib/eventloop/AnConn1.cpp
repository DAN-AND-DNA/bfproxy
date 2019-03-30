#include <eventloop/AnConn1.h>
#include <eventloop/AnEvent1.h>
#include <eventloop/AnMod1.h>
#include <eventloop/AnBuffer.h>
#include <eventloop/AnSocket1.h>

namespace dan { namespace eventloop {

AnConn1::AnConn1(AnMod1* pstMod, std::shared_ptr<AnEvent1>& pstEvent, uint64_t ulSessionId):
    m_pstMod(pstMod),
    m_pstEvent(pstEvent),
    m_pstInBuffer(new dan::eventloop::AnBuffer(65536)),
    m_pstOutBuffer(new dan::eventloop::AnBuffer(65536)),
    m_iFd(-1),
    m_ulSessionId(ulSessionId)
{
    if(m_pstEvent != nullptr && m_pstEvent->GetFd() > 0)
    {
        m_iFd = m_pstEvent->GetFd();
    
        m_pstEvent->FiredRead([this](){
            auto writeable = m_pstInBuffer->Writeable();
            auto readbale = m_pstInBuffer->Readable();
            auto processed = m_pstInBuffer->Processed();
            
            dan::log::BFLog::INFO("size:{} writeable:{} readbale:{} processed:{}", m_pstInBuffer->Size(), writeable, readbale, processed);
            if(unlikely(writeable <= 100))
            {
                if(processed >= readbale)
                {
                    ::memcpy(m_pstInBuffer->BeginPtr(), m_pstInBuffer->ReadPtr(), readbale);
                    m_pstInBuffer->SetReadIndex(0);
                    m_pstInBuffer->SetWriteIndex(readbale);
                }
                else
                {
                   if(m_pstInBuffer->MakeSpace() == false) 
                   {
                        dan::log::BFLog::ERR("too many msg, be attacked ?!");
                        this->Close();
                        return;
                   }
                }
                writeable = m_pstInBuffer->Writeable();
            }
            auto recved = dan::eventloop::AnSocket1::Recv(m_iFd, m_pstInBuffer->WritePtr(), writeable);
            auto writeIndex = m_pstInBuffer->WriteIndex();

            if(likely(recved > 0))
            {
                dan::log::BFLog::INFO("recv {} bytes", recved);
                m_pstInBuffer->SetWriteIndex(writeIndex + recved);
                auto readable = m_pstInBuffer->Readable();
                auto res = m_pstMod->HandleC2S(m_pstInBuffer->ReadPtr(), readable, this->m_ulSessionId, this->m_iFd);
                
                if(likely(res > 0))
                {
                    m_pstInBuffer->SetReadIndex(m_pstInBuffer->ReadIndex() + res);
                }
                else if(res == 0)
                {
                    // do nothing
                }
                else
                {
                    dan::log::BFLog::ERR("bad req");
                    this->Close();
                }
            }
            else if(recved == 0)
            {
                dan::log::BFLog::INFO("client{} go away:", this->m_ulSessionId);
                this->Close();
            }
            else
            {
                if(errno != EAGAIN && errno != EINTR)
                {
                    dan::log::BFLog::ERR("recv failed:{}", strerror(errno));
                    dan::log::BFLog::INFO("client{} go away:", this->m_ulSessionId);
                    this->Close();
                }
            }

        });

        m_pstEvent->FiredWrite([this](){
            auto readable = m_pstOutBuffer->Readable();
            if(readable == 0)
            {
                m_pstEvent->DisableWrite();
                return;
            }

            auto sended = AnSocket1::Send(m_iFd, m_pstOutBuffer->ReadPtr(), readable);

            if(likely(sended >= 0))
            {
                m_pstOutBuffer->SetReadIndex(m_pstOutBuffer->ReadIndex() + sended);
                if(readable == static_cast<uint64_t>(sended))
                {
                    m_pstEvent->DisableWrite();
                    return;
                }
            }
            else
            {
                dan::log::BFLog::ERR("send failed:{}", strerror(errno));
                this->Close();
            }
        });
        m_pstEvent->EnableRead();

    }

}

AnConn1::~AnConn1()
{
}

int AnConn1::TrySend(uint8_t* pstBytes, uint64_t ulSize)
{
    if(pstBytes == nullptr || ulSize == 0)
    {
        return 0;
    }

    auto sended = AnSocket1::Send(m_iFd, pstBytes, ulSize);

    if(sended >= 0)
    {
        m_pstOutBuffer->SetReadIndex(m_pstOutBuffer->ReadIndex() + sended);
        if(static_cast<uint64_t>(sended) < ulSize)
        {
            ::memcpy(m_pstOutBuffer->WritePtr(), pstBytes + sended, ulSize - sended);
            m_pstOutBuffer->SetWriteIndex(m_pstOutBuffer->WriteIndex() + ulSize - sended);
            m_pstEvent->EnableWrite();
        }
    }
    else
    {
        dan::log::BFLog::ERR("try send failed:{}", strerror(errno));
        return -1;
    }
    return 0;
}

void AnConn1::Close()
{
    if(m_pstMod == nullptr)
    {
        return;
    }
    //dan::log::BFLog::INFO("close client");
    m_pstMod->KickConn(this->m_ulSessionId);
}

}}
