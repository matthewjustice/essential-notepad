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
// ReadAllFileBytes
// Read all the bytes of a file specified by hFile.
// The bytes are read into buffer dst of size dstSize.
//
BOOL ReadAllFileBytes(HANDLE hFile, BYTE * dst, size_t dstSize)
{
    BYTE readBuffer[CB_BUFFER];
    DWORD bytesRead;
    DWORD byteOffset = 0;

    // Read the bytes of the file chunks matching our buffer size, and
    // append that text to the dst buffer as we go.
    do
    {
        if(!ReadFile(hFile, readBuffer, CB_BUFFER, &bytesRead, NULL))
        {
            return FALSE;
        }

        // Copy the bytes into the text buffer.
        if(memcpy_s(dst+byteOffset, dstSize, readBuffer, bytesRead) != 0)
        {
            return FALSE;
        }

        // Update our offset into the dst buffer
        byteOffset += bytesRead;
    } while (bytesRead);

    return TRUE;
}

//
// ConvertBytesToString
// Given a data buffer of bytes that contains string data
// that may be ANSI, UTF-8, or UTF-16, convert the data
// to a wide character string.
//
BOOL ConvertBytesToString(BYTE * data, size_t dataSize, WCHAR * wideText, size_t wideTextSize)
{
    BOOL success = FALSE;

    // Detect the encoding
    if(dataSize >= 2 && data[0] == 0xFF && data[1] == 0xFE)
    {
        DebugLog("UTF-16 LE detected");
        // The BOM says this is UTF-16 LE (or UTF-32, but ignore that)
        // So just treat the data as wide text and copy it into the output buffer.
        // Start copying from +2 to skip over the BOM bytes.
        if(SUCCEEDED(StringCchCopyW(wideText, wideTextSize / sizeof(WCHAR), (LPWSTR)(data+2))))
        {
            success = TRUE;
        }
    }
    else if(dataSize >= 2 && data[0] == 0xFE && data[1] == 0xFF)
    {
        DebugLog("UTF-16 BE is not supported");
        success = FALSE;
    }
    else
    {
        DebugLog("Treating data as UTF-8 or ANSI");

        // Check for UTF-8 BOM
        if(dataSize >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        {
            DebugLog("\nUTF-8 with BOM detected");
            // Move the data pointer past the BOM
            data += 3;
        }

        // MultiByteToWideChar handles UTF-8 and ANSI
        if(MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, data, -1, wideText, wideTextSize / sizeof(WCHAR)) != 0)
        {
            success = TRUE;
        }
    }

    return success;
}

//
// Set the text in g_hwndEdit to the characters specified in data.
//
void SetEditText(BYTE * data, size_t dataSize)
{
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
        if(!ConvertBytesToString(data, dataSize, wideText, wideTextSize))
        {
            // If the conversion failed, set wideText to an empty string
            wideText[0] = 0;
        }

        // Select all current text in the edit control
        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);

        // Replace the selected text in the edit control, or append if none selected 
        SendMessageW(g_hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)wideText);

        HeapFree(GetProcessHeap(), 0, wideText);
    }

    return;
}

//
// SetEditTextFromFile
// Read the text from the specified file path and
// populate the edit control with that text.
//
void SetEditTextFromFile(LPTSTR filePath)
{
    HANDLE hFile;
    BYTE * fileBytes;
    size_t fileBytesSize;
    LARGE_INTEGER fileSize;

    // Open the file
    hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // Get the file size
    if(GetFileSizeEx(hFile, &fileSize))
    {
        // Allocate a buffer that is large enough to hold the entire contents
        // of the file, plus two bytes for a null terminator (which will already
        // be present due to allocating HEAP_ZERO_MEMORY). Only one byte is needed
        // for the terminator if ANSI/UTF-8, but 2 is needed for UTF-16.
        fileBytesSize = (size_t)fileSize.QuadPart + 2;
        fileBytes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileBytesSize);
        if(fileBytes)
        {
            // Read all the bytes of the file into fileBytes
            if(ReadAllFileBytes(hFile, fileBytes, fileBytesSize))
            {
                SetEditText(fileBytes, fileBytesSize);
            }

            HeapFree(GetProcessHeap(), 0, fileBytes);
        }
    }

    CloseHandle(hFile);
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