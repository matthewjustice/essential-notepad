/* -------------------------------------------------------------

edit.c
    Essential Notepad - A basic Notepad implementation for Windows
    Code for working with the edit control.

by: Matthew Justice

---------------------------------------------------------------*/
#include <windows.h>
#include <stdbool.h>
#include "esnpad.h"

extern HWND g_hwndMain;
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
    DWORD style = WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_NOHIDESEL;
    if(wordWrap)
    {
        style &= ~ES_AUTOHSCROLL;
    }

    // Create the edit control
    g_hwndEdit = CreateWindowEx(0, L"Edit", textBuffer,
        style, 0, 0, editWidth, editHeight, hwndParent, (HMENU)IDC_EDIT, g_hinst, NULL);

    if(g_hwndEdit)
    {
        // Set the focus to the edit control
        SetFocus(g_hwndEdit);

        // Set the font for the edit control based on the current DPI
        UINT dpi = GetDpiForWindow(hwndParent);
        HFONT hFont = CreateScaledFont(dpi);
        SendMessage(g_hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    // If we allocated a text buffer, free it now
    if(textBuffer)
    {
        HeapFree(GetProcessHeap(), 0, textBuffer);
    }

    // Return TRUE if the edit control was created successfully
    return (g_hwndEdit != NULL);
}

//
// Set the text in g_hwndEdit to the characters specified in data.
//
BOOL SetEditText(BYTE * data, size_t dataSize)
{
    BOOL success = FALSE;
    WCHAR * wideText;
    size_t wideTextSize;

    // Allocate a buffer for holding our text as wide characters.
    // It needs to be 2 times the size of the file, since worst-case
    // each UTF-8 byte expands to a wide char.
    wideTextSize = dataSize * 2;
    wideText = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wideTextSize);
    if(wideText)
    {
        // Convert the data to a wide string.
        if(ConvertBytesToString(data, dataSize, wideText, wideTextSize))
        {
            // Treat the function as successful if we're able to convert the
            // bytes to a string.
            success = TRUE;
        }
        else 
        {
            // If the conversion failed, set wideText to an empty string
            wideText[0] = 0;
        }

        // Select all current text in the edit control
        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);

        // Replace the selected text in the edit control, or append if none selected 
        SendMessageW(g_hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)wideText);

        // Scroll the edit control to the top
        SendMessage(g_hwndEdit, EM_SETSEL, 0, 0);
        SendMessage(g_hwndEdit, EM_SCROLLCARET, 0, 0);

        HeapFree(GetProcessHeap(), 0, wideText);
    }

    return success;
}

//
// MainWndOnControlColorEdit
// Handles the WM_CTLCOLOREDIT message for the edit control.
//
LRESULT MainWndOnControlColorEdit(HDC hdc)
{
    COLORREF textColor;
    COLORREF backgroundColor;

    HMENU hMenu = GetMenu(g_hwndMain);
    UINT darkModeMenuState = GetMenuState(hMenu, IDM_VIEW_DARKMODE, MF_BYCOMMAND);

    textColor = (darkModeMenuState & MF_CHECKED) ? DARK_MODE_TEXT_COLOR : LIGHT_MODE_TEXT_COLOR;
    backgroundColor = (darkModeMenuState & MF_CHECKED) ? DARK_MODE_BACKGROUND_COLOR : LIGHT_MODE_BACKGROUND_COLOR;

    // Set the text color
    SetTextColor(hdc, textColor);

    // Set the background color
    SetBkColor(hdc, backgroundColor); // Dark color

    // Return a brush with the new background color
    return (LRESULT) CreateSolidBrush(backgroundColor);
}
