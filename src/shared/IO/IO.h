//
//  IO.h
//  TransDB
//
//  Created by Miroslav Kudrnac on 19.04.13.
//  Copyright (c) 2013 Miroslav Kudrnac. All rights reserved.
//

#ifndef __TransDB__IO__
#define __TransDB__IO__

#include "../Defines.h"

class IO
{
public:
    enum ACCESS
    {
        IO_READ_ONLY    = 1,
        IO_WRITE_ONLY   = 2,
        IO_RDWR         = 3
    };

    enum FLAGS
    {
        IO_NORMAL       = 1,
        IO_DIRECT       = 2
    };
    
    enum SEEK_POS
    {
#ifdef WIN32
        IO_SEEK_SET     = FILE_BEGIN,
        IO_SEEK_CUR     = FILE_CURRENT,
        IO_SEEK_END     = FILE_END
#else
        IO_SEEK_SET     = SEEK_SET,
        IO_SEEK_CUR     = SEEK_CUR,
        IO_SEEK_END     = SEEK_END
#endif
    };
    
    /** Open file
	 */
    static HANDLE fopen(const char *pPath, ACCESS eAccess, FLAGS eFlags);
    
   	/** Get pos in file
	 */
	static int64 ftell(HANDLE hFile);
    
   	/** Seek in file
	 */
	static int64 fseek(HANDLE hFile, int64 offset, SEEK_POS eSeekPos);
    
   	/** Write data to file
	 */
	static size_t fwrite(const void *pBuffer, size_t nNumberOfBytesToWrite, HANDLE hFile);
    
   	/** Read data from file
	 */
	static size_t fread(void *pBuffer, size_t nNumberOfBytesToRead, HANDLE hFile);
    
   	/** Resize file
	 */
	static void fresize(HANDLE hFile, int64 newSize);
    
   	/** Close file handle
	 */
	static void fclose(HANDLE hFile);
    
   	/** Sync data to device
	 */
	static void fsync(HANDLE hFile);
    
    /** Get error number
     */
    static int ferror() NOEXCEPT;
};

class IOHandleGuard
{
public:
    explicit IOHandleGuard(const HANDLE &hHandle) : m_rHandle(hHandle)
    {
        
    }
    
    ~IOHandleGuard()
    {
        if(m_rHandle != INVALID_HANDLE_VALUE)
        {
            IO::fclose(m_rHandle);
        }
    }
    
private:
   	//disable copy constructor and assign
	DISALLOW_COPY_AND_ASSIGN(IOHandleGuard);
    
    //reference to handle
    const HANDLE &m_rHandle;
};

#endif /* defined(__TransDB__IO__) */
