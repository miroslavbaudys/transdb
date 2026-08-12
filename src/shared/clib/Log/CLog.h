//
//  CLog.h
//
//  Created by Miroslav Kudrnac on 15.09.14.
//  Copyright (c) 2014 Miroslav Kudrnac. All rights reserved.
//

#ifndef CLOG_H
#define CLOG_H 1

#ifdef __cplusplus
extern "C" {
#endif

void Log_Notice(const char * source, const char * format, ...);
void Log_Warning(const char * source, const char * format, ...);
void Log_Success(const char * source, const char * format, ...);
void Log_Error(const char * source, const char * format, ...);
void Log_Debug(const char * source, const char * format, ...);
    
#ifdef __cplusplus
}
#endif
    
#endif
