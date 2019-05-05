#include <db/AnMysql.h>
#include <db/dan_mysql.hpp>


namespace dan { namespace db {

AnMysql::AnMysql() noexcept :
    m_bIsConnected(false),
    m_pstRawConn(new danMysql())
{
    //m_bIsConnected = m_pstRawConn->connect("127.0.0.1", "root", "206206A@b", 3306, "king");
    m_bIsConnected = m_pstRawConn->connect("127.0.0.1", "root", "lkoji9u87", 3306, "king");
}


AnMysql::~AnMysql() noexcept
{}


int AnMysql::Ping() noexcept
{
    std::string strCmd = "ping";
    dan::danMysqlDo stDo(m_pstRawConn.get(), strCmd);
    if(stDo.doPing())
    {
        return 0;
    }
    return -1;
}



// new ass
int AnMysql::Insert(uint64_t ulUserId, std::string& strOpenid, const uint8_t*pstValue, uint64_t ulLen) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(), "insert into bf_user_0(userid,openid,updatetime,player) values(?,?,unix_timestamp(),?)  on duplicate key update player = ?, updatetime = unix_timestamp()");
    stDo.setUint64(0, ulUserId);
    stDo.setString(1, strOpenid);
    stDo.setBlob(2, reinterpret_cast<const char*>(pstValue), ulLen);
    stDo.setBlob(3, reinterpret_cast<const char*>(pstValue), ulLen);
    if(stDo.doInsert() >= 0)
    {
        return 0;
    }
    return -1;
}

int AnMysql::Insert1(uint64_t ulUserId, std::string& strOpenid, uint8_t*pstValue, uint64_t ulLen) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(),"");
    if(stDo.doInsert1(ulUserId, strOpenid, pstValue, ulLen) >= 0)
    {
        return 0;
    }
    return -1;
}

int AnMysql::Insert2(uint64_t ulUserId, std::string& strOpenid, uint8_t*pstValue, uint64_t ulLen) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(),"");
    if(stDo.doInsert2(ulUserId, strOpenid, pstValue, ulLen) >= 0)
    {
        return 0;
    }
    return -1;
}



// old ass
int AnMysql::Update(uint64_t ulUserId, const uint8_t*pstValue, uint64_t ulLen) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(), "update bf_user_0 set player = ?, updatetime = unix_timestamp() where userid = ?");
    stDo.setBlob(0, reinterpret_cast<const char*>(pstValue), ulLen);
    stDo.setUint64(1, ulUserId);
    if(stDo.doInsert() >= 0)
    {
        return 0;
    }

    return -1;
}


// 
int AnMysql::Select(uint64_t ulUserId, uint8_t* pstBuffer) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(), "select player from bf_user_0 where userid = ?");
    stDo.setUint64(0, ulUserId);

    return stDo.doQuery1(pstBuffer);
}

int AnMysql::Select(std::string& strOpenid, uint8_t* pstBuffer) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(), "select player from bf_user_0 where openid = ?");
    stDo.setString(0, strOpenid);

    return stDo.doQuery1(pstBuffer);
}

int AnMysql::Select1(std::string& strOpenid, uint8_t* pstBuffer) noexcept
{
    dan::danMysqlDo stDo(m_pstRawConn.get(), "select player from bf_user_1 where openid = ?");
    stDo.setString(0, strOpenid);

    return stDo.doQuery1(pstBuffer);
}





}}

