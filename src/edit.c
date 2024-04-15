/* -------------------------------------------------------------

edit.c
    Essential Notepad - A basic Notepad implementation for Windows
    Code for working with the edit control.

by: Matthew Justice

---------------------------------------------------------------*/
#include <windows.h>
#include <stdbool.h>
#include "esnpad.h"

extern HWND g_hwndEdit;
extern HINSTANCE g_hinst;

//
// CreateEditControl
// Creates the edit control window with the specified word wrap setting.
// Also destroys the existing edit control, if there is one.
//
BOOL CreateEditControl(HWND hwndParent, BOOL wordWrap)
{
    LPWSTR textBuffer = NULL;

    // These width & height values are defaults, only used when these isn't an existing edit control.
    // And even when they are used, the actual width and height will be set by a WM_SIZE message.
    int editWidth = 100;
    int editHeight = 100;

    // If there's an existing edit control, save its text, get its size, and then destroy it.
    if(g_hwndEdit)
    {
        // Get the length of the text in the edit control
        int textLength = GetWindowTextLength(g_hwndEdit);

        // Allocate a buffer to hold the text
        textBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (textLength + 1) * sizeof(WCHAR));
        if(textBuffer)
        {
            // Copy the text to the buffer
            GetWindowText(g_hwndEdit, textBuffer, textLength + 1);
        }

        // Get the size of the existing edit control
        RECT rect;
        GetWindowRect(g_hwndEdit, &rect);
        editWidth = rect.right - rect.left;
        editHeight = rect.bottom - rect.top;

        // Destroy the existing edit control
        DestroyWindow(g_hwndEdit);
        g_hwndEdit = NULL;
    }

    // Set the style of the edit control based on the word wrap setting
    DWORD style = WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL;
    if(wordWrap)
    {
        style &= ~ES_AUTOHSCROLL;
    }

    // Create the edit control
    g_hwndEdit = CreateWindowEx(0, L"Edit", textBuffer,
        style, 0, 0, editWidth, editHeight, hwndParent, (HMENU)IDC_EDIT, g_hinst, NULL);

    // If we allocated a text buffer, free it now
    if(textBuffer)
    {
        HeapFree(GetProcessHeap(), 0, textBuffer);
    }

    // Return TRUE if the edit control was created successfully
    return (g_hwndEdit != NULL);
}
