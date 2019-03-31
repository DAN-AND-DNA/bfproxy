#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <iostream>

//#define RELEASE

namespace dan { namespace log {


class Detail
{
public:
    Detail()
    {

#ifdef RELEASE

        spdlog::init_thread_pool(8192, 1);
        this->m_pstLogger = spdlog::daily_logger_mt<spdlog::async_factory>("bf_db", "./bf_db.log", 0, 0);
#else

#endif
    }
    ~Detail(){spdlog::drop_all();}
    std::shared_ptr<spdlog::logger> m_pstLogger;
};

class BFLog
{
public:
    BFLog(){if (stDetail.m_pstLogger != nullptr) {printf("EEEE\n");}}//BFLog::INFO("init bf log success......");}
    ~BFLog(){}//BFLog::INFO("close bf log.......");}

#ifdef RELEASE
    template<typename Arg1, typename... Args>
    static void DEBUG(const char* fmt, const Arg1 &arg1, const Args &... args){stDetail.m_pstLogger->debug(fmt, arg1, args...);}
    static void DEBUG(const char* szMsg){stDetail.m_pstLogger->debug(szMsg);}
 
    template<typename Arg1, typename... Args>
    static void INFO(const char* fmt, const Arg1 &arg1, const Args &... args){stDetail.m_pstLogger->info(fmt, arg1, args...);}
    static void INFO(const char* szMsg){stDetail.m_pstLogger->info(szMsg);}
    
    template<typename Arg1, typename... Args>
    static void WARN(const char* fmt, const Arg1 &arg1, const Args &... args){stDetail.m_pstLogger->warn(fmt, arg1, args...);}
    static void WARN(const char* szMsg){stDetail.m_pstLogger->warn(szMsg);}
    
    template<typename Arg1, typename... Args>
    static void ERR(const char* fmt, const Arg1 &arg1, const Args &... args){stDetail.m_pstLogger->error(fmt, arg1, args...);}
    static void ERR(const char* szMsg){stDetail.m_pstLogger->error(szMsg);} 
          
    template<typename Arg1, typename... Args>
    static void FATAL(const char* fmt, const Arg1 &arg1, const Args &... args){stDetail.m_pstLogger->critical(fmt, arg1, args...);}
    static void FATAL(const char* szMsg){stDetail.m_pstLogger->critical(szMsg);} 
    
    static void FLUSH(){stDetail.m_pstLogger->flush();}
#else
    template<typename Arg1, typename... Args>
    static void DEBUG(const char* fmt, const Arg1 &arg1, const Args &... args){spdlog::debug(fmt, arg1, args...);}
    static void DEBUG(const char* szMsg){spdlog::debug(szMsg);}

    template<typename Arg1, typename... Args>
    static void INFO(const char* fmt, const Arg1 &arg1, const Args &... args){spdlog::info(fmt, arg1, args...);}
    static void INFO(const char* szMsg){spdlog::info(szMsg);}
    template<typename Arg1, typename... Args>
    static void WARN(const char* fmt, const Arg1 &arg1, const Args &... args){spdlog::warn(fmt, arg1, args...);}
    static void WARN(const char* szMsg){spdlog::warn(szMsg);}
    
    template<typename Arg1, typename... Args>
    static void ERR(const char* fmt, const Arg1 &arg1, const Args &... args){spdlog::error(fmt, arg1, args...);}
    static void ERR(const char* szMsg){spdlog::error(szMsg);} 
   
    template<typename Arg1, typename... Args>
    static void FATAL(const char* fmt, const Arg1 &arg1, const Args &... args){spdlog::critical(fmt, arg1, args...);}
    static void FATAL(const char* szMsg){spdlog::critical(szMsg);} 
    
    static void FLUSH(){}
#endif

private:
    static Detail stDetail;
};


}}
