#include <log/BFLog.hpp>
#include <src/BfDBServer.h>
#include <log/Exception.h>
#include <signal.h>
//#include <setjmp.h>

namespace 
{
    //auto pstServer = std::make_shared<dan::bf::BfDBServer>("192.168.0.45", 3737);
    auto pstServer = std::make_shared<dan::bf::BfDBServer>("0.0.0.0", 3737);
}

void ErrorHandle(int)
{
    dan::log::BFLog::ERR("{}", dan::Exception("error").StackTrace());
    dan::log::BFLog::FLUSH();
    ::exit(-1);
}

void CloseHandle(int)
{
    dan::log::BFLog::WARN("get ctrl+c, server is close...");
    pstServer->Stop();
    dan::log::BFLog::FLUSH();
}

int main()
{
    dan::log::BFLog::WARN("init app......");
    ::signal(SIGPIPE, SIG_IGN);
    ::signal(SIGSEGV, ErrorHandle);
    ::signal(SIGINT,  CloseHandle);
    pstServer->Run();
    pstServer = nullptr;
    dan::log::BFLog::WARN("server close success!");
}
