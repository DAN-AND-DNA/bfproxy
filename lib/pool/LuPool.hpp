#pragma once

// object pool

#include <common/utils.hpp>
#include <list>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>

namespace dan { namespace pool{

template<typename T>
class LuPool : common::NonCopyable {
public:
    LuPool(const std::string& strName, int dwMaxIdleSize = 10, uint16_t dwMaxKeepTime = 60) noexcept;
    LuPool(std::string&& strName, int dwMaxIdleSize = 10, uint16_t dwMaxKeepTime = 60) noexcept;
    ~LuPool();
public:
    std::shared_ptr<T>                      GetIdle() noexcept;
    void                                    GiveBack(std::shared_ptr<T>&& pstObj) noexcept;
    std::string&                            PoolName() const{return m_strPoolName;}
    uint32_t                                IdleSize();
private:
    int                                     KeepIdle();
private:
    std::string                             m_strPoolName;
    std::list<std::shared_ptr<T>>           m_stObjList;
    uint32_t                                m_dwIdleSize;
    uint32_t                                m_dwPoolSize;
    uint32_t                                m_dwLastCheckTime;
    int                                     m_dwMaxIdleSize;
    uint16_t                                m_dwMaxKeepTime;
    std::mutex                              m_stMutex;

};


template<typename T>
LuPool<T>::LuPool(const std::string& strName, int dwMaxIdleSize, uint16_t dwMaxKeepTime) noexcept:
    m_strPoolName(strName),
    m_stObjList(),
    m_dwPoolSize(0),
    m_dwLastCheckTime(common::Now2Ms()),
    m_dwMaxIdleSize(dwMaxIdleSize),
    m_dwMaxKeepTime(dwMaxKeepTime),
    m_stMutex()
{}


template<typename T>
LuPool<T>::LuPool(std::string&& strName, int dwMaxIdleSize, uint16_t dwMaxKeepTime) noexcept:
    m_strPoolName(std::move(strName)),
    m_stObjList(),
    m_dwPoolSize(0),
    m_dwLastCheckTime(common::Now2Ms()),
    m_dwMaxIdleSize(dwMaxIdleSize),
    m_dwMaxKeepTime(dwMaxKeepTime),
    m_stMutex()
{}


template<typename T>
LuPool<T>::~LuPool(){printf("clean %s\n", m_strPoolName.c_str());}

template<typename T>
uint32_t LuPool<T>::IdleSize()
{
    std::lock_guard<std::mutex> guard(m_stMutex);
    return static_cast<uint32_t>(m_stObjList.size());
}

template<typename T>
int LuPool<T>::KeepIdle()
{
    std::lock_guard<std::mutex> guard(m_stMutex);
 
    for(uint16_t i = 0; i< m_dwMaxIdleSize; i++)
    {
        m_stObjList.push_back(std::move(std::make_shared<T>()));
        m_dwPoolSize++;

    }
    return 0;
}


template<typename T>
std::shared_ptr<T> LuPool<T>::GetIdle() noexcept
{
    std::lock_guard<std::mutex> guard(m_stMutex);
    while(true)
    {
        auto dwIdleSize  = m_stObjList.size();
        if(dwIdleSize > 0)
        {
            auto temp = *m_stObjList.begin();
            m_stObjList.pop_front();
            return temp;
        }
        else
        {
            for(uint16_t i = 0; i< m_dwMaxIdleSize; i++)
            {
                m_stObjList.push_back(std::move(std::make_shared<T>()));
                m_dwPoolSize++;
            }
        }
    }
    return nullptr;
}


template<typename T>
void LuPool<T>::GiveBack(std::shared_ptr<T>&& pstObj) noexcept
{
    std::lock_guard<std::mutex> guard(m_stMutex);
    auto dwIdleSize  = m_stObjList.size();
    if(pstObj != nullptr)
    {
        if(dwIdleSize > m_dwMaxIdleSize)
        {
            --m_dwPoolSize;
        }
        else
        {
            m_stObjList.push_back(std::move(pstObj));
        }
    }

    uint32_t now = common::Now2Ms();
    int needClean = static_cast<int>(m_stObjList.size()) - m_dwMaxIdleSize;

    if(needClean <= 0)
    {
        // do nothing
    }
    else if(now - m_dwLastCheckTime >= m_dwMaxKeepTime)
    {
        while(needClean-- > 0)
        {
            printf("needclen:%d\n",needClean);
            printf("err1\n");
            m_stObjList.pop_front();
            printf("err2\n");
            --m_dwPoolSize;
        }
    }

    m_dwLastCheckTime = now;
}




}}
