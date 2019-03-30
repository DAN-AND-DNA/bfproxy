#include <src/BfThread.h>
#include <memory>
#include <db/AnMysql.h>
#include <pool/LuPool.hpp>
#include <common/utils.hpp>
#include <msg/DBMsg.h>
#include <msg/WorkerMsg.h>
#include <eventloop/AnSocket.h>
#include <string.h>
#include <arpa/inet.h>

namespace {
    std::shared_ptr<dan::pool::LuPool<dan::db::AnMysql>> m_pstDBPool(new dan::pool::LuPool<dan::db::AnMysql>("dbpool", 30));
}

namespace dan { namespace bf {

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
struct message_queue BfThread::stWorkerQueue = {};
struct message_queue BfThread::stPendingQueue = {};
#pragma GCC diagnostic error "-Wmissing-field-initializers"
int BfThread::iEpollFd = -1;
int BfThread::iWorkerSize = 12;
std::atomic<int> BfThread::iClosedSize(0);


BfThread::BfThread():
    m_stThread(),
    m_stC2SBuffer(),
    m_stS2SBuffer()
{
    m_stThread = std::move(std::thread([this](){this->DoJob();}));
}

int BfThread::OnInit(int iMaxSize, int iMaxQueueSize, int iEpollFd, int iWorkerSize)
{
    if (::message_queue_init(&stWorkerQueue, iMaxSize, iMaxQueueSize) != 0 || ::message_queue_init(&BfThread::stPendingQueue, iMaxSize, iMaxQueueSize) != 0)
    {
        return -1;
    }
    BfThread::iEpollFd = iEpollFd;
    BfThread::iWorkerSize = iWorkerSize;
    return 0;
}

void BfThread::OnDestory()
{
    dan::log::BFLog::WARN("closing worker msg queue...");
    while(true)
    {
        if(BfThread::iClosedSize.load() == BfThread::iWorkerSize)
        {
            break;
        }
        sleep(1);
    }
    ::message_queue_destroy(&BfThread::stWorkerQueue);
    ::message_queue_destroy(&BfThread::stPendingQueue);
}

void BfThread::Stop()
{
    for(int i = 0; i < BfThread::iWorkerSize; ++i)
    {
        struct dan::msg::WorkerMsg* pstMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&dan::bf::BfThread::stWorkerQueue));
        pstMsg->dwCmd = CLOSE;
        ::message_queue_write(&dan::bf::BfThread::stWorkerQueue, pstMsg);
    }

    BfThread::OnDestory();
}


BfThread::~BfThread()
{
    m_stThread.join();
    dan::log::BFLog::INFO("close woker success");
}

void BfThread::DoJob()
{
    auto run = true;
    auto pstDBConn = m_pstDBPool->GetIdle();
    uint64_t i = 1;
        
    while(run)
    {
        // block here
        struct dan::msg::WorkerMsg* pstMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_read(&stWorkerQueue));
        auto unfinished = true;
        while(unfinished)
        {
            switch(pstMsg->dwCmd)
            {
                // select by userid
                case SELECT_BY_USERID:
                {   
                    uint64_t userid = *(reinterpret_cast<uint64_t*>(pstMsg->pstData));
                    int  iConnFd = pstMsg->iConnFd;
                    uint16_t dwReqSeq = pstMsg->dwSeq;
                    
                    if(likely(iConnFd > 0))
                    {
                        dan::log::BFLog::INFO("select by userid:{}", userid);
                        int iResLen = pstDBConn->Select(userid, this->m_stS2SBuffer);
                        if(likely(iResLen >= 0 && iResLen < 512))
                        {
                            // db op success
                            uint8_t dwCmd =  SUCCESS;
                            uint16_t dwLen = ::htons(static_cast<uint16_t>(iResLen));
                            uint16_t dwResSeq = ::htons(dwReqSeq);
                            ::memcpy(m_stC2SBuffer, &dwLen, 2);
                            ::memcpy(m_stC2SBuffer + 2, &dwCmd, 1);
                            ::memcpy(m_stC2SBuffer + 3, &dwResSeq, 2);
                            ::memcpy(m_stC2SBuffer + 5, this->m_stS2SBuffer, dwLen);
                    
                            auto sended = dan::eventloop::AnSocket::Send(iConnFd, this->m_stC2SBuffer, dwLen + 5); 
                            
                            if(likely(sended == dwLen + 5))
                            {
                            }
                            else if(sended < 0)
                            {
                                // error notify main thread close this fd
                                struct dan::msg::WorkerMsg* pstErrorMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&BfThread::stPendingQueue));
                                pstErrorMsg->iErrno = errno;
                                pstErrorMsg->iConnFd = pstMsg->iConnFd;
                                pstErrorMsg->ulSessionId = pstMsg->ulSessionId;
                                pstErrorMsg->dwCmd = ERROR;
                                ::message_queue_write(&BfThread::stPendingQueue, pstErrorMsg);
                                auto n = dan::eventloop::AnSocket::Write(BfThread::iEpollFd, &i, sizeof i);
                                if(n != sizeof i)
                                {
                                    ::message_queue_message_free(&BfThread::stPendingQueue, pstErrorMsg);
                                }
                            }
                            else if(dwLen + 5 > sended)
                            {
                                // notify main thread the socket buffer is full
                                struct dan::msg::WorkerMsg* pstPendingMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&BfThread::stPendingQueue));
                                pstPendingMsg->iConnFd = pstMsg->iConnFd;
                                pstPendingMsg->ulSessionId = pstMsg->ulSessionId;
                                pstPendingMsg->dwCmd = PENDING;
                                ::memcpy(pstPendingMsg->pstData, this->m_stC2SBuffer + sended, (dwLen + 5 - sended));

                                ::message_queue_write(&BfThread::stPendingQueue, pstPendingMsg);
                                auto n = dan::eventloop::AnSocket::Write(BfThread::iEpollFd, &i, sizeof i);
                                if(n != sizeof i)
                                {
                                    ::message_queue_message_free(&BfThread::stPendingQueue, pstPendingMsg);
                                }
                            }

                            unfinished = false;
                            break;
                        }
                        else if(iResLen >= 512)
                        {
                            //TODO so big
                        }
                        else
                        {
                            //db failed
                            pstDBConn = nullptr;
                            pstDBConn = m_pstDBPool->GetIdle();
                           
                            unfinished = true;
                            break;
                        }
                    }
                    else
                    {
                        // bad fd
                        unfinished = false;
                        break;
                    }

                }
                case SELECT_BY_OPENID:
                {
                    std::string strOpenid(reinterpret_cast<char*>(pstMsg->pstData), pstMsg->dwLen);
                    int  iConnFd = pstMsg->iConnFd;
                    uint16_t dwReqSeq = pstMsg->dwSeq;
                   
                    //dan::log::BFLog::INFO("select by openid:{} {}", strOpenid, common::Now2Us());
                    auto iResLen = pstDBConn->Select(strOpenid, this->m_stS2SBuffer);
                    //int iResLen = 0;

                    //int iResLen = 0;
                    if(iConnFd > 0)
                    {
                        if(likely(iResLen >= 0 && iResLen < 512))
                        {
                            uint8_t dwCmd = SUCCESS;
                            uint16_t dwLen = ::htons(static_cast<uint16_t>(iResLen));
                            uint16_t dwResSeq = ::htons(dwReqSeq);
                            ::memcpy(m_stC2SBuffer, &dwLen, 2);
                            ::memcpy(m_stC2SBuffer + 2, &dwCmd, 1);
                            ::memcpy(m_stC2SBuffer + 3, &dwResSeq, 2);
                            ::memcpy(m_stC2SBuffer + 5, this->m_stS2SBuffer, iResLen);

                            auto sended =  dan::eventloop::AnSocket::Send(iConnFd, this->m_stC2SBuffer, iResLen + 5);


                            if(likely(sended == iResLen + 5))
                            {
                            }
                            else if(sended < 0)
                            {
                                // error notify main thread close this fd
                                struct dan::msg::WorkerMsg* pstErrorMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&BfThread::stPendingQueue));
                                pstErrorMsg->iErrno = errno;
                                pstErrorMsg->iConnFd = pstMsg->iConnFd;
                                pstErrorMsg->ulSessionId = pstMsg->ulSessionId;
                                pstErrorMsg->dwCmd = ERROR;
                                ::message_queue_write(&BfThread::stPendingQueue, pstErrorMsg);
                                auto n = dan::eventloop::AnSocket::Write(BfThread::iEpollFd, &i, sizeof i);
                                if(n != sizeof i)
                                {
                                    ::message_queue_message_free(&BfThread::stPendingQueue, pstErrorMsg);
                                }

                            }
                            else if(iResLen + 5 > sended)
                            {
                                // notify main thread the socket buffer is full
                                struct dan::msg::WorkerMsg* pstPendingMsg = static_cast<struct dan::msg::WorkerMsg*>(::message_queue_message_alloc_blocking(&BfThread::stPendingQueue));
                                pstPendingMsg->iConnFd = pstMsg->iConnFd;
                                pstPendingMsg->ulSessionId = pstMsg->ulSessionId;
                                pstPendingMsg->dwCmd = PENDING;
                                ::memcpy(pstPendingMsg->pstData, this->m_stC2SBuffer + sended, (dwLen + 5 - sended));

                                ::message_queue_write(&BfThread::stPendingQueue, pstPendingMsg);
                                auto n = dan::eventloop::AnSocket::Write(BfThread::iEpollFd, &i, sizeof i);
                                if(n != sizeof i)
                                {
                                    ::message_queue_message_free(&BfThread::stPendingQueue, pstPendingMsg);
                                }
                            }
                            unfinished = false;
                            break;
                        }
                        else if(iResLen >= 512)
                        {
                            //TODO too big
                            unfinished = false;
                            break;
                        }
                        else
                        {
                            //db failed
                            pstDBConn = nullptr;
                            pstDBConn = m_pstDBPool->GetIdle();
                           
                            unfinished = true;
                            break;
                        }
                    }
                    else
                    {
                        // bad fd
                        unfinished = false;
                        break;
                    }
                }
                case INSERT:
                {
                    uint64_t ulUserid = *(reinterpret_cast<uint64_t*>(pstMsg->pstData));
                    uint8_t dwOpenidLen = *(reinterpret_cast<uint8_t*>(pstMsg->pstData + 8));
                    std::string strOpenid(reinterpret_cast<char*>(pstMsg->pstData + 9), dwOpenidLen);

                    dan::log::BFLog::DEBUG("insert openid len:{}, userid:{}, openid:{}\n", dwOpenidLen, ulUserid, strOpenid);
                    auto iResLen = pstDBConn->Insert(ulUserid, strOpenid, pstMsg->pstData + 9 + dwOpenidLen, pstMsg->dwLen - 9 - dwOpenidLen);
                    if(likely(iResLen == 0))
                    {
                     // do nothing
                        unfinished = false;
                        break;
                    }      
                    else
                    {
                        pstDBConn = nullptr;
                        pstDBConn = m_pstDBPool->GetIdle();
                        unfinished = true;
                        break;
                    }
                }
                case CLOSE:
                {
             //       printf("get close msg\n");
                    run = false;
                    unfinished = false;
                    iClosedSize.fetch_add(1, std::memory_order_relaxed);
                    break; 
                }
                default:
                {
                    unfinished = false;
                    break;
                }
            }
        }
        //printf("clean msg\n");
        message_queue_message_free(&BfThread::stWorkerQueue, pstMsg);
    }
    if(pstDBConn != nullptr)
    {
       m_pstDBPool->GiveBack(std::move(pstDBConn));
    }
}




}}
