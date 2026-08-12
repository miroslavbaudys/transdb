//
//  CCommon.h
//
//  Created by Miroslav Kudrnac on 10.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef CCOMMON_H
#define CCOMMON_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include "../Defines.h"
#include "Buffers/CByteBuffer.h"

/**
 */
int CCommon_decompressGzip(const uint8 *pData, size_t dataLen, bbuff *buffOut, int zlibBufferSize);

/**
 */
int CCommon_compressGzip(int compressionLevel, const uint8 *pData, size_t dataLen, bbuff *buffOut, int zlibBufferSize);

/**
 */
int CCommon_compressGzip_Buffer(int compressionLevel, const uint8 *pData, size_t dataLen, uint8 *dstBuffer, size_t dstBufferSize, size_t *outputSize);
    
/**
 */
bool CCommon_isGziped(const uint8 *pData);

/**
 */
bool CCommon_CheckFileExists(const char *pFileName, bool oCreate);

/**
 */
time_t CCommon_GetLastFileModificationTime(const char *pFilePath);

/**
 */
unsigned int CCommon_MakeIP(const char * str);

/**
 */
bool CCommon_isDigit(const unsigned char c);

/**
 */
bool CCommon_isWhiteSpace(const unsigned char c);

#ifdef __cplusplus
}
#endif
    
#endif
