#pragma once

#include <functional>

namespace dan { namespace eventloop {

using CALLBACK = std::function<void()>;

//for epoll
#define EPOLL_WAIT_TIME 10000

//for event
#define EVENT_SERVER 0
#define EVENT_CLIENT 1

// for socket
#define SOCKET_SERVER 0
#define SOCKET_CLIENT 1

}}


