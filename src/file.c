/* -------------------------------------------------------------

file.c
    Essential Notepad - A basic Notepad implementation for Windows
    Code for working with files

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "esnpad.h"

//
// globals
//
WCHAR g_activeFile[MAX_PATH]= {0};
int g_fileEncoding = ENCODING_UNSPECIFIED;

extern HWND g_hwndMain;
extern HWND g_hwndEdit;
extern BOOL g_dirtyText;

//
// SetActiveFile
// Sets the active file in global memory, and
// updates the window title to include the file name.
//
BOOL SetActiveFile(LPWSTR filePath)
{
    BOOL success = FALSE;
    LPWSTR fileName;
    LPWSTR windowTitle;

    // Copy the specified filePath into the active file global
    if(SUCCEEDED(StringCchCopyW(g_activeFile, MAX_PATH, filePath)))
    {
        // Get a pointer to the the file name in the path.
        fileName = PathFindFileNameW(filePath);

        // Allocate a buffer to hold the new window title.
        windowTitle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CB_WINDOW_TITLE);
        if(windowTitle)
        {
            // Generate a window title that contains the file name and app name
            if(SUCCEEDED(StringCchPrintf(windowTitle, CB_WINDOW_TITLE, L"%s - %s", fileName, APP_TITLE_W)))
            {
                // Set the window title
                if(SetWindowText(g_hwndMain, windowTitle))
                {
                    success = TRUE;
                }
            }

            HeapFree(GetProcessHeap(), 0, windowTitle);
        }
    }

    if(!success)
    {
        // We weren't able to set the active file or the window title.
        // Set the window title to only the app name so we don't
        // accidentally show the wrong file name in the title.
        SetWindowText(g_hwndMain, APP_TITLE_W);
    }

    return success;
}

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
        DebugLog(L"UTF-16 LE detected");
        // The BOM says this is UTF-16 LE (or UTF-32, but ignore that)
        // So just treat the data as wide text and copy it into the output buffer.
        // Start copying from +2 to skip over the BOM bytes.
        if(SUCCEEDED(StringCchCopyW(wideText, wideTextSize / sizeof(WCHAR), (LPWSTR)(data+2))))
        {
            g_fileEncoding = ENCODING_UTF_16_LE;
            success = TRUE;
        }
    }
    else if(dataSize >= 2 && data[0] == 0xFE && data[1] == 0xFF)
    {
        DebugLog(L"UTF-16 BE is not supported");
        g_fileEncoding = ENCODING_UNSPECIFIED;
        success = FALSE;
    }
    else
    {
        DebugLog(L"Treating data as UTF-8 or ANSI");

        // Check for UTF-8 BOM
        if(dataSize >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        {
            DebugLog(L"\nUTF-8 with BOM detected");
            g_fileEncoding = ENCODING_UTF_8_BOM;
            // Move the data pointer past the BOM
            data += 3;
        }
        else
        {
            // Treat both ANSI and UTF-8 as ENCODING_UTF_8
            g_fileEncoding = ENCODING_UTF_8;
        }

        // MultiByteToWideChar handles UTF-8 and ANSI
        if(MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCCH)data, -1, wideText, (int)(wideTextSize / sizeof(WCHAR))) != 0)
        {
            success = TRUE;
        }
    }

    return success;
}

//
// SetEditTextFromFile
// Read the text from the specified file path and
// populate the edit control with that text.
//
void SetEditTextFromFile(LPWSTR filePath)
{
    HANDLE hFile;
    BYTE * fileBytes;
    size_t fileBytesSize;
    LARGE_INTEGER fileSize;

    // If this function fails, there should be no active file.
    // Assume failure until the file is opened successfully.
    ZeroMemory(g_activeFile, sizeof(g_activeFile));

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
                if(SetEditText(fileBytes, fileBytesSize))
                {
                    // When the edit text is first set from file, it is clean.
                    g_dirtyText = FALSE;

                    if(SetActiveFile(filePath))
                    {
                        DebugLog(L"Active file is %s", g_activeFile);
                    }
                }
                else
                {
                    DebugLog(L"No active file");
                }
            }

            HeapFree(GetProcessHeap(), 0, fileBytes);
        }
    }

    CloseHandle(hFile);
    return;
}

//
// MainWndOnFileOpen
// Handles IDM_FILE_OPEN by prompting the user
// for a file to open, then populating the edit
// control with the contents of that file.
//
void MainWndOnFileOpen(void)
{
    OPENFILENAME ofn;
    WCHAR filePath[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(&filePath, sizeof(filePath));

    // Bring up the File Open Dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"txt";

    // pairs of null-terminated filter strings
    ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
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

//
// WriteBytesToActiveFile
// Write the specified bytes to g_activeFile
//
BOOL WriteBytesToActiveFile(LPCVOID bytes, DWORD byteCount)
{
    BOOL success = FALSE;
    HANDLE hFile;
    DWORD bytesWritten = 0;

    hFile = CreateFile(g_activeFile, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        // Early exit
        DebugLog(L"Couldn't open file for writing: %s", g_activeFile);
        return FALSE;
    }

    success = WriteFile(hFile, bytes, byteCount, &bytesWritten, NULL);

    CloseHandle(hFile);

    return success;
}

//
// ConvertWideTextToUTF16
// Convert wide text string to UTF-16 LE with BOM.
//
// widetext is the null-terminated wide text string to convert.
// wideTextSize is the size if the wideText in bytes
// writeBytesCount is an output param, the number of bytes that should be written to a file.
//
// The function returns a pointer to a UTF-16 encoded array of bytes that can written a file.
// This returned array should be freed by the caller with HeapFree().
//
BYTE * ConvertWideTextToUTF16(LPWSTR wideText, size_t wideTextSize, size_t * writeBytesCount)
{
    // Allocate a buffer for the UTF-16 LE text.
    size_t fileBytesSize = wideTextSize + UTF16_BOM_BYTES;
    BYTE * fileBytes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileBytesSize);
    if(fileBytes)
    {
        // Set our UTF-16 LE BOM
        fileBytes[0] = 0xFF;
        fileBytes[1] = 0xFE;

        // Copy in the wide text, as-is, after the BOM
        if(SUCCEEDED(StringCchCopy((LPWSTR)(fileBytes+UTF16_BOM_BYTES), fileBytesSize-UTF16_BOM_BYTES, wideText)))
        {
            // Tell the caller to write 2 byte less to file since we don't need the (wide) null terminator.
            *writeBytesCount = fileBytesSize - 2;
        }
    }

    return fileBytes;
}

//
// ConvertWideTextToUTF8
// Convert wide text string to UTF-8
//
// widetext is the null-terminated wide text string to convert.
// extraBytes is the number of bytes to reserve at the beginning of the buffer (for a BOM)
// writeBytesCount is an output param, the number of bytes that should be written to a file.
//
// The function returns a pointer to a UTF-8 encoded array of bytes that can written a file.
// TThis returned array should be freed by the caller with HeapFree().
//
BYTE * ConvertWideTextToUTF8(LPWSTR wideText, int extraBytes, size_t * writeBytesCount)
{
    // The pointer and count for the data we'll write to the file
    BYTE * fileBytes = NULL;
    size_t fileBytesSize = 0;

    // Allocate a buffer for the UTF-8 text.
    // First, get the required size to hold UTF-8 text. Note that UTF-8 can actually
    // be larger than wide characters, although this is uncommon.
    int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, wideText, -1, NULL, 0, NULL, NULL);

    // Make sure we got a valid byte count.
    if(utf8ByteCount == 0)
    {
        DebugLog(L"Unable to determine UTF-8 byte count.");
        *writeBytesCount = 0;
        return NULL;
    }

    // Allocate a buffer to hold the file-ready UTF-8 data.
    fileBytesSize = utf8ByteCount + extraBytes;
    fileBytes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileBytesSize);
    if(fileBytes)
    {
        // Convert the string.
        if(WideCharToMultiByte(CP_UTF8, 0, wideText, -1, (LPSTR)(fileBytes+extraBytes), (int)(fileBytesSize-extraBytes), NULL, NULL) != 0)
        {
            // Tell the caller to write 1 byte less to file since we don't need the null terminator.
            *writeBytesCount = fileBytesSize - 1;
        }
    }

    return fileBytes;
}

//
// SaveEditTextToActiveFile
// Writes the text in the edit control to
// the file specified in g_activeFile.
//
void SaveEditTextToActiveFile()
{
    // The text we will read from the edit control
    int textLength = 0;
    LPWSTR wideText = NULL;
    size_t wideTextSize = 0;

    // The pointer and count that we will pass to WriteBytesToActiveFile
    BYTE * writeBytesPtr = NULL;
    size_t writeBytesCount = 0;

    DebugLog(L"Saving text to %s with encoding %d", g_activeFile, g_fileEncoding);

    // Get the text from the edit control.
    // Start by determining how much memory we need to hold it.
    textLength = GetWindowTextLength(g_hwndEdit);

    // Allocate a buffer that has one extra character for the null terminator
    wideTextSize = (textLength + 1) * sizeof(WCHAR);
    wideText = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, wideTextSize);
    if(wideText)
    {
        if(GetWindowText(g_hwndEdit, wideText, (int)(wideTextSize / sizeof(WCHAR))) != 0)
        {
            // We have our wide character text. Now encode it.
            switch (g_fileEncoding)
            {
            case ENCODING_UTF_16_LE:
                writeBytesPtr = ConvertWideTextToUTF16(wideText, wideTextSize, &writeBytesCount);
                break;
            case ENCODING_UTF_8_BOM:
                // Save as UTF-8 with a BOM
                writeBytesPtr = ConvertWideTextToUTF8(wideText, UTF8_BOM_BYTES, &writeBytesCount);
                if(writeBytesPtr)
                {
                    // Set our UTF-8 BOM
                    writeBytesPtr[0] = 0xEF;
                    writeBytesPtr[1] = 0xBB;
                    writeBytesPtr[2] = 0xBF;
                }
                break;
            default:
                // Save as UTF-8
                writeBytesPtr = ConvertWideTextToUTF8(wideText, 0, &writeBytesCount);
                break;
            }

            // Write the bytes to the active file
            if(writeBytesPtr && writeBytesCount > 0)
            {
                // TODO: Handle failure
                if(WriteBytesToActiveFile(writeBytesPtr, (DWORD)writeBytesCount))
                {
                    // When the edit text is initially written to file, it is clean.
                    g_dirtyText = FALSE;
                }
            }

            // Free our write bytes buffer
            if(writeBytesPtr)
            {
                HeapFree(GetProcessHeap(), 0, writeBytesPtr);
            }
        }

        // Free our wide text buffer
        HeapFree(GetProcessHeap(), 0, wideText);
    }

    return;
}

//
// MainWndOnFileSaveAs
// Handles IDM_FILE_SAVE_AS by prompting the user
// for a file to save, then writing the contents of 
// the edit control to that file.
//
void MainWndOnFileSaveAs(void)
{
    OPENFILENAME ofn;
    WCHAR filePath[MAX_PATH];

    ZeroMemory(&ofn, sizeof(ofn));

    // If there's an active file, use it as the default save as file path.
    if(g_activeFile[0] != 0)
    {
        if(FAILED(StringCchCopyW(filePath, MAX_PATH, g_activeFile)))
        {
            // Failed to copy the active file, so zero out the filePath buffer.
            ZeroMemory(&filePath, sizeof(filePath));
        }
    }
    else
    {
        // No active file, just zero out the filePath buffer.
        ZeroMemory(&filePath, sizeof(filePath));
    }

    // Bring up the File Save Dialog
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.Flags = OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"txt";

    // pairs of null-terminated filter strings
    // These should align with the ENCODING_ constants,
    // and the first entry in this filter is assigned index 1.
    ofn.lpstrFilter = L"UTF-8 text file (*.txt)\0*.txt\0UTF-8 (with BOM) text file (*.txt)\0*.txt\0UTF-16 LE text file (*.txt)\0*.txt\0";
    
    // Set the default encoding to the one in use, if possible.
    if(g_fileEncoding == ENCODING_UTF_8 || g_fileEncoding == ENCODING_UTF_8_BOM || g_fileEncoding == ENCODING_UTF_16_LE)
    {
        ofn.nFilterIndex = g_fileEncoding;
    }
    else
    {
        ofn.nFilterIndex = 0;
    }

    // Pointer to a buffer that contains an initial file name 
    // If lpstrFile contains a path, that path is the initial directory
    // If first byte is NULL, then no initial path.
    // On return, contains the user-selected file path.
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;

    // file name and extension of the selected file
    // Can be null since we don't need to get the name only
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;

    // Dialog box settings
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

    // Pointer to a buffer that contains the default extension
    ofn.lpstrDefExt = L"txt";

    // If the user specifies a file name and clicks the OK button,
    // the return value is nonzero (TRUE). 
    if (GetSaveFileName(&ofn))
    {
        if(SetActiveFile(filePath))
        {
            g_fileEncoding = ofn.nFilterIndex;
            SaveEditTextToActiveFile();
        }
    }

    return;
}

//
// MainWndOnFileSave
// Handles IDM_FILE_SAVE by prompting the user
// for a file to save if needed, and then writing the contents of 
// the edit control to that file.
//
void MainWndOnFileSave(void)
{
    if(g_activeFile[0] != 0)
    {
        // There's already an active file name. Save there.
        SaveEditTextToActiveFile();

        // Clear the dirty indicator in the window title.
        UpdateTitleDirtyIndicator();
    }
    else
    {
        // There's no active file. Treat this as Save As.
        MainWndOnFileSaveAs();
    }

    return;
}
