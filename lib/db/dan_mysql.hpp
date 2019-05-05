#pragma once

#include <map>
#include <string>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <algorithm>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <log/BFLog.hpp>

namespace dan
{

class danMysql
{
public:
    danMysql();
    ~danMysql();

    void disconnect();//关闭和mysql的连接
    bool connect(const char* ip,
                 const char* userName,
                 const char* userPasswd,
                 const uint16_t port=3306,
                 const char* dbName=NULL);//连接到mysql
    const std::string escapeString(const std::string&value)const;
    const std::string escapeBlob(const char* pstToFrom, unsigned long iLength);
 
    MYSQL* getMysql(){return mysql_;}
    MYSQL_STMT*  stmt;
    MYSQL_STMT*  stmt1;
private:
    MYSQL* mysql_;
    bool isConnected_;
    const char* ip_;
    const char* userName_;
    const char* userPasswd_;
    const char* dbName_;
    uint16_t port_; 
};

class danMysqlDo
{
public:
    danMysqlDo(danMysql*ptr,const std::string& sql);
    ~danMysqlDo(){}
    bool setBlob(const uint8_t idx, const char* pstValue, unsigned long dlLength);
    bool setString(const uint8_t& idx,const std::string& value);
    bool setInt(const uint8_t& idx,const int& value);
    bool setUint64(const uint8_t& idx, const uint64_t& value);
    bool setDouble(const uint8_t& idx,const double& value); 
    bool setNull(const uint8_t& idx);

    uint32_t getRowNum(){return static_cast<uint32_t>(rowValue_.size());}
    uint32_t getFieldNum(){return static_cast<uint32_t>(fieldName_.size());}
    const std::string makeSql();//填入参数,完成sql语句

    const std::string getFieldValue(uint32_t row , uint32_t field);
    int doInsert();//执行插入
    int doInsert1(uint64_t ulUserid, std::string&strOpenid,const uint8_t* pstValue, uint64_t length);//执行插入
    int doInsert2(uint64_t ulUserid, std::string&strOpenid,const uint8_t* pstValue, uint64_t length);//执行插入
    int doQuery();//执行查询
    int doQuery1(uint8_t* pstBuffer);
    int doReplace();
    bool doPing();
private:
    typedef std::map<uint32_t,std::string> fieldValue_t;//域值
    std::map<uint32_t,fieldValue_t> rowValue_;//行值
    std::map<uint32_t,std::string> args_;  //参数
    std::map<uint32_t,std::string> fieldName_; //域名字
    danMysql* danMysqlPtr_;
    std::string sql_;
    uint8_t argsCount_; //参数个数
};

//===============================================
const std::string danMysqlDo::getFieldValue(uint32_t row,uint32_t field)
{
    if(getRowNum() == 0)
    {
        return "";
    }

    if(row>=getRowNum())
    {
        //std::cout<<"getRowNum 错误"<<std::endl;
        dan::log::BFLog::ERR("mysql getRowNum failed"); 
        return "";
    }
    if(field>=getFieldNum())
    {
        dan::log::BFLog::ERR("mysql getFieldNum failed");
        return "";
    }
    return (rowValue_[row])[field];
}

int danMysqlDo::doQuery()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        //std::cout<<"doQuery 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        return -1;
    }
    MYSQL_RES* result=mysql_store_result(danMysqlPtr_->getMysql());
    if(result==NULL)
    {
        //发生错误
       // std::cout<<"doQuery 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        mysql_free_result(result);
        return 0;
    }

    uint32_t fieldNum=mysql_num_fields(result);
    MYSQL_ROW row;
    MYSQL_FIELD*field;

    uint32_t i=0;
    while((field=mysql_fetch_field(result)))
    {
        fieldName_[i]=field->name;
        ++i;
    }
    i=0;
    while((row=mysql_fetch_row(result)))
    {
        fieldValue_t fieldValue;
        for(uint32_t n=0;n<fieldNum;n++)
        {
            //填充每个域值
            fieldValue[n]=(row[n]? row[n]:"NULL");
           // std::cout<<fieldValue[n]<<std::endl;
        }
        //填充每个行值
        rowValue_[i]=fieldValue;
        i++;
    }
    mysql_free_result(result);
    //std::cout<<(rowValue_[0])[0];
    return 0;
}


int danMysqlDo::doQuery1(uint8_t* pstBuffer)
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        return -1;
    }
    MYSQL_RES* result=mysql_store_result(danMysqlPtr_->getMysql());
    if(result==NULL)
    {
        mysql_free_result(result);
        return 0;
    }

    //uint32_t fieldNum=mysql_num_fields(result);
    MYSQL_ROW row;
    MYSQL_FIELD*field;

    uint32_t i=0;
    while((field=mysql_fetch_field(result)))
    {
        fieldName_[i]=field->name;
        ++i;
    }
    row = mysql_fetch_row(result);
    //fieldValue[n]=(row[n]? row[n]:"NULL");
    //printf("queuy:%lu\n", sizeof(row[0]));
    
    if(row)
    {
        auto len = mysql_fetch_lengths(result);
        int l = static_cast<int>(len[0]);
        //printf("queuy:%lu\n", len[0]); 
        ::memcpy(pstBuffer, row[0], len[0]);

        mysql_free_result(result);
        return l;
    }
    else
    {
        mysql_free_result(result);
        return 0;
    }
}



bool danMysqlDo::doPing()
{
    if(mysql_ping(danMysqlPtr_->getMysql()))
    {
        std::cout<<"doPing 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        return false;
    }

    return true;
}

const std::string danMysqlDo::makeSql()
{
    int foundPos=0;
    int beginPos=0;
    std::string sql;
    sql=sql_;
    for(unsigned int i=0;i<argsCount_;++i)
    {

#pragma GCC diagnostic ignored "-Wconversion"

        foundPos=sql.find('?',beginPos+foundPos);
        sql.replace(foundPos,1,args_[i]);
    }
    return sql;
}

int danMysqlDo::doInsert()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        //std::cout<<"doInsert 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        dan::log::BFLog::ERR("do insert {}", mysql_error(danMysqlPtr_->getMysql()));
        return -1;
    }
    int lastInsertId=mysql_insert_id(danMysqlPtr_->getMysql());
    dan::log::BFLog::INFO("last insert id:{}", lastInsertId);
    return lastInsertId;
}


int danMysqlDo::doInsert1(uint64_t ulUserid, std::string&strOpenid ,const uint8_t* pstValue, uint64_t length)
{
    /*
    if (mysql_stmt_send_long_data(stmt,0, reinterpret_cast<const char*>(pstValue), length))
    {
        dan::log::BFLog::ERR("stmt send failed:{}", mysql_stmt_error(stmt));
        return -1;
    }
    */

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    auto strlen  = strOpenid.size();
    
    
    bind[0].buffer = reinterpret_cast<void*>(&ulUserid);
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].length = 0;
    bind[0].is_null = 0;
    
    bind[1].buffer = const_cast<char*>((strOpenid.c_str()));
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].length = &(strlen);
    bind[1].is_null = 0;
    
    bind[2].buffer = const_cast<void*>(reinterpret_cast<const void*>(pstValue));
    bind[2].buffer_type = MYSQL_TYPE_BLOB;
    bind[2].buffer_length = length;
    bind[2].length = &length;
    bind[2].is_null = 0;
    
    bind[3].buffer = const_cast<void*>(reinterpret_cast<const void*>(pstValue));
    bind[3].buffer_type = MYSQL_TYPE_BLOB;
    bind[3].buffer_length = length;
    bind[3].length = &length;
    bind[3].is_null = 0;
 
    if(mysql_stmt_bind_param(danMysqlPtr_->stmt, bind))
    {
        dan::log::BFLog::ERR("stmt bind failed:{}", mysql_stmt_error(danMysqlPtr_->stmt));
        return -1;
    }
    
    if(mysql_stmt_execute(danMysqlPtr_->stmt))
    {
        dan::log::BFLog::ERR("stmt execute failed:{}", mysql_stmt_error(danMysqlPtr_->stmt));
        return -1;
    }
    return 0;
}

int danMysqlDo::doInsert2(uint64_t ulUserid, std::string&strOpenid ,const uint8_t* pstValue, uint64_t length)
{
    /*
    if (mysql_stmt_send_long_data(stmt,0, reinterpret_cast<const char*>(pstValue), length))
    {
        dan::log::BFLog::ERR("stmt send failed:{}", mysql_stmt_error(stmt));
        return -1;
    }
    */

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    auto strlen  = strOpenid.size(); 
    
    
    bind[0].buffer = reinterpret_cast<void*>(&ulUserid);
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].length = 0;
    bind[0].is_null = 0;
    
    bind[1].buffer = const_cast<char*>((strOpenid.c_str()));
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].length = &(strlen);
    bind[1].is_null = 0;
    
    bind[2].buffer = const_cast<void*>(reinterpret_cast<const void*>(pstValue));
    bind[2].buffer_type = MYSQL_TYPE_BLOB;
    bind[2].buffer_length = length;
    bind[2].length = &length;
    bind[2].is_null = 0;
    
    bind[3].buffer = const_cast<void*>(reinterpret_cast<const void*>(pstValue));
    bind[3].buffer_type = MYSQL_TYPE_BLOB;
    bind[3].buffer_length = length;
    bind[3].length = &length;
    bind[3].is_null = 0;
 
    if(mysql_stmt_bind_param(danMysqlPtr_->stmt1, bind))
    {
        dan::log::BFLog::ERR("stmt1 bind failed:{}", mysql_stmt_error(danMysqlPtr_->stmt1));
        return -1;
    }
    
    if(mysql_stmt_execute(danMysqlPtr_->stmt1))
    {
        dan::log::BFLog::ERR("stmt1 execute failed:{}", mysql_stmt_error(danMysqlPtr_->stmt1));
        return -1;
    }
    return 0;
}



int danMysqlDo::doReplace()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        //std::cout<<"doRelace 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        return -1;
    }
    int lastInsertId=mysql_insert_id(danMysqlPtr_->getMysql());
    return lastInsertId;
}

bool danMysqlDo::setBlob(const uint8_t idx, const char* pstValue, unsigned long dlLength)
{
    if(idx>argsCount_)
    {
        dan::log::BFLog::ERR("set blob : 参数个数不一致");
        return false;
    }
    std::stringstream ss;

    std::string escapedValue =  danMysqlPtr_->escapeBlob(pstValue, dlLength);
    ss<<"\""<<escapedValue<<"\"";
    args_[idx]=ss.str();
    return true;
}

bool danMysqlDo::setString(const uint8_t& idx,const std::string& value)
{
    if(idx>argsCount_)
    {
        dan::log::BFLog::ERR("set string : 参数个数不一致");
        return false;
    }
    std::stringstream ss;
    std::string escapedValue=danMysqlPtr_->escapeString(value);
    ss<<"\""<<escapedValue<<"\"";
    args_[idx]=ss.str();
    return true;

}

bool danMysqlDo::setInt(const uint8_t& idx,const int& value)
{
    if(idx>argsCount_)
    {
        dan::log::BFLog::ERR("set int : 参数个数不一致");
        return false;
    }
    std::stringstream ss;
    ss<<value;
    args_[idx]=ss.str();
    return true;
}

bool danMysqlDo::setUint64(const uint8_t& idx,const uint64_t& value)
{
    if(idx>argsCount_)
    {
        dan::log::BFLog::ERR("set UINT64 : 参数个数不一致");
        return false;
    }
    std::stringstream ss;
    ss<<value;
    args_[idx]=ss.str();
    return true;
}


bool danMysqlDo::setDouble(const uint8_t& idx,const double& value)
{
    if(idx>argsCount_)
    {
        std::cout<<"参数个数不一致"<<std::endl;
        return false;
    }
    std::stringstream ss;
    ss<<value;
    args_[idx]=ss.str();
    return true;
}

bool danMysqlDo::setNull(const uint8_t& idx)
{
    if(idx>argsCount_)
    {
        std::cout<<"参数个数不一致"<<std::endl;
        return false;
    }
    args_[idx]="NULL";
    return true;
}
danMysqlDo::danMysqlDo(danMysql*ptr,const std::string& sql):
    danMysqlPtr_(ptr),
    sql_(sql),
    argsCount_(0)
{
    if(!sql.empty())
        argsCount_ = std::count(sql.begin(),sql.end(),'?');
    
    //std::cout<<argsCount_;
}

//===============================================
const std::string danMysql::escapeString(const std::string&value)const
{
    if(!isConnected_)
    {
        dan::log::BFLog::ERR("danMysql::escapeString() 未连接到mysql，无法转义字符串");
        //std::cout<<"danMysql::escapeString() 未连接到mysql，无法转义字符串"<<std::endl;
        return "";
    }
#pragma GCC diagnostic ignored "-Wold-style-cast"

    char*cValue=(char*)calloc(1,value.length()*2+1);
    mysql_real_escape_string(mysql_,cValue,value.c_str(),value.length());
    std::string ret=cValue;
    free (cValue);
    return ret;
}


const std::string danMysql::escapeBlob(const char* pstFromBuffer,  unsigned long dlLength)
{
    if(!isConnected_)
    {
        //std::cout<<"danMysql::escapeByte() 未连接到mysql，无法转义字符串"<<std::endl;
        dan::log::BFLog::ERR("danMysql::escapeBlob() 未连接到mysql，无法转义字符串");
        return"";
    }

    if(!pstFromBuffer)
    {

        dan::log::BFLog::ERR("danMysql::escapeBlob() nil ptr");
      //  std::cout<<"danMysql::escapeByte() nil ptr"<<std::endl;
        return"";
    }
    char* pstToBuffer = (char*) calloc(1, 2*dlLength + 2);
    ::mysql_real_escape_string(mysql_, pstToBuffer, pstFromBuffer, dlLength);
    std::string strRet = pstToBuffer;
    free(pstToBuffer);
    return strRet;

}

bool danMysql::connect(const char* ip,
                 const char* userName,
                 const char* userPasswd,
                 const uint16_t port,
                 const char* dbName)
{
    disconnect(); //先断开之前的连接

    ip_ = ip;
    userName_ = userName;
    userPasswd_ = userPasswd;
    port_ = port;
    mysql_ = mysql_init(NULL); //新建一个

    /*
    if (mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4") == 0)
    {
        dan::log::BFLog::INFO("danMysql::connect() 设置mysql字符集成功");
    }
    else
    {
        dan::log::BFLog::ERR("danMysql::connect() 设置mysql字符集失败");
        return false;
    }
    */

    if(mysql_real_connect(mysql_,
                          ip,
                          userName,
                          userPasswd,
                          dbName,
                          port,NULL,0)==NULL)
    {
        //连接失败
        isConnected_ = false;
        dan::log::BFLog::ERR("danMysql::connect() 连接mysql失败");
    }
    else
    {
        //连接成功
        isConnected_ = true;
        dan::log::BFLog::INFO("danMysql::connect() 连接mysql成功");
    }

    stmt = mysql_stmt_init(mysql_);
    if(!stmt)
    {
        dan::log::BFLog::ERR("smt init failed");
        return false;
    }
    else
    {
        dan::log::BFLog::INFO("stmt init success");
    }

    //#define INSERT "insert into bf_user_0(userid,openid,player) values(?)"
    #define  INSERT "insert into bf_user_0(userid,openid,updatetime,player) values(?,?,unix_timestamp(),?)  on duplicate key update player = ?, updatetime = unix_timestamp()"
 
    if(mysql_stmt_prepare(stmt, INSERT, strlen(INSERT)))
    {
        dan::log::BFLog::ERR("stmt prepare failed:{}", mysql_stmt_error(stmt));
        return false;
    }
    
    stmt1 = mysql_stmt_init(mysql_);
    if(!stmt1)
    {
        dan::log::BFLog::ERR("smt1 init failed");
        return false;
    }
    else
    {
        dan::log::BFLog::INFO("stmt1 init success");
    }

    //#define INSERT "insert into bf_user_0(userid,openid,player) values(?)"
    #define  INSERT1 "insert into bf_user_1(userid,openid,updatetime,player) values(?,?,unix_timestamp(),?)  on duplicate key update player = ?, updatetime = unix_timestamp()"
 
    if(mysql_stmt_prepare(stmt1, INSERT1, strlen(INSERT1)))
    {
        dan::log::BFLog::ERR("stmt1 prepare failed:{}", mysql_stmt_error(stmt1));
        return false;
    }


    return true;
}

void danMysql::disconnect()
{
    if(mysql_==nullptr)
        return;
    mysql_close(mysql_);
    isConnected_=false;
}

danMysql::danMysql():
    mysql_(nullptr),isConnected_(false),port_(0)
{
}

danMysql::~danMysql()
{
     if (mysql_stmt_close(stmt))
     {
        dan::log::BFLog::ERR("close stmt failed");
     }

    if (mysql_stmt_close(stmt1))
     {
        dan::log::BFLog::ERR("close stmt1 failed");
     }
     
     dan::log::BFLog::INFO("关闭和mysql的连接");
    //std::cout<<"danMysql::~danMysql() 关闭和mysql的连接"<<std::endl;
    disconnect();
}





}
