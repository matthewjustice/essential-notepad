/* -------------------------------------------------------------

file.c
    Essential Notepad - A basic Notepad implementation for Windows
    Code for working with files

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <strsafe.h>
#include "esnpad.h"

extern HWND g_hwndMain;
extern HWND g_hwndEdit;

//
// SetEditTextFromFile
// Read the text from the specified file path and
// populate the edit control with that text.
//
void SetEditTextFromFile(LPTSTR filePath)
{
    HANDLE hFile;
    BYTE buffer[CB_BUFFER];
    DWORD bytesRead;
    WCHAR text[CCH_TEXT];

    // Select all current text in the edit control
    SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);

    // Open the file
    hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // Read the bytes of the file chunks matching our buffer size, and
    // add that text to the edit control as we go.
    do
    {
        if(!ReadFile(hFile, buffer, CB_BUFFER-1, &bytesRead, NULL))
        {
            CloseHandle(hFile);
            return;
        }

        // Add a null terminator after the last byte read
        buffer[bytesRead] = 0;

        // Convert from UTF-8 to WCHAR. This also handles ANSI, and UTF-8 with BOM too.
        if(MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, buffer, -1, text, CCH_TEXT) != 0)
        {
            // Replace the selected text in the edit control, or append if none selected 
            SendMessageW(g_hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
        }

    } while (bytesRead);

    return;
}

//
// MainWndOnOpenFile
// Handles IDM_FILE_OPEN by prompting the user
// for a file to open, then populating the edit
// control with the contents of that file.
//
void MainWndOnOpenFile(void)
{
    OPENFILENAME ofn;
    TCHAR filePath[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(&filePath, sizeof(filePath));

    // Bring up the File Open Dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("txt");

    // pairs of null-terminated filter strings
    ofn.lpstrFilter = TEXT("Text files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0");
    ofn.nFilterIndex = 0;

    // Pointer to a buffer that contains an initial file name 
    // If lpstrFile contains a path, that path is the initial directory
    // If first byte is NULL, then no initial path
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;

    // file name and extension of the selected file
    // Can be null since we don't need to get the name only
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    // If the user specifies a file name and clicks the OK button,
    // the return value is nonzero (TRUE). 
    if (GetOpenFileName(&ofn))
    {
        SetEditTextFromFile(ofn.lpstrFile);
    }

    return;
}
