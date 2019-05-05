#pragma once

#include <common/utils.hpp>
#include <memory>

namespace dan {
class danMysql;
}


namespace dan { namespace db {

class AnMysql: common::NonCopyable 
{
public:
    AnMysql() noexcept;
    ~AnMysql() noexcept;

    int Ping() noexcept;
    int Insert(uint64_t ulUserId, std::string& strOpenid, const uint8_t* pstValue, uint64_t ulLen) noexcept;
    int Insert1(uint64_t ulUserId, std::string& strOpenid, uint8_t* pstValue, uint64_t ulLen) noexcept;
    int Insert2(uint64_t ulUserId, std::string& strOpenid, uint8_t* pstValue, uint64_t ulLen) noexcept;
    int Update(uint64_t ulUserid, const uint8_t* pstValue, uint64_t ulLen) noexcept;
    int Select(uint64_t ulUserId, uint8_t* s) noexcept;
    int Select(std::string& strOpenid, uint8_t* s) noexcept;
    int Select1(std::string& strOpenid, uint8_t* s) noexcept;
private:
    bool m_bIsConnected;
    std::unique_ptr<dan::danMysql> m_pstRawConn;
};

}}
