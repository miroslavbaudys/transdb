//
//  Vector.h
//
//  Created by Miroslav Kudrnac on 20.01.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef TransDB_Vector_h
#define TransDB_Vector_h

template<class T, class SizeT = size_t>
class Vector
{
public:
    typedef T*              iterator;
    typedef T               value_type;
    typedef SizeT           size_type;
	static const size_type	granularity = 16;
    
	//constructor
    Vector() NOEXCEPT : m_pBuff(NULL),
                        m_size(0),
                        m_capacity(0)
    {

    }
    
	//constructor
    Vector(size_type initialSize) : m_pBuff(NULL),
                                    m_size(0),
                                    m_capacity(0)
    {
        reserve(initialSize);
    }
    
	//copy constructor
    Vector(const Vector<T, SizeT> &v) : m_pBuff(NULL),
										m_size(0), 
										m_capacity(0)
    { 
		*this = v;
    }

	//moveable constructor
	Vector(Vector<T, SizeT> &&v) NOEXCEPT : m_pBuff(NULL),
                                            m_size(0),
                                            m_capacity(0)
	{
		*this = std::move(v);	
	}

    ~Vector()
    {
        clear();
    }
    
    INLINE iterator begin() NOEXCEPT
    {
        return m_pBuff;
    }
    
    INLINE iterator end() NOEXCEPT
    {
        return m_pBuff + m_size;
    }
    
    INLINE void push_back(const T& value)
    {   
        if(m_capacity == m_size)
        {
            size_type newCapacity = m_capacity == 0 ? granularity : m_capacity * 2;
            reserve(newCapacity);
        }
        
        m_pBuff[m_size] = value;
        ++m_size;
    }
    
    INLINE void reserve(size_type newCapacity)
    {
        assert(newCapacity >= 0);
        
        //only if new size is bigger than capacity
        if(newCapacity > m_capacity)
        {
            //set capacity
            m_capacity = newCapacity;
            // resize buffer
            void *pNewBuff = _REALLOC(m_pBuff, sizeof(T) * m_capacity);
            if(pNewBuff == NULL)
            {
                _FREE((void*)m_pBuff);
                throw std::bad_alloc();
            }
            m_pBuff = (T*)pNewBuff;
        }
    }
    
    INLINE void resize(size_type newSize)
    {
        reserve(newSize);
        m_size = newSize;
    }
    
    INLINE void clear() NOEXCEPT
    {
        if(m_pBuff)
        {
            _FREE(m_pBuff);
            m_pBuff = NULL;
        }
        m_size      = 0;
        m_capacity  = 0;
    }
        
    INLINE size_type capacity() const NOEXCEPT
    {
        return m_capacity;
    }
    
    INLINE size_type size() const NOEXCEPT
    {
        return m_size;
    }
    
    INLINE bool empty() const NOEXCEPT
    {
        return m_size == 0;
    }
    
    INLINE T &front() NOEXCEPT
    {
        return m_pBuff[0];
    }
    
    INLINE T &back() NOEXCEPT
    {
        return m_pBuff[m_size - 1];
    }
    
    INLINE void pop_back() NOEXCEPT
    {
        --m_size;
    }
    
	INLINE T &operator[](size_type index) NOEXCEPT
    {
        assert(index >= 0);
        assert(index < m_size);
        return m_pBuff[index];
    }

    INLINE const T &operator[](size_type index) const NOEXCEPT
    {
        assert(index >= 0);
        assert(index < m_size);
        return m_pBuff[index];
    }
    
    INLINE const value_type *data() const NOEXCEPT
    {
        assert(m_pBuff);
        return &m_pBuff[0];
    }

    INLINE value_type *data() NOEXCEPT
    {
        assert(m_pBuff);
        return &m_pBuff[0];
    }
    
    INLINE Vector<T, SizeT> &operator=(const Vector<T, SizeT> &v)
    {
		if(this != &v)
		{
            //clear
            _FREE(m_pBuff);
            
			//copy
            m_pBuff = NULL;
			m_size = v.m_size;
			m_capacity = v.m_capacity;
            
			//copy buff
			if(m_capacity)
			{
				m_pBuff = (T*)_MALLOC(sizeof(T) * m_capacity);
                if(m_pBuff == NULL)
                {
                    throw std::bad_alloc();
                }
				memcpy(m_pBuff, v.m_pBuff, sizeof(T) * m_size);
			}
		}
        return *this;
    }

	INLINE Vector<T, SizeT>& operator=(Vector<T, SizeT> &&v) NOEXCEPT
	{
		if(this != &v)
		{
            //clear
            _FREE(m_pBuff);
            
            //set new data
            m_pBuff = v.m_pBuff;
            m_size = v.m_size;
            m_capacity = v.m_capacity;
            
            //clear data
            v.m_pBuff = NULL;
            v.m_size = 0;
            v.m_capacity = 0;
		}
		return *this;
	}
    
private:    
    T           *m_pBuff;
    SizeT       m_size;
    SizeT       m_capacity;
};

#endif
