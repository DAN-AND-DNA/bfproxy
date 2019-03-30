#pragma once

#include <common/utils.hpp>
#include <cstdio>

namespace dan { namespace eventloop {


class AnSocket: common::NonCopyable
{
public:
    AnSocket(bool bIsServer, int iPort = 7777, const char* szAddress = "127.0.0.1") noexcept;
    AnSocket(int iClientFd) noexcept;
    ~AnSocket();
   // bool IsServer(){return m_bIsServer;}
    int Fd(){return m_iOwnerFd;}
    int GetFd(){return m_iOwnerFd;}
    int GetSocketType(){if (m_bIsServer) return SOCK_SERVER; else return SOCK_CLIENT;}
    
    int Accept();
    ssize_t Recv(void* pstBuffer, size_t dwLength);
    ssize_t Read(void* pstBuffer, size_t dwLength);
    ssize_t Send(void* pstBuffer, size_t dwLength);
    ssize_t Write(void* pstBuffer, size_t dwLength);
    
    static ssize_t Recv(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Read(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Send(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Write(int fd, void* pstBuffer, size_t dwLength);

private:
    int  m_iOwnerFd;
    int  m_iPort;
    bool m_bIsServer;

};


}}
