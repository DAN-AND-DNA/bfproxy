#include <eventloop/AnConn.h>
#include <eventloop/AnSocket.h>
#include <eventloop/AnEvent.h>
#include <eventloop/AnBuffer.h>
#include <eventloop/TcpServer.h>
#include <eventloop/AnMod.h>

#include <iostream> // for test
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

namespace dan { namespace eventloop {

AnConn::AnConn(AnMod* pstMod, int iClientFd, uint64_t ulUid, EventLoop* m_pstEventLoop, uint32_t dwAvgSeqSize) noexcept:
    m_ulUid(ulUid),
    m_pstClientSocket(new AnSocket(iClientFd)),
    m_pstClientEvent(new AnEvent(iClientFd, m_pstEventLoop)),
    m_pstInBuffer(new AnBuffer(65536*2)),
    m_pstOutBuffer(new AnBuffer(65536*2)),
    m_dwAvgSeqSize(dwAvgSeqSize),
    m_pstMod(pstMod)
{
    OnInit();
}

AnConn::AnConn(AnMod* pstMod, uint64_t ulUid, EventLoop* m_pstEventLoop, uint32_t dwAvgSeqSize) noexcept:
    m_ulUid(ulUid),
    m_pstClientSocket(new AnSocket(false)),
    m_pstClientEvent(new AnEvent(m_pstClientSocket->Fd(), m_pstEventLoop)),
    m_pstInBuffer(new AnBuffer(65536*2)),
    m_pstOutBuffer(new AnBuffer(65536*2)),
    m_dwAvgSeqSize(dwAvgSeqSize),
    m_pstMod(pstMod)
{
    OnInit();
}

AnConn::~AnConn()
{}
/*
int AnConn::TryConnect(const char* szServerAddress, uint16_t iServerPort) noexcept
{
    struct sockaddr_in stClientAddr;
    ::bzero(&stClientAddr, sizeof(stClientAddr));
    stClientAddr.sin_family = AF_INET;
    stClientAddr.sin_port   = ::htons(static_cast<uint16_t>(iServerPort));
    ::inet_pton(AF_INET, szServerAddress, &stClientAddr.sin_addr);
    ::socklen_t stSockLen = sizeof(stClientAddr);

    //TODO SET NONBLOCK
    
    // try connect directly
    printf("try connect:%s:%d \n", szServerAddress, iServerPort);
    int iRe = ::connect(m_pstClientSocket->Fd(), reinterpret_cast<struct sockaddr*>(&stClientAddr), stSockLen);

    if(iRe != 0 && errno == EINPROGRESS)
    {
        // waiting for epoll
        printf("交给epoll确认connect:\n");
        m_pstClientEvent->FiredWrite([this](){
            m_pstClientEvent->SubscribeWrite(false);
        });

        m_pstClientEvent->SubscribeWrite(true);
        return 1;
    }
    else if(iRe == 0)
    {
        printf("直接connect\n");
        return 0;
    }
    else if(iRe != 0)
    {
        printf("进行connect发生错误:%s\n", strerror(errno));
        return -1;
    }

    return -1;
}
*/

void AnConn::SetRCallback(MSGHANDLE stFun)
{
    ReadCallback = stFun;
}

void AnConn::SetWCallback(MSGHANDLE stFun)
{
    WriteCallback = stFun;
}


int AnConn::GetConnType()
{
    if(m_pstClientSocket != nullptr)
    {
        return m_pstClientSocket->GetSocketType();
    }
    return -1;
}


int AnConn::OnInit() noexcept
{
    if(this->GetConnType() == -1)
    {
        this->Close();
    }
    else if(this->GetConnType() == CONN_CLIENT)
    {
        m_pstMod->BuildC2SMsg(this->m_ulUid);
    }


    // read
    m_pstClientEvent->FiredRead([this](){
         if(this->m_pstClientEvent == nullptr) printf("aaaaaaa1\n");
        
        auto ulWriteable  = m_pstInBuffer->Writeable();
        auto ulReadable = m_pstInBuffer->Readable();
        auto ulWriteIndex = m_pstInBuffer->WriteIndex();

       
        if(ulWriteable <= (m_dwAvgSeqSize * 5))
        {
            if(m_pstInBuffer->Processed() >= (m_dwAvgSeqSize * 3))
            {
                // move to front
                ::memcpy(m_pstInBuffer->BeginPtr(), m_pstInBuffer->ReadPtr(), ulReadable);
            }

            ulWriteable = m_pstInBuffer->Writeable();
            if(ulWriteable <= (m_dwAvgSeqSize * 5))
            {
                // resize to x2
                if(m_pstInBuffer->MakeSpace() == false)
                {
                    // no more space (may be under attacked)
                    // FIXME here just kill
                    printf("10002\n");
                    this->Close();
                }
            }
        }   
        
        ulWriteable  = m_pstInBuffer->Writeable();
        auto lCount = m_pstClientSocket->Recv(m_pstInBuffer->WritePtr(), ulWriteable);
        
        //auto now = std::chrono::high_resolution_clock::now();
        
        if(lCount == 0)
        {
            printf("client close\n");
            this->Close();
        }
        else if(lCount > 0)
        {
            m_pstInBuffer->SetWriteIndex(ulWriteIndex + lCount);

            //=========================== just test, do not safety=============================
            //
            auto ulReadable = m_pstInBuffer->Readable();
            printf("receive %lu bytes\n", ulReadable);

            // deal msg
            int iDone = 0;

            switch(this->GetConnType())
            {
                case CONN_SERVER:
                    iDone = m_pstMod->HandleC2SMsg(m_pstInBuffer->ReadPtr(), ulReadable, this->m_ulUid);
                    break;
                case CONN_CLIENT:
                    iDone = m_pstMod->HandleS2CMsg(m_pstInBuffer->ReadPtr(), ulReadable, this->m_ulUid);
                    break;
            }

            if(iDone > 0)
            {
                m_pstInBuffer->SetReadIndex(m_pstInBuffer->ReadIndex() + iDone);
                printf("left:%lu\n", m_pstInBuffer->Readable());
                printf("size:%lu\n", m_pstInBuffer->Size());
                printf("index:%lu\n", m_pstInBuffer->ReadIndex());
            }
            else if(iDone < 0)
            {
                printf("10001\n");
                this->Close();
            }
        }
        else
        {
            if(errno != EAGAIN && errno != EINTR)
            {
                //TODO error!  just close
                printf("geterror1001:%s\n", strerror(errno));
                this->Close();
            }
        }

    });

    // write
    m_pstClientEvent->FiredWrite([this](){
        
        if(this->m_pstClientEvent != nullptr) printf("aaaaaaa1\n");
        auto ulReadable = m_pstOutBuffer->Readable();
        
        if(ulReadable == 0)
        {
            m_pstClientEvent->SubscribeWrite(false);
            return;
        }

        auto ulSended = m_pstClientSocket->Send(m_pstOutBuffer->ReadPtr(), ulReadable);

        if(ulSended > 0)
        {
            m_pstOutBuffer->SetReadIndex(m_pstOutBuffer->ReadIndex() + ulSended);
            if(m_pstOutBuffer->Readable() == 0)
            {
                m_pstClientEvent->SubscribeWrite(false);
                return;
            }
        }
        else if(ulSended < 0)
        {
            // RST
            this->Close();
        }

    });

    // error
    m_pstClientEvent->FiredError([this](){
    });

    // close
    m_pstClientEvent->FiredClose([this](){
       //     printf("e4\n");
      //      this->Close();
    });

    m_pstClientEvent->SubscribeRead(true);

    return 0;
}

void AnConn::Close() noexcept{printf("ea\n");   m_pstClientEvent->SubscribeNone(); m_pstMod->KickConn(m_ulUid);}


int  AnConn::TrySend(uint8_t* pstBytes, uint16_t dwSize) noexcept
{
    if(pstBytes == nullptr || dwSize == 0)
        return 0;

    ::memcpy(m_pstOutBuffer->WritePtr(), pstBytes, dwSize);
    m_pstOutBuffer->SetWriteIndex(m_pstOutBuffer->WriteIndex() + dwSize);

    auto sended = m_pstClientSocket->Send(m_pstOutBuffer->ReadPtr(), dwSize);

    printf("send:%lu\n", sended);

    if(sended < 0)
    {
        return -1;
    }
    else if(sended > 0)
    {
        m_pstOutBuffer->SetReadIndex(m_pstOutBuffer->ReadIndex() + sended);
        if(m_pstOutBuffer->Readable() == 0)
        {
            printf("left none\n");
            m_pstClientEvent->SubscribeWrite(false);
        }
        else
        {
            m_pstClientEvent->SubscribeWrite(true);
        }

        return 0;
    }
    return -1;
}


}}
