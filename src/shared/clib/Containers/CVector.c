//
//  CVector.c
//
//  Created by Miroslav Kudrnac on 10.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#include "CVector.h"

cvector *cvector_create(size_t itemSize)
{
    cvector *vec = calloc(1, sizeof(cvector));
    if(vec == NULL)
        return NULL;
    
    vec->itemSize = itemSize;
    return vec;
}

void cvector_destroy(cvector* self)
{
    free(self->data);
    free(self);
}

void cvector_clear(cvector* self)
{
    free(self->data);
    self->size = 0;
    self->capacity = 0;
}

void cvector_reserve(cvector* self, size_t newCapacity)
{
    //for WP8 - C89
    void *pNewBuffer;
    
    if(newCapacity > self->capacity)
    {
        //set capacity
        self->capacity = newCapacity;
        //resize buffer
        pNewBuffer = realloc(self->data, self->itemSize * newCapacity);
        if(pNewBuffer == NULL)
            return;
        
        self->data = pNewBuffer;
    }
}

void cvector_resize(cvector* self, size_t newSize)
{
    cvector_reserve(self, newSize);
    self->size = newSize;
}

void cvector_push_back(cvector* self, const void *value)
{
    //for WP8 - C89
    void *dst;
    
    if(self->capacity == self->size)
    {
        size_t newCapacity = self->capacity == 0 ? 16 : self->capacity * 2;
        cvector_reserve(self, newCapacity);
    }
    
    dst = (self->data + (self->itemSize * self->size));
    memcpy(dst, value, self->itemSize);
    ++self->size;
}










