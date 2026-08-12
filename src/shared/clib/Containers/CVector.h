//
//  CVector.h
//
//  Created by Miroslav Kudrnac on 10.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef CVECTOR_H
#define CVECTOR_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include "../../Defines.h"

typedef struct CVector
{
    uint8*  data;
    size_t  size;
    size_t  capacity;
    size_t  itemSize;
} cvector;


/**
 */
cvector *cvector_create(size_t itemSize);

/**
 */
void cvector_destroy(cvector* self);

/**
 */
void cvector_clear(cvector* self);

/**
 */
void cvector_reserve(cvector* self, size_t newCapacity);

/**
 */
void cvector_resize(cvector* self, size_t newSize);

/**
 */
void cvector_push_back(cvector* self, const void *value);


#ifdef __cplusplus
}
#endif

#endif
