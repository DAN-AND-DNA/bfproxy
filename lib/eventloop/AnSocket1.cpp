#include <eventloop/AnSocket1.h>
#include <eventloop/Common.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <log/BFLog.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

namespace dan { namespace eventloop {
int AnSocket1::MakeSocket(int iType, const char* szAddress, uint16_t iPort)
{
    switch(iType)
    {
        case SOCKET_SERVER:
        {
            struct ::addrinfo  stHints;
            struct ::addrinfo* pstResult;
            ::bzero(&stHints, sizeof(struct addrinfo));
            stHints.ai_family   = AF_INET;
            stHints.ai_socktype = SOCK_STREAM;
            stHints.ai_flags    = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV;
            char szPort[6] = {0};
            ::snprintf(szPort, 6, "%d", iPort);

            if(::getaddrinfo(szAddress, szPort, &stHints, &pstResult) < 0)
            {
                dan::log::BFLog::ERR("getaddrinfo failed:{}", ::strerror(errno));
                return -1;
            }
            int iServerFd = -1;

            for(struct ::addrinfo* pstCurr = pstResult; pstCurr != nullptr; pstCurr = pstResult->ai_next)
            {
                iServerFd = ::socket(pstCurr->ai_family, pstCurr->ai_socktype, pstCurr->ai_protocol);
                int iOptval = 1;
                if(iServerFd == -1 || 
                        ::setsockopt(iServerFd, SOL_SOCKET, SO_REUSEADDR, &iOptval, sizeof(iOptval)) == -1 ||
                        ::setsockopt(iServerFd, IPPROTO_TCP, TCP_NODELAY, &iOptval, sizeof(iOptval)) == -1 ||
                        ::bind(iServerFd, pstResult->ai_addr, pstResult->ai_addrlen) == -1 ||
                        ::listen(iServerFd, 128) == -1)
                {
                    dan::log::BFLog::ERR("init socket failed:{}", ::strerror(errno));
                    if(iServerFd != -1)
                    {
                        ::close(iServerFd);
                    }
                    continue;
                }      
            }

            if(pstResult != nullptr)
            {
                ::freeaddrinfo(pstResult);
                pstResult = nullptr;
            }

            if(iServerFd > 0)
            {
                int iFlags = ::fcntl(iServerFd, F_GETFD, 0);
                if(iFlags == -1)
                {   
                    return -1;
                }
                
                iFlags |= FD_CLOEXEC;
                
                if(::fcntl(iServerFd, F_SETFD, iFlags) == -1)
                {
                    return -1;
                }
                 
                return iServerFd;
            }
            return -1;
        }
        case SOCKET_CLIENT:
        {
            struct ::addrinfo  stHints;
            struct ::addrinfo* pstResult;

            ::bzero(&stHints, sizeof(struct addrinfo));
            stHints.ai_family   = AF_INET;
            stHints.ai_socktype = SOCK_STREAM;
            stHints.ai_flags    = AI_NUMERICHOST | AI_NUMERICSERV;
       
            char szPort[6] = {0};
            ::snprintf(szPort, 6, "%d", iPort);

            if(::getaddrinfo(szAddress, szPort, &stHints, &pstResult) < 0)
            {
                dan::log::BFLog::ERR("getaddrinfo failed: {}", ::strerror(errno));
                return -1;
            }

            int iClientFd = -1;
            for(struct ::addrinfo* pstCurr = pstResult; pstCurr != nullptr; pstCurr = pstResult->ai_next)
            {
                iClientFd = ::socket(pstCurr->ai_family, pstCurr->ai_socktype, pstCurr->ai_protocol);
                int iOptval = 1;
                if(iClientFd == -1 || 
                    ::setsockopt(iClientFd, SOL_SOCKET, SO_REUSEADDR, &iOptval, sizeof(iOptval)) == -1 ||
                    ::setsockopt(iClientFd, IPPROTO_TCP, TCP_NODELAY, &iOptval, sizeof(iOptval)) == -1)
                {
                    dan::log::BFLog::ERR("init failed: {}", ::strerror(errno));
                    if(iClientFd != -1)
                    {
                        ::close(iClientFd);
                    }
                    continue;
                }
            }

            if(pstResult != nullptr)
            {
                ::freeaddrinfo(pstResult);
                pstResult = nullptr;
            }
            
            if(iClientFd > 0)
            {
                int iFlags = ::fcntl(iClientFd, F_GETFD, 0);
                if(iFlags == -1)
                {   
                    return -1;
                }
                
                iFlags |= FD_CLOEXEC;
                
                if(::fcntl(iClientFd, F_SETFD, iFlags) == -1)
                {
                    return -1;
                }
                 
                return iClientFd;
            }
            return -1;
        }
        default:
            return -1;
    }
}


int AnSocket1::Accept(int iServerFd)
{
    if(iServerFd == -1)
    {
        return -1;
    }

    struct ::sockaddr_in stClientAddr;
    ::socklen_t stSocklen = sizeof(struct ::sockaddr_in);
    void* pstTmp = static_cast<void*> (&stClientAddr);

    int iClientFd = -1;
    while(true)
    {
        iClientFd = ::accept4(iServerFd, 
                              static_cast<struct sockaddr*>(pstTmp), 
                              &stSocklen, 
                              SOCK_CLOEXEC | SOCK_NONBLOCK);

        if(iClientFd == -1)
        {
            switch(errno)
            {
                case EINTR:
                case ECONNABORTED:      // client close
                case EAGAIN:
                case EPERM:
                case EMFILE:
                case EPROTO:
                {
                    return -1;
                }
                default:
                {
                    dan::log::BFLog::ERR("accept failed: {}", ::strerror(errno));
                    return -1;
                }
            }
        }
        else
        {
            break;
        }
    }
    return iClientFd;

}

ssize_t AnSocket1::Recv(int fd, void* pstBuffer, size_t dwLength)
{
    return ::recv(fd,  pstBuffer, dwLength, 0);
}

ssize_t AnSocket1::Read(int fd, void* pstBuffer, size_t dwLength)
{
    return ::read(fd,  pstBuffer, dwLength);
}



ssize_t AnSocket1::Send(int fd, void* pstBuffer, size_t dwLength)
{
    return ::send(fd,  pstBuffer, dwLength, 0);
}

ssize_t AnSocket1::Write(int fd, void* pstBuffer, size_t dwLength)
{

    return ::write(fd, pstBuffer, dwLength);
}


}}
