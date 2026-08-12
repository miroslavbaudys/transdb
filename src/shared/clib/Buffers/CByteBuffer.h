//
//  CByteBuffer.h
//
//  Created by Miroslav Kudrnac on 10.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef CBYTEBUFFER_H
#define CBYTEBUFFER_H 1

#include "../../Defines.h"

typedef struct CByteBuffer
{
    uint8*  storage;
    size_t  size;
    size_t  capacity;
	size_t  rpos;
	size_t  wpos;
} bbuff;

/** Create bytebuffer, storage is not allocated
 */
#define bbuff_create(self)                                                                  \
    do                                                                                      \
    {                                                                                       \
        self = (bbuff*)_CALLOC(1, sizeof(bbuff));                                           \
    }while(0)

/** Destroy bytebuffer and free storage
 */
#define bbuff_destroy(self)                                                                 \
    do                                                                                      \
    {                                                                                       \
        _FREE((self)->storage);                                                             \
        _FREE(self);                                                                        \
    }while(0)
    
    
/** Reserve space in storage
 */
#define bbuff_reserve(self, ressize)                                                        \
    do                                                                                      \
    {                                                                                       \
        size_t _bb_ressize = (ressize);                                                     \
        if(_bb_ressize > (self)->capacity)                                                  \
        {                                                                                   \
            (self)->capacity = _bb_ressize;                                                 \
            if(((self)->storage = (uint8*)_REALLOC((self)->storage, _bb_ressize)) == NULL)  \
            {                                                                               \
                assert(false);                                                              \
            }                                                                               \
        }                                                                                   \
    }while(0)
    
/** Resize storage
 */
#define bbuff_resize(self, newsize)                                                         \
    do                                                                                      \
    {                                                                                       \
        size_t _bb_newsize = (newsize);                                                     \
        bbuff_reserve(self, _bb_newsize);                                                   \
        (self)->size = _bb_newsize;                                                         \
        (self)->rpos = 0;                                                                   \
        (self)->wpos = (self)->size;                                                        \
    }while(0)

/** Append data to bytebuffer
 */
#define bbuff_append(self, src, len)                                                        \
    do                                                                                      \
    {                                                                                       \
        size_t _bb_len = (len);                                                             \
        if(_bb_len > 0)                                                                     \
        {                                                                                   \
            if((self)->size < ((self)->wpos + _bb_len))                                     \
            {                                                                               \
                bbuff_reserve(self, (self)->wpos + _bb_len);                                \
                (self)->size = (self)->wpos + _bb_len;                                      \
            }                                                                               \
            memcpy((self)->storage + (self)->wpos, src, _bb_len);                           \
            (self)->wpos += _bb_len;                                                        \
        }                                                                                   \
    }while(0)
   
/** Read data from bytebuffer
 */
#define bbuff_read(self, dst, len)                                                          \
    do                                                                                      \
    {                                                                                       \
        size_t _bb_len = (len);                                                             \
        if(((self)->rpos + _bb_len) <= (self)->size)                                        \
        {                                                                                   \
            memcpy(dst, (self)->storage + (self)->rpos, _bb_len);                           \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            memset(dst, 0, _bb_len);                                                        \
        }                                                                                   \
        (self)->rpos += _bb_len;                                                            \
    }while(0)
    
/** Put data to bytebuffer to specified position
 */
#define bbuff_put(self, pos, src, len)                                                      \
    do                                                                                      \
    {                                                                                       \
        size_t _bb_pos = (pos);                                                             \
        size_t _bb_len = (len);                                                             \
        assert(_bb_pos + _bb_len <= (self)->size);                                          \
        memcpy((self)->storage + _bb_pos, src, _bb_len);                                    \
    }while(0)
    
/** Clear bytebuffer data
 */
#define bbuff_clear(self)                                                                   \
    do                                                                                      \
    {                                                                                       \
        _FREE((self)->storage);                                                             \
        memset(self, 0, sizeof(bbuff));                                                     \
    }while(0)

#endif
