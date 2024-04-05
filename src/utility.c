/* -------------------------------------------------------------

file.c
    Essential Notepad - A basic Notepad implementation for Windows
    Utility code

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h> 
#include <stdarg.h>

//
// DebugLog
// A printf style logging function for debug builds
//
#ifdef DEBUG
#define DEBUG_LOG_BUFFER_SIZE 1024
void DebugLog(const WCHAR * format, ...)
{
    va_list vaArgs;
    static WCHAR buffer[DEBUG_LOG_BUFFER_SIZE];

    va_start(vaArgs, format);
    if(vswprintf(buffer, DEBUG_LOG_BUFFER_SIZE, format, vaArgs) > 0)
    {
        OutputDebugStringW(buffer);
    }
    va_end(vaArgs);
}
#else /* DEBUG */
//
// Do not log in release builds
//
void DebugLog(const WCHAR * format, ...) {}
#endif /* DEBUG */
