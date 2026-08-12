//
//  ReferenceCounter.h
//
//  Created by Miroslav Kudrnac on 22.11.13.
//
//

#ifndef PLTMobile_ReferenceCounter_h
#define PLTMobile_ReferenceCounter_h

class ReferenceCounter
{
public:
    explicit ReferenceCounter() : m_referenceCount(1)
    {
        
    }
    
    virtual ~ReferenceCounter()
    {
        assert(m_referenceCount == 0);
    }
    
    INLINE void retain()
    {
        ++m_referenceCount;
    }
    
    INLINE void release()
    {
        --m_referenceCount;
        if(m_referenceCount == 0)
        {
            delete this;
        }
    }
    
private:
    uint32  m_referenceCount;
};

#endif
