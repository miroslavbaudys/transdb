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

#include "Log.h"
#include "../clib/Log/CLog.h"

#ifdef ANDROID
    #include <android/log.h>
#endif

createFileSingleton(ScreenLog);

ScreenLog::ScreenLog()
{
	m_log_level		= 3;
#if defined(WIN32) && !defined(WP8)
	m_stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
	m_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
}

ScreenLog::~ScreenLog()
{

}

void ScreenLog::CreateFileLog(const std::string &sFilePath)
{
    m_pFileLog = std::unique_ptr<FileLog>(new FileLog(sFilePath));
}

void ScreenLog::Color(unsigned int color)
{
#ifndef WIN32
#ifndef MAC
		static const char* colorstrings[TBLUE+1] = {
			"",
				"\033[22;31m",
				"\033[22;32m",
				"\033[01;33m",
				"\033[0m",
				"\033[01;37m",
				"\033[1;34m",
		};
		fputs(colorstrings[color], stdout);
#endif
#elif defined(WP8)
	//nothing
#else
	SetConsoleTextAttribute(m_stdout_handle, (WORD)color);
#endif
}

void ScreenLog::Time()
{
#ifdef WP8
	char szBuf[512] = { 0 };
	wchar_t sWBuff[512] = { 0 };
	snprintf(szBuf, sizeof(szBuf), "%02u:%02u:%02u ", (uint32)g_localTime.tm_hour, (uint32)g_localTime.tm_min, (uint32)g_localTime.tm_sec);
	MultiByteToWideChar(CP_UTF8, 0, szBuf, -1, sWBuff, sizeof(sWBuff));
	OutputDebugString(sWBuff);
#else
	printf("%02u:%02u:%02u ", (uint32)g_localTime.tm_hour, (uint32)g_localTime.tm_min, (uint32)g_localTime.tm_sec);
#endif
}

void ScreenLog::Line()
{
	putchar('\n');
}

void ScreenLog::Notice(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
	NoticeVA(source, format, ap);
    va_end(ap);
}

void ScreenLog::NoticeVA(const char * source, const char * format, va_list ap)
{
    if(m_log_level < 0)
        return;
    
    //lock
    std::lock_guard<std::mutex> rGuard(m_lock);
    
    //check is logging is to file
    if(m_pFileLog && m_pFileLog->IsOpen())
    {
        //write to disk
        m_pFileLog->write(source, "N", format, ap);
    }
    else
    {
#if defined(ANDROID)
        __android_log_vprint(ANDROID_LOG_INFO, source, format, ap);
#elif defined(WP8)
        char scBuff[16384] = { 0 };
        wchar_t szBuf[16384] = { 0 };
        
        Time();
        OutputDebugString(L"N ");
        if (*source)
        {
            //convert to wchar
            wchar_t sWBuff[512] = { 0 };
            MultiByteToWideChar(CP_UTF8, 0, source, -1, sWBuff, sizeof(sWBuff));
            OutputDebugString(sWBuff);
            OutputDebugString(L": ");
        }
        
        vsnprintf(scBuff, sizeof(scBuff), format, ap);
        //convert to wchar
        MultiByteToWideChar(CP_UTF8, 0, scBuff, -1, szBuf, sizeof(szBuf));
        OutputDebugString(szBuf);
        OutputDebugString(L"\n");
#else
        Time();
        fputs("N ", stdout);
        if (*source)
        {
            Color(TWHITE);
            fputs(source, stdout);
            putchar(':');
            putchar(' ');
            Color(TNORMAL);
        }
        
        vprintf(format, ap);
        putchar('\n');
        Color(TNORMAL);
#endif
    }
}

void ScreenLog::Warning(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
	WarningVA(source, format, ap);
    va_end(ap);
}

void ScreenLog::WarningVA(const char * source, const char * format, va_list ap)
{
    if(m_log_level < 2)
        return;
    
    //lock
    std::lock_guard<std::mutex> rGuard(m_lock);
    
    //check is logging is to file
    if(m_pFileLog && m_pFileLog->IsOpen())
    {
        //write to disk
        m_pFileLog->write(source, "W", format, ap);
    }
    else
    {
#if defined(ANDROID)
		__android_log_vprint(ANDROID_LOG_WARN, source, format, ap);
#elif defined(WP8)
		char scBuff[16384] = { 0 };
		wchar_t szBuf[16384] = { 0 };

		Time();
		OutputDebugString(L"W ");
		if (*source)
		{
			//convert to wchar
			wchar_t sWBuff[512] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, source, -1, sWBuff, sizeof(sWBuff));
			OutputDebugString(sWBuff);
			OutputDebugString(L": ");
		}

		vsnprintf(scBuff, sizeof(scBuff), format, ap);
		//convert to wchar
		MultiByteToWideChar(CP_UTF8, 0, scBuff, -1, szBuf, sizeof(szBuf));
		OutputDebugString(szBuf);
		OutputDebugString(L"\n");
#else
		Time();
		Color(TYELLOW);
		fputs("W ", stdout);
		if (*source)
		{
			Color(TWHITE);
			fputs(source, stdout);
			putchar(':');
			putchar(' ');
			Color(TYELLOW);
		}

		vprintf(format, ap);
		putchar('\n');
		Color(TNORMAL);
#endif
    }
}

void ScreenLog::Success(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
	SuccessVA(source, format, ap);
    va_end(ap);
}

void ScreenLog::SuccessVA(const char * source, const char * format, va_list ap)
{
    if(m_log_level < 2)
        return;
    
    //lock
    std::lock_guard<std::mutex> rGuard(m_lock);
    
    //check is logging is to file
    if(m_pFileLog && m_pFileLog->IsOpen())
    {
        //write to disk
        m_pFileLog->write(source, "S", format, ap);
    }
    else
    {
#if defined(ANDROID)
		__android_log_vprint(ANDROID_LOG_INFO, source, format, ap);
#elif defined(WP8)
		char scBuff[16384] = { 0 };
		wchar_t szBuf[16384] = { 0 };

		Time();
		OutputDebugString(L"S ");
		if (*source)
		{
			//convert to wchar
			wchar_t sWBuff[512] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, source, -1, sWBuff, sizeof(sWBuff));
			OutputDebugString(sWBuff);
			OutputDebugString(L": ");
		}

		vsnprintf(scBuff, sizeof(scBuff), format, ap);
		//convert to wchar
		MultiByteToWideChar(CP_UTF8, 0, scBuff, -1, szBuf, sizeof(szBuf));
		OutputDebugString(szBuf);
		OutputDebugString(L"\n");
#else
		Time();
		Color(TGREEN);
		fputs("S ", stdout);
		if (*source)
		{
			Color(TWHITE);
			fputs(source, stdout);
			putchar(':');
			putchar(' ');
			Color(TGREEN);
		}

		vprintf(format, ap);
		putchar('\n');
		Color(TNORMAL);
#endif
    }
}

void ScreenLog::Error(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
	ErrorVA(source, format, ap);
    va_end(ap);
}

void ScreenLog::ErrorVA(const char * source, const char * format, va_list ap)
{
    if(m_log_level < 1)
        return;
    
    //lock
    std::lock_guard<std::mutex> rGuard(m_lock);
    
    //check is logging is to file
    if(m_pFileLog && m_pFileLog->IsOpen())
    {
        //write to disk
        m_pFileLog->write(source, "E", format, ap);
    }
    else
    {
#if defined(ANDROID)
		__android_log_vprint(ANDROID_LOG_ERROR, source, format, ap);
#elif defined(WP8)
		char scBuff[16384] = { 0 };
		wchar_t szBuf[16384] = { 0 };

		Time();
		OutputDebugString(L"E ");
		if (*source)
		{
			//convert to wchar
			wchar_t sWBuff[512] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, source, -1, sWBuff, sizeof(sWBuff));
			OutputDebugString(sWBuff);
			OutputDebugString(L": ");
		}

		vsnprintf(scBuff, sizeof(scBuff), format, ap);
		//convert to wchar
		MultiByteToWideChar(CP_UTF8, 0, scBuff, -1, szBuf, sizeof(szBuf));
		OutputDebugString(szBuf);
		OutputDebugString(L"\n");
#else
		Time();
		Color(TRED);
		fputs("E ", stdout);
		if (*source)
		{
			Color(TWHITE);
			fputs(source, stdout);
			putchar(':');
			putchar(' ');
			Color(TRED);
		}

		vprintf(format, ap);
		putchar('\n');
		Color(TNORMAL);
#endif
    }
}

void ScreenLog::Debug(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
	DebugVA(source, format, ap);
    va_end(ap);
}

void ScreenLog::DebugVA(const char * source, const char * format, va_list ap)
{
    if(m_log_level < 3)
        return;
    
    //lock
    std::lock_guard<std::mutex> rGuard(m_lock);
    
    //check is logging is to file
    if(m_pFileLog && m_pFileLog->IsOpen())
    {
        //write to disk
        m_pFileLog->write(source, "D", format, ap);
    }
    else
    {
#if defined(ANDROID)
		__android_log_vprint(ANDROID_LOG_DEBUG, source, format, ap);
#elif defined(WP8)
		char scBuff[16384] = { 0 };
		wchar_t szBuf[16384] = { 0 };

		Time();
		OutputDebugString(L"D ");
		if (*source)
		{
			//convert to wchar
			wchar_t sWBuff[512] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, source, -1, sWBuff, sizeof(sWBuff));
			OutputDebugString(sWBuff);
			OutputDebugString(L": ");
		}

		vsnprintf(scBuff, sizeof(scBuff), format, ap);
		//convert to wchar
		MultiByteToWideChar(CP_UTF8, 0, scBuff, -1, szBuf, sizeof(szBuf));
		OutputDebugString(szBuf);
		OutputDebugString(L"\n");
#else
		Time();
		Color(TBLUE);
		fputs("D ", stdout);
		if (*source)
		{
			Color(TWHITE);
			fputs(source, stdout);
			putchar(':');
			putchar(' ');
			Color(TBLUE);
		}

		vprintf(format, ap);
		putchar('\n');
		Color(TNORMAL);
#endif
    }
}

FileLog::FileLog(const std::string &sFilePath) : m_hFile(INVALID_HANDLE_VALUE), m_filename(sFilePath)
{
    CommonFunctions::CheckFileExists(m_filename.c_str(), true);
    m_hFile = IO::fopen(m_filename.c_str(), IO::IO_RDWR, IO::IO_NORMAL);
	if(m_hFile == INVALID_HANDLE_VALUE)
    {
        Log.Error(__FUNCTION__, "Cannot create log file on path: %s. Log to file is disabled.", m_filename.c_str());
    }
}

FileLog::~FileLog()
{
    IO::fclose(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
}

void FileLog::write(const char *source, const char *level, const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
    write(source, level, format, ap);
	va_end(ap);
}

void FileLog::write(const char *source, const char *level, const char *format, va_list ap)
{
    time_t t;
    size_t l;
    tm aTm;
    char out[4096];
    
    //[2014-06-21 11:48:17] N main: Starting server in: RELEASE mode. SVN version: 102M.
    //[2014-06-21 11:48:17] [level] [source]: [message]
    t = time(NULL);
    localtime(&t, &aTm);
    
    //add date time, level and source
    l = snprintf(out, sizeof(out), "[%-4d-%02d-%02d %02d:%02d:%02d] %s %s: ", aTm.tm_year+1900, aTm.tm_mon+1, aTm.tm_mday, aTm.tm_hour, aTm.tm_min, aTm.tm_sec, level, source);
    //add message
    l += vsnprintf(&out[l], sizeof(out) - l, format, ap);
    //add new line
    l += snprintf(&out[l], sizeof(out) - l, "\n");
    
    //write to end
    IO::fseek(m_hFile, 0, IO::IO_SEEK_END);
    IO::fwrite(&out, l, m_hFile);
}

void FileLog::getLogFileContent(bbuff *pContent)
{
    //get file size
    IO::fseek(m_hFile, 0, IO::IO_SEEK_END);
    size_t fileSize = (size_t)IO::ftell(m_hFile);
    IO::fseek(m_hFile, 0, IO::IO_SEEK_SET);
    
    //prealloc
    bbuff_reserve(pContent, pContent->wpos + fileSize);
    
    //append data
    uint8 buff[4096];
    size_t bytesRead;
    while((bytesRead = IO::fread(buff, sizeof(buff), m_hFile)) > 0)
    {
        bbuff_append(pContent, buff, bytesRead);
    }
}

//C interface
void Log_Notice(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log.NoticeVA(source, format, ap);
    va_end(ap);
}

void Log_Warning(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log.WarningVA(source, format, ap);
    va_end(ap);
}

void Log_Success(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log.SuccessVA(source, format, ap);
    va_end(ap);
}

void Log_Error(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log.ErrorVA(source, format, ap);
    va_end(ap);
}

void Log_Debug(const char * source, const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    Log.DebugVA(source, format, ap);
    va_end(ap);
}





