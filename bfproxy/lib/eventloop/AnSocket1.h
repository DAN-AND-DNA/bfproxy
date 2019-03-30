#pragma once

#include <stdint.h>
#include <stdio.h>


namespace dan { namespace eventloop {

class AnSocket1
{
public:
    static int MakeSocket(int iType, const char* szAddress, uint16_t iPort);
    static int Accept(int iServerFd);
    
    static ssize_t Recv(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Read(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Send(int fd, void* pstBuffer, size_t dwLength);
    static ssize_t Write(int fd, void* pstBuffer, size_t dwLength);


};

}}
