//
//  ConcurrentQueue.h
//
//  Created by Miroslav Kudrnac on 29.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef TransDB_ConcurrentQueue_h
#define TransDB_ConcurrentQueue_h

template <class T>
class ConcurrentQueue
{
public:
    explicit ConcurrentQueue(size_t maxsize = 0) NOEXCEPT : m_maxsize(maxsize)
    {
    }
    
    /** Return the approximate size of the queue (not reliable!).
     */
    size_t size()
    {
        std::lock_guard<std::mutex> rLock(m_rLock);
        return m_rQueue.size();
    }
    
    /**
     */
    bool put(const T &item)
    {
        std::unique_lock<std::mutex> rLock(m_rLock);
        if(m_maxsize > 0)
        {
            while(m_rQueue.size() == m_maxsize)
            {
                m_rNotFullCond.wait(rLock);
            }
        }
        
        m_rQueue.push(item);
        m_rNotEmptyCond.notify_one();
        return true;
    }
    
    /**
     */
    bool get(T &item, uint32 timeout)
    {
        std::unique_lock<std::mutex> rLock(m_rLock);
        //
        bool status = m_rNotEmptyCond.wait_for(rLock, std::chrono::seconds(timeout), [this](){ return !m_rQueue.empty(); } );
        if(status)
        {
            item = m_rQueue.front();
            m_rQueue.pop();
            m_rNotFullCond.notify_one();
        }
        return status;
    }
    
    /** unblock all waiting threads
     */
    INLINE void abort()
    {
        std::lock_guard<std::mutex> rLock(m_rLock);
        m_rNotEmptyCond.notify_all();
        m_rNotFullCond.notify_all();
    }
    
private:
    DISALLOW_COPY_AND_ASSIGN(ConcurrentQueue);
    
    //variables
    std::queue<T>               m_rQueue;
    std::condition_variable     m_rNotEmptyCond;
    std::condition_variable     m_rNotFullCond;
    std::mutex                  m_rLock;
    size_t                      m_maxsize;
};

#endif
