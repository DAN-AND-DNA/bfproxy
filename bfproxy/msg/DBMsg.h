#pragma once

#include <stdint.h>
#include <vector>

namespace dan { namespace msg {

struct DBMsg {
    uint8_t                 dwCmd;
    uint16_t                dwLen;
    uint16_t                dwSeq;
    int                     iConnFd;
    int                     iErrno;
    uint64_t                ulConnId;
    uint8_t                 pstData[512];
};



}}
