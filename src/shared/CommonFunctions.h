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

#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include "Defines.h"
#include "Packets/ByteBuffer.h"
#include "zlib/zlib.h"

class CommonFunctions
{
public:
	static int decompressGzip(const uint8 *pData, size_t dataLen, ByteBuffer &rBuffOut, int zlibBufferSize);
	static int compressGzip(int compressionLevel, const uint8 *pData, size_t dataLen, ByteBuffer &rBuffOut, int zlibBufferSize);
    static INLINE bool isGziped(const uint8 *pData)
    {
        return (pData[0] == 0x1f) && (pData[1] == 0x8b);
    }
    
    static bool CheckFileExists(const char *pFileName, bool oCreate);
    static time_t GetLastFileModificationTime(const char *pFilePath);
    
    static std::vector<std::string> StrSplit(const std::string & src, const std::string & sep);
    static void replace(std::string &str, const char* find, const char* rep, uint32 limit = 0);
    static std::string FormatOutputString(const char * Prefix, const char * Description, bool useTimeStamp);
    static void SetThreadName(const char* format, ...);
    
    static INLINE uint32 hex2number(const char *pHexNumber)
    {
        uint32 ret;
        std::stringstream ss;
        ss << std::hex << pHexNumber;
        ss >> ret;
        return ret;
    }
    
    static INLINE unsigned int MakeIP(const char * str)
    {
        // convert the input IP address to an integer
        unsigned int address = inet_addr(str);
        return address;
    }
    
    static INLINE bool isDigit(const unsigned char c)
    {
        return ::isdigit(c) != 0;
    }
    
    static INLINE bool isWhiteSpace(const unsigned char c)
    {
        return ::isspace(c) != 0;
    }
    
};

#endif
