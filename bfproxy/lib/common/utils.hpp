#pragma once

#include <chrono>

#include <log/BFLog.hpp>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define SOCK_SERVER 0
#define SOCK_CLIENT 1
#define CONN_SERVER 0
#define CONN_CLIENT 1


// noncopyable
namespace common{

class NonCopyable
{
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable operator= (const NonCopyable&) =delete;
protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

inline static uint32_t Now2Ms()
{
    using namespace  std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

}

inline static uint64_t Now2Us()
{
    using namespace  std::chrono;
    return static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());

}

inline static uint32_t Now2S()
{
    using namespace  std::chrono;
    return static_cast<uint32_t>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());

}


}
