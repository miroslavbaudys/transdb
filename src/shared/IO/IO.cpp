//
//  IO.cpp
//  TransDB
//
//  Created by Miroslav Kudrnac on 19.04.13.
//  Copyright (c) 2013 Miroslav Kudrnac. All rights reserved.
//

#include "IO.h"

HANDLE IO::fopen(const char *pPath, ACCESS eAccess, FLAGS eFlags)
{
    HANDLE hHandle = INVALID_HANDLE_VALUE;
#ifdef WIN32
    DWORD dwDesiredAccess = 0;
    DWORD dwFlagsAndAttributes = 0;
    switch(eAccess)
    {
        case IO_READ_ONLY:
            dwDesiredAccess = GENERIC_READ;
            dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
            break;
        case IO_WRITE_ONLY:
            dwDesiredAccess = GENERIC_WRITE;
            break;
        case IO_RDWR:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            break;
    }
    
    //handle flags
    if(eFlags & IO_NORMAL)
        dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
    else if(eFlags & IO_DIRECT)
        dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
    
#if defined(WIN32) && !defined(WP8)
    hHandle = CreateFile(pPath, dwDesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, dwFlagsAndAttributes, NULL);
#else
	wchar_t awPath[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, pPath, -1, awPath, sizeof(awPath));
	hHandle = CreateFile2(awPath, dwDesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, NULL);
#endif
#else
    int flags = 0;
    switch(eAccess)
    {
        case IO_READ_ONLY:
            flags |= O_RDONLY;
            break;
        case IO_WRITE_ONLY:
            flags |= O_WRONLY;
            break;
        case IO_RDWR:
            flags |= O_RDWR;
            break;
    }
    
#ifndef MAC
    //handle flags
    if(eFlags & IO_DIRECT)
        flags |= O_DIRECT;
    
    //open file
    hHandle = ::open(pPath, flags);
#else
    //open file
    hHandle = ::open(pPath, flags);
    
    //handle flags
    if(hHandle != INVALID_HANDLE_VALUE)
    {
        if(eFlags & IO_DIRECT)
        {
            if(fcntl(hHandle, F_NOCACHE, 1) < 0)
            {
                IO::fclose(hHandle);
                hHandle = INVALID_HANDLE_VALUE;
            }
        }
    }
#endif
#endif
    return hHandle;
}

int64 IO::ftell(HANDLE hFile)
{
#ifdef WIN32
    LARGE_INTEGER liDistanceToMove = { 0 };
    LARGE_INTEGER lpNewFilePointer;
    
    BOOL ret = SetFilePointerEx(hFile, liDistanceToMove, &lpNewFilePointer, FILE_CURRENT);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: SetFilePointerEx failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
    return lpNewFilePointer.QuadPart;
#else
    off_t ret = ::lseek(hFile, 0, SEEK_CUR);
    if(ret == static_cast<off_t>(-1))
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: lseek failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
    return static_cast<int64>(ret);
#endif
}

int64 IO::fseek(HANDLE hFile, int64 offset, SEEK_POS eSeekPos)
{
    int origin = static_cast<int>(eSeekPos);
#ifdef WIN32
    LARGE_INTEGER liDistanceToMove;
    LARGE_INTEGER lpNewFilePointer;
    
    liDistanceToMove.QuadPart = offset;
    BOOL ret = SetFilePointerEx(hFile, liDistanceToMove, &lpNewFilePointer, origin);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: SetFilePointerEx failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
    return lpNewFilePointer.QuadPart;
#else
    off_t ret = ::lseek(hFile, offset, origin);
    if(ret == static_cast<off_t>(-1))
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: lseek failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
    return static_cast<int64>(ret);
#endif
}

size_t IO::fwrite(const void *pBuffer, size_t nNumberOfBytesToWrite, HANDLE hFile)
{
#ifdef WIN32
    DWORD lpNumberOfBytesWritten = 0;
    BOOL ret = WriteFile(hFile, pBuffer, (DWORD)nNumberOfBytesToWrite, &lpNumberOfBytesWritten, NULL);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: WriteFile failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }

    //check how many bytes are written by write
    if(lpNumberOfBytesWritten != (DWORD)nNumberOfBytesToWrite)
    {
	char rError[512];
	snprintf(rError, sizeof(rError), "%s: WriteFile failed bytes to write: %u, bytes writen by write: %u", __FUNCTION__, (uint32)nNumberOfBytesToWrite, (uint32)lpNumberOfBytesWritten);
	throw std::runtime_error(rError);
    }
#else
    ssize_t lpNumberOfBytesWritten = ::write(hFile, pBuffer, nNumberOfBytesToWrite);
    if(lpNumberOfBytesWritten == -1)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: write failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }

    //check how many bytes are written by write
    if (lpNumberOfBytesWritten != (ssize_t)nNumberOfBytesToWrite)
    {
	char rError[512];
	snprintf(rError, sizeof(rError), "%s: write failed bytes to write: %lu, bytes writen by write: %lu", __FUNCTION__, (unsigned long)nNumberOfBytesToWrite, (unsigned long)lpNumberOfBytesWritten);
	throw std::runtime_error(rError);
    }
#endif
    return lpNumberOfBytesWritten;
}

size_t IO::fread(void *pBuffer, size_t nNumberOfBytesToRead, HANDLE hFile)
{
#ifdef WIN32
    DWORD lpNumberOfBytesRead = 0;
    BOOL ret = ReadFile(hFile, pBuffer, (DWORD)nNumberOfBytesToRead, &lpNumberOfBytesRead, NULL);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: ReadFile failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
#else
    ssize_t lpNumberOfBytesRead = ::read(hFile, pBuffer, nNumberOfBytesToRead);
    if(lpNumberOfBytesRead == -1)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: read failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
#endif
    return lpNumberOfBytesRead;
}

void IO::fresize(HANDLE hFile, int64 newSize)
{
#ifdef WIN32
    BOOL ret;
	LARGE_INTEGER liDistanceToMove;
    LARGE_INTEGER lpNewFilePointer;
    
    liDistanceToMove.QuadPart = newSize;
    ret = SetFilePointerEx(hFile, liDistanceToMove, &lpNewFilePointer, FILE_BEGIN);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: SetFilePointerEx failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
    
	ret = SetEndOfFile(hFile);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: SetEndOfFile failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
#else
    int ret = ::ftruncate(hFile, newSize);
    if(ret == -1)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: ftruncate failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
#endif
}

void IO::fclose(HANDLE hFile)
{
#ifdef WIN32
    BOOL ret = CloseHandle(hFile);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: CloseHandle failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
#else
    int ret = ::close(hFile);
    if(ret == -1)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: close failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
#endif
}

void IO::fsync(HANDLE hFile)
{
#ifdef WIN32
    BOOL ret = ::FlushFileBuffers(hFile);
    if(ret == FALSE)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: FlushFileBuffers failed errno: %d", __FUNCTION__, GetLastError());
        throw std::runtime_error(rError);
    }
#else
    int ret = ::fsync(hFile);
    if(ret == -1)
    {
        char rError[512];
        snprintf(rError, sizeof(rError), "%s: fsync failed errno: %d", __FUNCTION__, errno);
        throw std::runtime_error(rError);
    }
#endif
}

int IO::ferror() NOEXCEPT
{
#ifdef WIN32
    return GetLastError();
#else
    return errno;
#endif
    
}









