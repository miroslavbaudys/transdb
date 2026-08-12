//
//  CCommon.c
//
//  Created by Miroslav Kudrnac on 10.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#include "../zlib/zlib.h"
#include "CCommon.h"

#define GZIP_ENCODING				16

int CCommon_decompressGzip(const uint8 *pData, size_t dataLen, bbuff *buffOut, int zlibBufferSize)
{
    //set up stream
    z_stream stream;
    memset(&stream, 0, sizeof(z_stream));
    
    //init for gzip
    int ret = inflateInit2(&stream, GZIP_ENCODING+MAX_WBITS);
    if(ret != Z_OK)
        return ret;
 
	//buffer for zlib - VLA
#if !defined(WIN32) && !defined(__STDC_NO_VLA__)
	Bytef zlibBuff[zlibBufferSize];
#else
	Bytef *zlibBuff = alloca(sizeof(Bytef) * zlibBufferSize);
#endif
    
    /* decompress until deflate stream ends or end of file */
    stream.avail_in = (uInt)dataLen;
    stream.next_in = (Bytef*)pData;
    
    /* run inflate() on input until output buffer not full */
    do
    {
        stream.avail_out = zlibBufferSize;
        stream.next_out = zlibBuff;
        ret = inflate(&stream, Z_FINISH);
        switch (ret)
        {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                inflateEnd(&stream);
                return ret;
            }
        }
        
        uInt processed = zlibBufferSize - stream.avail_out;
        bbuff_append(buffOut, zlibBuff, processed);
        
    }while(stream.avail_out == 0);
    
    /* clean up and return */
    inflateEnd(&stream);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int CCommon_compressGzip(int compressionLevel, const uint8 *pData, size_t dataLen, bbuff *buffOut, int zlibBufferSize)
{
    //set up stream
    z_stream stream;
    memset(&stream, 0, sizeof(z_stream));
    
    //init for gzip
    int ret = deflateInit2(&stream, compressionLevel, Z_DEFLATED, GZIP_ENCODING+MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if(ret != Z_OK)
        return ret;
    
	//buffer for zlib - VLA
#if !defined(WIN32) && !defined(__STDC_NO_VLA__)
	Bytef zlibBuff[zlibBufferSize];
#else
	Bytef *zlibBuff = alloca(sizeof(Bytef) * zlibBufferSize);
#endif
    
    //setup input informations
    stream.next_in = (Bytef*)pData;
    stream.avail_in = (uInt)dataLen;
    
    do
    {
        stream.avail_out = zlibBufferSize;
        stream.next_out = zlibBuff;
        ret = deflate(&stream, Z_FINISH);
        switch (ret)
        {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                deflateEnd(&stream);
                return ret;
            }
        }
        
        uInt processed = zlibBufferSize - stream.avail_out;
        bbuff_append(buffOut, zlibBuff, processed);
        
    }while(stream.avail_out == 0);
    
    /* clean up and return */
    deflateEnd(&stream);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int CCommon_compressGzip_Buffer(int compressionLevel,
                                const uint8 *pData,
                                size_t dataLen,
                                uint8 *dstBuffer,
                                size_t dstBufferSize,
                                size_t *outputSize)
{
    //set up stream
    z_stream stream;
    memset(&stream, 0, sizeof(z_stream));
    
    //clear output size
    *outputSize = 0;
    
    //init for gzip
    int ret = deflateInit2(&stream, compressionLevel, Z_DEFLATED, GZIP_ENCODING+MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if(ret != Z_OK)
        return ret;
    
    //setup input informations
    stream.next_in = (Bytef*)pData;
    stream.avail_in = (uInt)dataLen;
    
    //process in one call
    do
    {
        //setup output informations
        stream.avail_out = (uInt)(dstBufferSize - *outputSize);
        stream.next_out = dstBuffer;
        
        ret = deflate(&stream, Z_FINISH);
        switch (ret)
        {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                deflateEnd(&stream);
                return ret;
            }
        }
        //compressed data size
        *outputSize += dstBufferSize - stream.avail_out;
        
    }while(stream.avail_out == 0);
    
    /* clean up and return */
    deflateEnd(&stream);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

bool CCommon_isGziped(const uint8 *pData)
{
    return (pData[0] == 0x1f) && (pData[1] == 0x8b);
}

bool CCommon_CheckFileExists(const char *pFileName, bool oCreate)
{
	bool oReturnVal = true;
	FILE * rFile;
	rFile = fopen(pFileName, "rb");
	if(rFile == NULL && oCreate)
	{
		//create file
		rFile = fopen(pFileName, "wb+");
		oReturnVal = false;
	}
    
	if(rFile)
	{
		fclose(rFile);
	}
	
	return oReturnVal;
}

time_t CCommon_GetLastFileModificationTime(const char *pFilePath)
{
#ifdef WIN32
	struct _stat rStat;
	if(_stat(pFilePath, &rStat) != -1)
		return rStat.st_mtime;
	else
		return 0;
#else
	struct stat rStat;
    if(stat(pFilePath, &rStat) != -1)
        return rStat.st_mtime;
    else
		return 0;
#endif
}

unsigned int CCommon_MakeIP(const char * str)
{
    // convert the input IP address to an integer
    unsigned int address = inet_addr(str);
    return address;
}

bool CCommon_isDigit(const unsigned char c)
{
    return isdigit(c) != 0;
}

bool CCommon_isWhiteSpace(const unsigned char c)
{
    return isspace(c) != 0;
}
