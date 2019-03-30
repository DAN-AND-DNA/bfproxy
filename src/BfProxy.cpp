#include <src/BfProxy.h>
#include <pool/LuPool.hpp>
#include <eventloop/EventLoop.h>
#include <eventloop/AnSocket.h>
#include <eventloop/AnEvent.h>
#include <common/utils.hpp>
#include <src/BfThread.h>
#include <iostream>
#include <arpa/inet.h>
#include <msg/DBMsg.h>
#include <eventloop/AnConn.h>

namespace dan { namespace bf {

BfProxy::BfProxy(const char* szServerAddress, uint16_t dwServerPort) noexcept:
    AnMod(szServerAddress, dwServerPort),
    m_pstThreadPool(new dan::pool::LuPool<dan::bf::BfThread>("workerpool", std::thread::hardware_concurrency())),
    m_stConns(),
    m_ulUidGen(1000)
{
    if(dan::bf::BfThread::OnInit(512, 1024 * 8, this->EpollFd(), std::thread::hardware_concurrency()) != 0)
    {
        ::exit(-1);
    }
    dan::log::BFLog::INFO("Init BfProxy succes......");
    
}

BfProxy::~BfProxy() noexcept
{
    dan::bf::BfThread::OnDestory();
}


int BfProxy::PreRun() noexcept
{
    //1. check threadpool
    auto pstThread = m_pstThreadPool->GetIdle();
    if(pstThread->GetID() == std::thread::id())
    {
        return -1;
    }

    m_pstThreadPool->GiveBack(std::move(pstThread));

    //2. init server event
    m_pstServerEvent->FiredRead([this](){
            m_ulUidGen++;
            auto pstConn = this->NewConn(m_ulUidGen);
            if(pstConn == nullptr)
            {
                return;
            }

            m_stConns[m_ulUidGen] = std::move(pstConn);

    });

    // db msg event
    m_pstDBMsgEvent->FiredRead([this](){
            this->GetDBMsg();
            while(true)
            {
                struct dan::msg::DBMsg* pstMsg = static_cast<struct dan::msg::DBMsg*>(::message_queue_tryread(&dan::bf::BfThread::stPendingQueue));
               
                if(pstMsg == NULL)
                {
                    break;
                }
                else
                {
                    // dispater msg
                    //auto ulConnId = pstMsg->ulConnId;
                    //auto dwLen = ::htons(pstMsg->dwLen);
                    //auto iConnFd = pstMsg->iConnFd;
                    auto dwCmd = pstMsg->dwCmd;
                    //auto iError = pstMsg->iErrno;
                    

                    /*
                    uint8_t buffer[pstMsg->dwLen + 5];
                    
                    ::memcpy(buffer, &dwLen, 2);
                    ::memcpy(buffer + 2, &(pstMsg->dwCmd), 1);
                    ::memcpy(buffer + 3, &dwSeq, 2);
                    ::memcpy(buffer + 5, pstMsg->pstData, pstMsg->dwLen);
                    auto pst = m_stConns[ulConnId];
                    if(pst != nullptr)
                    {
                        pst->TrySend(buffer,  static_cast<uint16_t>(5 + pstMsg->dwLen));
                    }
                    */

                    switch (dwCmd)
                    {
                        case ERROR:
                        {
                            break;
                        }
                        case PENDING:
                        {
                            break;
                        }
                    }

                    message_queue_message_free(&dan::bf::BfThread::stPendingQueue, pstMsg);
                }
            }

    });
    m_pstServerEvent->SubscribeRead(true);
    m_pstDBMsgEvent->SubscribeRead(true);

    return 0;
}

int BfProxy::KickConn(uint64_t ulUid)
{
    m_stConns.erase(ulUid);
    return 0;
}

int BfProxy::HandleS2CMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulConnId)
{
    if(ulSize < 5)
    {
        return 0;
    }

    uint16_t dwLen = ::ntohs(*(reinterpret_cast<uint16_t*>(pstBytes)));
    uint8_t  dwCmd = *(reinterpret_cast<uint8_t*>(pstBytes + 2));
    uint16_t dwSeq = ::ntohs(*(reinterpret_cast<uint16_t*>(pstBytes + 3)));

    if(ulSize - 5 < dwLen)
    {
        // need more bytes
        return 0;
    }
 
    struct dan::msg::DBMsg* pstMsg = static_cast<struct dan::msg::DBMsg*>(::message_queue_message_alloc_blocking(&dan::bf::BfThread::stWorkerQueue));
    pstMsg->dwLen = dwLen;
    pstMsg->dwCmd = dwCmd;
    pstMsg->dwSeq = dwSeq;
    pstMsg->ulConnId = ulConnId;
    pstMsg->iConnFd = -1;

    ::memcpy(pstMsg->pstData, pstBytes + 5, dwLen);
    ::message_queue_write(&dan::bf::BfThread::stWorkerQueue, pstMsg); 
     
    printf("dis1 msg:%lu\n", common::Now2Us());

    return dwLen + 5;
}

int BfProxy::HandleC2SMsg(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulRobotId)
{
    return 0;
}

int BfProxy::BuildC2SMsg(uint64_t ulRobotId)
{
    return 0;
}

void BfProxy::Run()
{
    if(this->PreRun() == 0)
    {
        m_pstEventLoop->Loop();
    }
    else
    {
        printf("init failed");
        exit(-1);
    }
}


}}
