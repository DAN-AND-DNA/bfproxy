#include <src/BfDBServer.h>
#include <src/BfThread.h>
#include <msg/WorkerMsg.h>
#include <eventloop/AnEvent1.h>
#include <eventloop/AnSocket1.h>
#include <eventloop/AnConn1.h>
#include <pool/LuPool.hpp>
#include <arpa/inet.h>

namespace dan { namespace bf {
BfDBServer::BfDBServer(const char* szServerAddress, uint16_t dwServerPort) noexcept:
    AnMod1(szServerAddress, dwServerPort),
    m_ulSessionIdGen(2019),
    m_stConns(),
    m_pstThreadPool(new dan::pool::LuPool<dan::bf::BfThread>("workerpool", 2 * std::thread::hardware_concurrency()))        // for io task
{
    
    // init worker
    if(dan::bf::BfThread::OnInit(512, 1024 * 8, this->GetEpollFd(), 2 * std::thread::hardware_concurrency()) != 0)
    {
        dan::log::BFLog::ERR("init worker pool failed");
        ::exit(-1);
    }

    // accept new conn
    m_pstServerEvent->FiredRead([this](){
        this->m_ulSessionIdGen++;
        auto pstConn = this->Accept(this->m_ulSessionIdGen);
        m_stConns[this->m_ulSessionIdGen] = std::move(pstConn);
        dan::log::BFLog::INFO("new client:{}", this->m_ulSessionIdGen);
      
    });

    // break from loop
    m_pstWorkerEvent->FiredRead([this](){
    
    });

    m_pstServerEvent->EnableRead();
    m_pstWorkerEvent->EnableRead();
}

BfDBServer::~BfDBServer()
{
    dan::log::BFLog::WARN("server is closing...");
    BfThread::Stop();
}

void BfDBServer::KickConn(uint64_t ulSessionId)
{
    m_stConns.erase(ulSessionId);
}

int BfDBServer::HandleC2S(uint8_t* pstBytes, uint64_t ulSize, uint64_t ulSessionId, int iConnFd)
{
    // parse protocol
    
    //dan::log::BFLog::INFO("dispatch1 msg:{}", common::Now2Us());
    dan::log::BFLog::INFO("dispatch msg");
    if(ulSize < 5)
    {
        return 0;
    }
    uint16_t dwLen = ::ntohs(*(reinterpret_cast<uint16_t*>(pstBytes)));
    uint8_t  dwCmd = *(reinterpret_cast<uint8_t*>(pstBytes + 2));
    uint16_t dwSeq = ::ntohs(*(reinterpret_cast<uint16_t*>(pstBytes + 3)));
    if(static_cast<uint64_t>(dwLen + 5) > ulSize)
    {
        return 0;
    }


    struct dan::msg::WorkerMsg* pstMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&dan::bf::BfThread::stWorkerQueue));
 
    pstMsg->dwLen = dwLen;
    pstMsg->dwCmd = dwCmd;
    pstMsg->dwSeq = dwSeq;
    pstMsg->ulSessionId = ulSessionId;
    pstMsg->iConnFd = iConnFd; 
    
    ::memcpy(pstMsg->pstData, pstBytes + 5, dwLen);
    
    ::message_queue_write(&dan::bf::BfThread::stWorkerQueue, pstMsg);
    //dan::log::BFLog::INFO("dispatch2 msg:{}", common::Now2Us());
  

    return dwLen + 5;
}

int BfDBServer::MakeSure()
{
    auto pstThread = m_pstThreadPool->GetIdle();
  
    if(pstThread->GetID() == std::thread::id())
    {
        dan::log::BFLog::ERR("create worker pool failed");
        return -1;
    }
  
    m_pstThreadPool->GiveBack(std::move(pstThread));
    dan::log::BFLog::INFO("worker: {}", m_pstThreadPool->IdleSize());
    return 0;
}

void BfDBServer::Run()
{
    if(this->MakeSure() == 0)
    {   
        AnMod1::Run();
    }
}


}}
