#pragma once

#include <thread>
#include <threadmq/message_queue.h>
#include <atomic>

namespace dan { namespace db {
class AnMysql;
}}

namespace dan { namespace pool {
template<typename T>
class LuPool;
}}

namespace dan { namespace bf {

#define SELECT_BY_USERID 0
#define INSERT 1
#define SUCCESS 2
#define ERROR 3
#define BADCMD 4
#define SELECT_BY_OPENID 5
#define INSERT1 6
#define SELECT_BY_OPENID1 7

#define SO_BIG 6
#define DB_ERROR 7
#define PENDING 8
#define CLOSE 9

 
class BfThread
{
public:
    BfThread();
    ~BfThread();

    std::thread::id GetID() {return std::this_thread::get_id();}
    static int OnInit(int iMaxMsgSize, int iMaxQueueSize, int iEpollFd, int iWorkerSize);
    static struct message_queue                           stWorkerQueue;
    static struct message_queue                           stPendingQueue;
    static void OnDestory();
    static void Stop();
private:
    void DoJob();
private:
    std::thread                                             m_stThread;
    uint8_t                                                 m_stC2SBuffer[1024];
    uint8_t                                                 m_stS2SBuffer[1024];
    static int                                              iEpollFd;
    static int                                              iWorkerSize;
    static std::atomic<int>                                 iClosedSize;
};


}}
