/* -------------------------------------------------------------

find.c
    Essential Notepad - A basic Notepad implementation for Windows
    Code for working with the find dialog and searching text.

by: Matthew Justice

---------------------------------------------------------------*/
#include <windows.h>
#include <stdbool.h>
#include "esnpad.h"

extern HWND g_hwndMain;
extern HWND g_hwndEdit;
extern HWND g_hwndFind;
extern HINSTANCE g_hinst;

//
// FindTextInEditControl
// Searches for the specified text in the edit control,
// with the specified options, and selects it if found.
//
void FindTextInEditControl(LPWSTR searchText, BOOL matchCase, BOOL searchDown)
{
    DebugLog(L"FindTextInEditControl: Searching for '%s', matchCase=%d, searchDown=%d\n",
        searchText, matchCase, searchDown);

    if (!g_hwndEdit)
    {
        DebugLog(L"No edit control available for searching.\n");
        return;
    }

    // Get the length of the text in the edit control
    int textLength = GetWindowTextLength(g_hwndEdit);

    // Allocate a buffer to hold the text
    LPWSTR textBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (textLength + 1) * sizeof(WCHAR));
    if(textBuffer)
    {
        // Copy the text to the buffer
        GetWindowText(g_hwndEdit, textBuffer, textLength + 1);
    }

    // Find the current selection
    DWORD startPos = 0;
    DWORD endPos = 0;
    SendMessage(g_hwndEdit, EM_GETSEL, (WPARAM)&startPos, (LPARAM)&endPos);
    DebugLog(L"Current selection: start=%d, end=%d\n", startPos, endPos);

    // Determine the search start position
    DWORD searchStart = searchDown ? endPos : (startPos > 0 ? startPos - 1 : 0);
    DebugLog(L"Search starting at position: %d\n", searchStart);

    // Find the text
    DWORD foundPos = (DWORD)-1;
    if (searchDown)
    {
        // Search downwards
        for (DWORD i = searchStart; i <= textLength - wcslen(searchText); i++)
        {
            if ((matchCase && wcsncmp(&textBuffer[i], searchText, wcslen(searchText)) == 0) ||
                (!matchCase && _wcsnicmp(&textBuffer[i], searchText, wcslen(searchText)) == 0))
            {
                foundPos = i;
                break;
            }
        }
    }
    else
    {
        // Search upwards
        for (DWORD i = searchStart; i != (DWORD)-1; i--)
        {
            if ((matchCase && wcsncmp(&textBuffer[i], searchText, wcslen(searchText)) == 0) ||
                (!matchCase && _wcsnicmp(&textBuffer[i], searchText, wcslen(searchText)) == 0))
            {
                foundPos = i;
                break;
            }
        }
    }

    // If found, select the text in the edit control
    if (foundPos != (DWORD)-1)
    {
        DebugLog(L"Text found at position: %d\n", foundPos);
        // Select the found text in the edit control
        SendMessage(g_hwndEdit, EM_SETSEL, foundPos, foundPos + (DWORD)wcslen(searchText));
        // Scroll to the selection
        SendMessage(g_hwndEdit, EM_SCROLLCARET, 0, 0);
    }
    else
    {
        DebugLog(L"Text not found.\n");
        MessageBox(g_hwndMain, L"The text was not found.", APP_TITLE_W, MB_OK | MB_ICONINFORMATION);
    }

    // Free the text buffer
    if(textBuffer)
    {
        HeapFree(GetProcessHeap(), 0, textBuffer);
    }
}

//
// FindDlgProc
// Dialog procedure for the Find dialog
//
INT_PTR CALLBACK FindDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(lparam);

    switch(msg)
    {
    case WM_INITDIALOG:
        CheckRadioButton(hdlg, IDC_DIRECTION_UP, IDC_DIRECTION_DOWN, IDC_DIRECTION_DOWN);
        return TRUE;
    case WM_COMMAND:
        switch(LOWORD(wparam))
        {
        case IDC_FIND_NEXT:
            {
                // Get the state of the dialog options
                BOOL matchCase = (IsDlgButtonChecked(hdlg, IDC_MATCH_CASE) == BST_CHECKED);
                BOOL searchDown = (IsDlgButtonChecked(hdlg, IDC_DIRECTION_DOWN) == BST_CHECKED);

                // Get the search text
                WCHAR searchText[CCH_FIND_TEXT] = {0};
                GetDlgItemText(hdlg, IDC_FIND_TEXT, searchText, CCH_FIND_TEXT);

                // If there's search text, perform the search
                if(wcslen(searchText) > 0)
                {
                    FindTextInEditControl(searchText, matchCase, searchDown);
                }
            }
            return TRUE;
        case IDCANCEL:
            DestroyWindow(hdlg);
            g_hwndFind = NULL;
            return TRUE;
        }
        break;
    }

    return FALSE;
}

//
// MainWndOnEditFind
// Handles IDM_EDIT_FIND by showing the Find dialog.
//
void MainWndOnEditFind(void)
{
    if(g_hwndFind == NULL)
    {
        g_hwndFind = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_FIND), g_hwndMain, FindDlgProc);
        ShowWindow(g_hwndFind, SW_SHOW);
    }
    else
    {
        SetFocus(g_hwndFind);
    }
}
