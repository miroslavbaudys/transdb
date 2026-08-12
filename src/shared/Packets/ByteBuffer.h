/*
 * Game server
 * Copyright (C) 2010 Miroslav 'Wayland' Kudrnac
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include "../Defines.h"
#include "../clib/Buffers/CByteBuffer.h"

class ByteBuffer
{
public:
	ByteBuffer() NOEXCEPT
	{
        bbuff_create(m_pBuff);
        bbuff_reserve(m_pBuff, 512);
	}
    
	ByteBuffer(size_t res) NOEXCEPT
	{
        bbuff_create(m_pBuff);
        bbuff_reserve(m_pBuff, res);
	}
    
    //copy ctor
	ByteBuffer(const ByteBuffer &buf) NOEXCEPT
    {
        bbuff_create(m_pBuff);
        *this = buf;
    }
    
    //moveable ctor
    ByteBuffer(ByteBuffer &&buf) NOEXCEPT
    {
        bbuff_create(m_pBuff);
        *this = std::move(buf);
    }
    
	virtual ~ByteBuffer()
    {
        bbuff_destroy(m_pBuff);
        m_pBuff = NULL;
    }
    
    INLINE ByteBuffer &operator=(const ByteBuffer &buf) NOEXCEPT
    {
        if(this != &buf)
        {
            bbuff_clear(m_pBuff);
            bbuff_append(m_pBuff, buf.m_pBuff->storage, buf.m_pBuff->size);
            m_pBuff->rpos = buf.m_pBuff->rpos;
            m_pBuff->wpos = buf.m_pBuff->wpos;
        }
        return *this;
    }
    
    INLINE ByteBuffer &operator=(ByteBuffer &&buf) NOEXCEPT
    {
        if(this != &buf)
        {
            std::swap(m_pBuff, buf.m_pBuff);
        }
        return *this;
    }
    
	INLINE void clear() NOEXCEPT
	{
        bbuff_clear(m_pBuff);
	}
    
	template <typename T>
    INLINE void append(T value) NOEXCEPT
	{
		append((uint8*)&value, sizeof(value));
	}
    
	template <typename T>
    INLINE void put(size_t pos, T value) NOEXCEPT
	{
		put(pos,(uint8*)&value, sizeof(value));
	}
    
	// stream like operators for storing data
	INLINE ByteBuffer &operator<<(bool value) NOEXCEPT
	{
		append<char>((char)value);
		return *this;
	}
    
	// unsigned
	INLINE ByteBuffer &operator<<(uint8 value) NOEXCEPT
	{
		append<uint8>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(uint16 value) NOEXCEPT
	{
		append<uint16>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(uint32 value) NOEXCEPT
	{
		append<uint32>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(uint64 value) NOEXCEPT
	{
		append<uint64>(value);
		return *this;
	}
    
	// signed as in 2e complement
	INLINE ByteBuffer &operator<<(int8 value) NOEXCEPT
	{
		append<int8>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(int16 value) NOEXCEPT
	{
		append<int16>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(int32 value) NOEXCEPT
	{
		append<int32>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(int64 value) NOEXCEPT
	{
		append<int64>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(float value) NOEXCEPT
	{
		append<float>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(double value) NOEXCEPT
	{
		append<double>(value);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(const std::string &value) NOEXCEPT
	{
		append((uint8*)value.c_str(), value.length());
		append((uint8)0);
		return *this;
	}
    
	INLINE ByteBuffer &operator<<(const char *str) NOEXCEPT
	{
		append((uint8*)str, strlen(str));
		append((uint8)0);
		return *this;
	}
    
	// stream like operators for reading data
	INLINE ByteBuffer &operator>>(bool &value) NOEXCEPT
	{
		value = read<char>() > 0 ? true : false;
		return *this;
	}
    
	//unsigned
	INLINE ByteBuffer &operator>>(uint8 &value) NOEXCEPT
	{
		value = read<uint8>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(uint16 &value) NOEXCEPT
	{
		value = read<uint16>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(uint32 &value) NOEXCEPT
	{
		value = read<uint32>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(uint64 &value) NOEXCEPT
	{
		value = read<uint64>();
		return *this;
	}
    
	//signed as in 2e complement
	INLINE ByteBuffer &operator>>(int8 &value) NOEXCEPT
	{
		value = read<int8>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(int16 &value) NOEXCEPT
	{
		value = read<int16>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(int32 &value) NOEXCEPT
	{
		value = read<int32>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(int64 &value) NOEXCEPT
	{
		value = read<int64>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(float &value) NOEXCEPT
	{
		value = read<float>();
		return *this;
	}
    
	INLINE ByteBuffer &operator>>(double &value) NOEXCEPT
	{
		value = read<double>();
		return *this;
	}
    
	ByteBuffer &operator>>(std::string& value)
	{
		value.clear();
		value.reserve(16);
		for(;;)
		{
			char c = read<char>();
			if(c == 0)
                break;
            
			value += c;
		}
		return *this;
	}
    
	INLINE uint8 operator[](size_t pos) const NOEXCEPT
	{
		return read<uint8>(pos);
	}
    
	INLINE size_t rpos() const NOEXCEPT
	{
		return m_pBuff->rpos;
	}
    
	INLINE size_t rpos(size_t rpos) NOEXCEPT
	{
		m_pBuff->rpos = rpos;
		return m_pBuff->rpos;
	}
    
	INLINE size_t wpos() NOEXCEPT
	{
		return m_pBuff->wpos;
	}
    
	INLINE size_t wpos(size_t wpos) NOEXCEPT
	{
		m_pBuff->wpos = wpos;
		return m_pBuff->wpos;
	}
    
	template <typename T>
    INLINE T read() NOEXCEPT
	{
        T r;
        bbuff_read(m_pBuff, &r, sizeof(T));
		return r;
	}
    
	template <typename T>
    INLINE T read(size_t pos) const NOEXCEPT
	{
		if(pos + sizeof(T) > size())
		{
			return (T)0;
		}
		else
		{
            T value;
            bbuff_read(m_pBuff, &value, sizeof(T));
			return value;
		}
	}
    
	INLINE void read(uint8 *dest, size_t len) NOEXCEPT
	{
        bbuff_read(m_pBuff, dest, len);
	}
    
	INLINE const uint8 *contents() const NOEXCEPT
	{
		return m_pBuff->storage;
	}
    
	INLINE size_t size() const NOEXCEPT
	{
		return m_pBuff->size;
	}
    
	INLINE void resize(size_t newsize) NOEXCEPT
	{
        bbuff_resize(m_pBuff, newsize);
	}
    
	INLINE void reserve(size_t ressize) NOEXCEPT
	{
		if(ressize > size())
		{
            bbuff_reserve(m_pBuff, ressize);
		}
	}
    
	INLINE void append(const std::string & str) NOEXCEPT
	{
		append((uint8*)str.c_str(), str.length());
		append<uint8>(0);
	}
    
	INLINE void append(const char * src, size_t cnt) NOEXCEPT
	{
		append((const uint8*)src, cnt);
	}
    
	INLINE void append(const void * src, size_t cnt) NOEXCEPT
	{
		append((const uint8*)src, cnt);
	}
    
	INLINE void append(const uint8 *src, size_t cnt) NOEXCEPT
	{
        bbuff_append(m_pBuff, src, cnt);
	}
    
	INLINE void append(const ByteBuffer& buffer) NOEXCEPT
	{
		if(buffer.size() > 0)
		{
			append(buffer.contents(), buffer.size());
		}
	}
    
	INLINE void put(size_t pos, const uint8 * src, size_t cnt) NOEXCEPT
	{
        bbuff_put(m_pBuff, pos, src, cnt);
	}
    
	void hexlike() NOEXCEPT
	{
		size_t j = 1, k = 1;
		printf("STORAGE_SIZE: %u\n", (unsigned int)size());
		for(size_t i = 0; i < size(); i++)
		{
			if ((i == (j*8)) && ((i != (k*16))))
			{
				if (read<uint8>(i) <= 0x0F)
				{
					printf("| 0%X ", read<uint8>(i));
				}
				else
				{
					printf("| %X ", read<uint8>(i));
				}
                
				j++;
			}
			else if (i == (k*16))
			{
				rpos(rpos()-16);	// move read pointer 16 places back
				printf(" | ");	  // write split char
                
				for (int x = 0; x <16; x++)
				{
					printf("%c", read<uint8>(i-16 + x));
				}
                
				if (read<uint8>(i) <= 0x0F)
				{
					printf("\n0%X ", read<uint8>(i));
				}
				else
				{
					printf("\n%X ", read<uint8>(i));
				}
                
				k++;
				j++;
			}
			else
			{
				if (read<uint8>(i) <= 0x0F)
				{
					printf("0%X ", read<uint8>(i));
				}
				else
				{
					printf("%X ", read<uint8>(i));
				}
			}
		}
		printf("\n");
	}
    
	INLINE void reverse() NOEXCEPT
	{
		std::reverse(m_pBuff->storage, m_pBuff->storage + m_pBuff->size);
	}
    
protected:
    bbuff*  m_pBuff;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename T> ByteBuffer &operator<<(ByteBuffer &b, std::vector<T> v)
{
	b <<(uint32)v.size();
	for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
		b <<*i;
	}
	return b;
}

template <typename T> ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v)
{
	uint32 vsize;
	b>> vsize;
	v.clear();
	while(vsize--) {
		T t;
		b>> t;
		v.push_back(t);
	}
	return b;
}

template <typename T> ByteBuffer &operator<<(ByteBuffer &b, std::list<T> v)
{
	b <<(uint32)v.size();
	for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i) {
		b <<*i;
	}
	return b;
}

template <typename T> ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v)
{
	uint32 vsize;
	b>> vsize;
	v.clear();
	while(vsize--) {
		T t;
		b>> t;
		v.push_back(t);
	}
	return b;
}

template <typename K, typename V> ByteBuffer &operator<<(ByteBuffer &b, std::map<K, V> &m)
{
	b << (uint32)m.size();
	for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
		b << i->first << i->second;
	}
	return b;
}

template <typename K, typename V> ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V> &m)
{
	uint32 msize;
	b >> msize;
	m.clear();
	while(msize--)
    {
		K k;
		V v;
		b >> k >> v;
		m.insert(make_pair(k, v));
	}
	return b;
}

#endif
