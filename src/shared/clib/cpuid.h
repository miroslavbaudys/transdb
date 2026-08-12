//
//  cpuid.h
//
//  Created by Miroslav Kudrnac on 02.05.13.
//  Copyright (c) 2013 Miroslav Kudrnac. All rights reserved.
//

#ifndef CPUID_H
#define CPUID_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include "../Defines.h"

/* SSE42 flag */
#define SSE42_FEATURE_BIT   (1 << 20)
/* AVX flag */
#define AVX_FEATURE_BIT     (1 << 28)
/* CPU features */
#define CPUID_FEATURES      1

/* CPUID function */
#ifdef WIN32
    static void cpuid(int *CPUInfo, int InfoType)
    {
        __cpuid(CPUInfo, InfoType);
    }
#else
    static inline void cpuid(int *CPUInfo, unsigned int number)
    {
        /* Store number in %eax, call CPUID, save %eax, %ebx, %ecx, %edx in variables.
         As output constraint "m=" has been used as this keeps gcc's optimizer
         from overwriting %eax, %ebx, %ecx, %edx by accident */
        __asm__("movl %4, %%eax; cpuid; movl %%eax, %0; movl %%ebx, %1; movl %%ecx, %2; movl %%edx, %3;"
        : "=m" (CPUInfo[0]),
            "=m" (CPUInfo[1]),
            "=m" (CPUInfo[2]),
            "=m" (CPUInfo[3])				  /* output */
            : "r"  (number)                   /* input */
            : "eax", "ebx", "ecx", "edx"      /* no changed registers except output registers */
            );
    }
#endif

#ifdef __cplusplus
}
#endif

#endif
