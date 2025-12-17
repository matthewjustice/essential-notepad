/* -------------------------------------------------------------

esnpad.h
   Essential Notepad - A basic Notepad implementation for Windows
   Shared header file for esnpad project

by: Matthew Justice

---------------------------------------------------------------*/
#ifndef _ESNPAD_H_
#define _ESNPAD_H_

#include <windows.h>

// General Constants
#define IDC_EDIT           100
#define IDC_STATUS         101
#define CB_BUFFER          512
#define CCH_FIND_TEXT      256

#define APP_TITLE_A        "Essential Notepad"
#define APP_TITLE_W        L"Essential Notepad"

// Resource constants
#define IDR_MENUMAIN          200
#define IDR_ACCELMAIN         201
#define IDM_FILE_NEW          300
#define IDM_FILE_OPEN         301
#define IDM_FILE_SAVE         302
#define IDM_FILE_SAVE_AS      303
#define IDM_FILE_EXIT         304
#define IDM_VIEW_WORDWRAP     305
#define IDM_VIEW_DARKMODE     306
#define IDM_EDIT_UNDO         307
#define IDM_EDIT_CUT          308
#define IDM_EDIT_COPY         309
#define IDM_EDIT_PASTE        310
#define IDM_EDIT_DELETE       311
#define IDM_EDIT_SELECT_ALL   312
#define IDM_EDIT_FIND         313

// Dialog constants
#define IDC_STATIC            -1
#define IDD_FIND              400
#define IDC_FIND_TEXT         401
#define IDC_FIND_NEXT         402
#define IDC_MATCH_CASE        403
#define IDC_DIRECTION_UP      404
#define IDC_DIRECTION_DOWN    405

// File related constants
#define ENCODING_UNSPECIFIED -1
#define ENCODING_ANSI         0
#define ENCODING_UTF_8        1
#define ENCODING_UTF_8_BOM    2
#define ENCODING_UTF_16_LE    3
#define ENCODING_UTF_16_BE    4

#define UTF16_BOM_BYTES       2
#define UTF8_BOM_BYTES        3

// The max size of the window title, in bytes.
// This needs to accomodate a file name (which will be < MAX_PATH)
// + the name of the app (18 wchars) + a separator (3 wchars)
// + an indicator to show if the file is currently being edited.
// I'm rounding all of that up to 32.
// Multiply all by 2 to account for wide chars.
#define CB_WINDOW_TITLE       ((MAX_PATH + 32) * 2)

// The max size of a prompt message, in bytes.
// Large enough to hold the prompt message and a file name.
#define CB_PROMPT_MESSAGE     512

// Colors for dark mode
#define DARK_MODE_TEXT_COLOR        RGB(0xCC, 0xCC, 0xCC)
#define DARK_MODE_BACKGROUND_COLOR  RGB(0x1F, 0x1F, 0x1F)

// Colors for light mode
#define LIGHT_MODE_TEXT_COLOR        RGB(0x00, 0x00, 0x00)
#define LIGHT_MODE_BACKGROUND_COLOR  RGB(0xFF, 0xFF, 0xFF)

// Function prototypes - main
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitApp();
BOOL InitWindow(int);
int MsgLoop(void);
void UpdateTitleDirtyIndicator(void);

// Function prototypes - file.c
void SetEditTextFromFile(LPWSTR filePath);
BOOL ConvertBytesToString(BYTE * data, size_t dataSize, WCHAR * wideText, size_t wideTextSize);
void MainWndOnFileOpen(void);
void MainWndOnFileSaveAs(void);
void MainWndOnFileSave(void);

// Function prototypes - edit.c
BOOL CreateEditControl(HWND hwndParent, BOOL wordWrap);
BOOL SetEditText(BYTE * data, size_t dataSize);
LRESULT MainWndOnControlColorEdit(HDC hdc);

// Function prototypes - find.c
void MainWndOnEditFind(void);

// Function prototypes - utility.c
void DebugLog(const WCHAR * format, ...);

#endif // _ESNPAD_H_
