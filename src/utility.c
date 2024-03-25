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
void DebugLog(const char * format, ...)
{
    va_list vaArgs;
    static char buffer[1024];

    va_start(vaArgs, format);
    if(vsprintf(buffer, format, vaArgs) > 0)
    {
        OutputDebugStringA(buffer);
    }
    va_end(vaArgs);
}
#else /* DEBUG */
//
// Do not log in release builds
//
void DebugLog(const char * format, ...) {}
#endif /* DEBUG */
