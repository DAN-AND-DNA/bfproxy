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
//#include"common/logging.hpp"
//#include"dan_logger.hpp"
//#include <google/protobuf/message.h>

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

    MYSQL*getMysql(){return mysql_;}
private:
    MYSQL*mysql_;
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
    int doQuery();//执行查询
    int doQuery1(uint8_t* pstBuffer);
    int doReplace();
    bool doPing();
private:
    typedef std::map<uint32_t,std::string>fieldValue_t;//域值
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
        std::cout<<"getRowNum 错误"<<std::endl;
        return "";
    }
    if(field>=getFieldNum())
    {
        std::cout<<"getFieldNum 错误"<<std::endl;
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
        std::cout<<"doInsert 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        return -1;
    }
    int lastInsertId=mysql_insert_id(danMysqlPtr_->getMysql());
    return lastInsertId;
}

int danMysqlDo::doReplace()
{
    std::string sql=makeSql();
    if(mysql_query(danMysqlPtr_->getMysql(),sql.c_str()))
    {
        std::cout<<"doRelace 错误:"<<mysql_error(danMysqlPtr_->getMysql())<<std::endl;
        return -1;
    }
    int lastInsertId=mysql_insert_id(danMysqlPtr_->getMysql());
    return lastInsertId;
}

bool danMysqlDo::setBlob(const uint8_t idx, const char* pstValue, unsigned long dlLength)
{
    if(idx>argsCount_)
    {
        std::cout<<"参数个数不一致"<<std::endl;
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
        std::cout<<"参数个数不一致"<<std::endl;
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
        std::cout<<"参数个数不一致"<<std::endl;
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
        std::cout<<"参数个数不一致"<<std::endl;
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
        argsCount_=std::count(sql.begin(),sql.end(),'?');
    //std::cout<<argsCount_;
}

//===============================================
const std::string danMysql::escapeString(const std::string&value)const
{
    if(!isConnected_)
    {

        std::cout<<"danMysql::escapeString() 未连接到mysql，无法转义字符串"<<std::endl;
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

        std::cout<<"danMysql::escapeByte() 未连接到mysql，无法转义字符串"<<std::endl;
        return"";
    }

    if(!pstFromBuffer)
    {
        std::cout<<"danMysql::escapeByte() nil ptr"<<std::endl;
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

    ip_=ip;
    userName_=userName;
    userPasswd_=userPasswd;
    port_=port;
    mysql_=mysql_init(NULL); //新建一个
    if(mysql_real_connect(mysql_,
                          ip,
                          userName,
                          userPasswd,
                          dbName,
                          port,NULL,0)==NULL)
    {
        //连接失败
        isConnected_=false;
        std::cout<<"danMysql::connect() 连接mysql失败"<<std::endl;
        return false;
    }
    else
    {
        //连接成功
        isConnected_=true;
        std::cout<<"danMysql::connect() 连接mysql成功"<<std::endl;
        return true;
    }


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

    std::cout<<"danMysql::~danMysql() 关闭和mysql的连接"<<std::endl;
 
    disconnect();
}





}
