//
//  crc32.c
//
//  Created by Miroslav Kudrnac on 15.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#include "../cpuid.h"
#include "../Log/CLog.h"
#include "crc32_sse.h"

#ifdef WIN32
    #include <nmmintrin.h>
#else
    static inline uint32_t _mm_crc32_u32(uint32_t crc, uint32_t value)
    {
        __asm__("crc32l %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
        return crc;
    }

    static inline uint32_t _mm_crc32_u8(uint32_t crc, uint8_t value)
    {
        __asm__("crc32b %[value], %[crc]\n" : [crc] "+r" (crc) : [value] "rm" (value));
        return crc;
    }
#endif

// reversed 0x1EDC6F41
#define CRCPOLY	0x82f63b78
#define CRCINIT	0xFFFFFFFF

// Slicing-by-4 and slicing-by-8 algorithms by Michael E. Kounavis and Frank L. Berry from Intel Corp.
// http://www.intel.com/technology/comms/perfnet/download/CRC_generators.pdf
UINT m_crc_slicing[8][256];

//function called base on if CPU has HW support for crc32
typedef UINT (*crc32_func_t)(const BYTE * buf, SIZE_T len);

void crc32_slicing_init(void)
{
    for (uint32 i = 0; i <= 0xFF; i++) {
        uint32 x = i;
        for (uint32 j = 0; j < 8; j++)
            x = (x>>1) ^ (CRCPOLY & (-(int)(x & 1)));
        m_crc_slicing[0][i] = x;
    }
    
    for (uint32 i = 0; i <= 0xFF; i++) {
        uint32 c = m_crc_slicing[0][i];
        for (uint32 j = 1; j < 8; j++) {
            c = m_crc_slicing[0][c & 0xFF] ^ (c >> 8);
            m_crc_slicing[j][i] = c;
        }
    }
}

SIZE_T Min(SIZE_T x, SIZE_T y)
{
    // Branchless minimum (from http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax)
    //return y ^ ((x ^ y) & -(x < y));
    return (x < y) ? x : y; // CMOV is used when /arch:SSE2 is on
}

UINT crc32_slicing_by_8(const BYTE* buf, SIZE_T len)
{
    UINT crc = CRCINIT;
    
    // Align to DWORD boundary
    SIZE_T align = (sizeof(DWORD) - (INT_PTR)buf) & (sizeof(DWORD) - 1);
    align = Min(align, len);
    len -= align;
    for (; align; align--)
        crc = m_crc_slicing[0][(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    
    SIZE_T nqwords = len / (sizeof(DWORD) + sizeof(DWORD));
    for (; nqwords; nqwords--) {
        crc ^= *(DWORD*)buf;
        buf += sizeof(DWORD);
        UINT next = *(DWORD*)buf;
        buf += sizeof(DWORD);
        crc =
        m_crc_slicing[7][(crc      ) & 0xFF] ^
        m_crc_slicing[6][(crc >>  8) & 0xFF] ^
        m_crc_slicing[5][(crc >> 16) & 0xFF] ^
        m_crc_slicing[4][(crc >> 24)] ^
        m_crc_slicing[3][(next      ) & 0xFF] ^
        m_crc_slicing[2][(next >>  8) & 0xFF] ^
        m_crc_slicing[1][(next >> 16) & 0xFF] ^
        m_crc_slicing[0][(next >> 24)];
    }
    
    len &= sizeof(DWORD) * 2 - 1;
    for (; len; len--)
        crc = m_crc_slicing[0][(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

UINT crc32_hardware_unrolled(const BYTE * buf, SIZE_T len)
{
    UINT crc = CRCINIT;
    
    // Align to DWORD boundary
    SIZE_T align = (sizeof(DWORD) - (INT_PTR)buf) & (sizeof(DWORD) - 1);
    align = Min(align, len);
    len -= align;
    for (; align; align--)
        crc = m_crc_slicing[0][(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
    
    SIZE_T ndqwords = len / (sizeof(DWORD) * 4);
    for (; ndqwords; ndqwords--) {
        crc = _mm_crc32_u32(crc, *(DWORD*)buf);
        crc = _mm_crc32_u32(crc, *(DWORD*)(buf + sizeof(DWORD) ));
        crc = _mm_crc32_u32(crc, *(DWORD*)(buf + sizeof(DWORD) * 2 ));
        crc = _mm_crc32_u32(crc, *(DWORD*)(buf + sizeof(DWORD) * 3 ));
        buf += sizeof(DWORD) * 4;
    }
    
    len &= sizeof(DWORD) * 4 - 1;
    for (; len; len--)
        crc = _mm_crc32_u8(crc, *buf++);
    return ~crc;
}

//will be set from crc32_init()
crc32_func_t m_crc32_func;

void crc32_init(void)
{
    //init slicing
    crc32_slicing_init();
    
    //get CPU instruction support
    int CPUInfo[4];
    cpuid(CPUInfo, CPUID_FEATURES);
    
    //init function handler
    if(CPUInfo[2] & SSE42_FEATURE_BIT)
    {
        Log_Notice(__FUNCTION__, "SSE42 supported. Using Crc32 with HW support.");
        m_crc32_func = crc32_hardware_unrolled;
    }
    else
    {
        Log_Notice(__FUNCTION__, "SSE42 not supported. Using Crc32 without HW support.");
        m_crc32_func = crc32_slicing_by_8;
    }
}

uint32 crc32_compute(const BYTE* buf, SIZE_T len)
{
    uint32 crc32 = m_crc32_func(buf, len);
    return crc32;
}




