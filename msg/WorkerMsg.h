#pragma once

#include <stdint.h>
#include <vector>

namespace dan { namespace msg {

struct WorkerMsg {
    uint8_t                 dwCmd;
    uint16_t                dwLen;
    uint16_t                dwSeq;
    int                     iConnFd;
    int                     iErrno;
    uint64_t                ulSessionId;
    uint8_t                 pstData[512];
};



}}
