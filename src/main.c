/* -------------------------------------------------------------

main.c
    Essential Notepad - A basic Notepad implementation for Windows

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "esnpad.h"

//
// globals
//
HINSTANCE g_hinst;  // main instance handle
HWND g_hwndMain;    // handle to main window
HWND g_hwndEdit = NULL;    // handle to edit control
HWND g_hwndStatus = NULL;  // handle to status control
WCHAR g_nameMainClass[] = L"MainWinClass"; // name of the main window class
LPWSTR g_cmdLineFile = NULL;
BOOL g_dirtyText = FALSE;   // "dirty" means text changes haven't been saved

extern WCHAR g_activeFile[MAX_PATH];


//
// WinMain
// program entry point
//
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd)
{
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);

    int argc = 0;
    LPWSTR * argv = NULL;

    // Get the command line args
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if((argc > 1) && PathFileExistsW(argv[1]))
    {
        // The first arg can be a text file name to open
        g_cmdLineFile = argv[1];
    }

    // Save the value of the main instance handle
    g_hinst = hInstance;

    // Initialize the window data and register the window class
    if (!InitApp())
    {
        MessageBox(NULL, L"Error during app init",
            APP_TITLE_W, MB_ICONERROR);

        return -1;
    }

    // Create the window and display it
    if(!InitWindow(nShowCmd))
    {
        MessageBox(NULL, L"Error during window init",
            APP_TITLE_W, MB_ICONERROR);

        return -1;
    }

    // run the message loop
    return MsgLoop();
}

//
// InitApp
// Initialize the window data and register the window class
//
BOOL InitApp()
{
    WNDCLASSEX wc;
    INITCOMMONCONTROLSEX icc;

    // Ensure that comctl32.dll is loaded, register status bar control class
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    if(!InitCommonControlsEx(&icc))
    {
        return FALSE;
    }

    // Describe the main window with the window class structure
    ZeroMemory(&wc, sizeof(wc));

    // required parameters
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hinst;
    wc.lpszClassName = g_nameMainClass;

    // additional parameters
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENUMAIN);

    // register the class so it can be used to create windows
    return RegisterClassEx(&wc);
}

//
// InitWindow
// Create the window and display it
// Save the window instance in a global
//
BOOL InitWindow(int nCmdShow)
{
    // Get the width and height of the primary monitor
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate the initial window size and position
    int windowHeight = screenHeight * 2 / 3;
    int windowWidth = windowHeight; // square window
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    // create the main window
    g_hwndMain = CreateWindowEx(0, g_nameMainClass, APP_TITLE_W,
        WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight,
        NULL, NULL, g_hinst, 0);

    if (!g_hwndMain)
    {
        return FALSE;
    }

    // make the window visible
    ShowWindow(g_hwndMain, nCmdShow);

    return TRUE;
}

//
// CreateScaledFont
// Helper function to create scaled font based on DPI
//
HFONT CreateScaledFont(UINT dpi)
{
    int scaledHeight = MulDiv(18, dpi, 96); // Base font size of 18
    return CreateFont(scaledHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, NULL);
}

//
// MainWndOnCreate
// Handles WM_CREATE for the main window
//
LRESULT MainWndOnCreate(HWND hwnd)
{
    // Create the edit control window
    if(!CreateEditControl(hwnd, TRUE))
    {
        // -1 indicates WM_CREATE failure
        return -1;
    }

    // Create the status bar control
    g_hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
        WS_VISIBLE|WS_CHILD|SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, (HMENU)IDC_STATUS, g_hinst, NULL);

    if(!g_hwndStatus)
    {
        // -1 indicates WM_CREATE failure
        return -1;
    }

    // Handle font scaling based on initial DPI
    UINT dpi = GetDpiForWindow(hwnd);
    HFONT hFont = CreateScaledFont(dpi);
    SendMessage(g_hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    if(g_cmdLineFile)
    {
        SetEditTextFromFile(g_cmdLineFile);
    }

    return 0;
}

//
// MainWndOnResize
// Handles WM_SIZE for the main window
//
LRESULT MainWndOnResize(int width, int height)
{
    if(g_hwndEdit && g_hwndStatus)
    {
        RECT rectStatus;
        int heightEdit;

        // Set the status bar size
        SendMessage(g_hwndStatus, WM_SIZE, SIZE_RESTORED, 0);

        // Get the status bar window rectangle
        GetWindowRect(g_hwndStatus, &rectStatus);

        // Calculate the height of the edit control
        heightEdit = height - (rectStatus.bottom - rectStatus.top);

        // Resize the edit control
        MoveWindow(g_hwndEdit, 0, 0, width, heightEdit, TRUE);
    }

    return 0;
}

//
// ConfirmSaveChanges
// If the edit text is dirty, ask the user if they want to save
// it before continuing.
// The function returns TRUE if the caller should continue with
// whatever they were planning on doing. A return of FALSE means
// to cancel the caller's intended operation.
//
BOOL ConfirmSaveChanges(void)
{
    if(!g_dirtyText)
    {
        // When the edit text is clean, there's nothing for us to do
        // here, and the caller should continue as usual.
        return TRUE;
    }

    // Allocate a buffer to hold the prompt message
    LPWSTR prompt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CB_PROMPT_MESSAGE);

    if(!prompt)
    {
        // If we can't allocate memory, just return TRUE.
        // The caller should continue as usual.
        return TRUE;
    }

    if(g_activeFile[0] != 0)
    {
        // There's an active file, so include its name in the prompt.
        // Get the filename part of the path
        LPWSTR fileName = PathFindFileNameW(g_activeFile);

        // Format the prompt message
        StringCchPrintf(prompt, CB_PROMPT_MESSAGE, L"Do you want to save your changes to %s?", fileName);
    }
    else
    {
        // There's no active file, so the prompt is generic.
        StringCchCopy(prompt, CB_PROMPT_MESSAGE, L"Do you want to save your changes?");
    }

    // Prompt the user for what they want to do.
    int result = MessageBox(g_hwndMain, prompt,
        APP_TITLE_W, MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON1|MB_APPLMODAL);

    // Free the memory we allocated for the prompt message
    HeapFree(GetProcessHeap(), 0, prompt);

    if(result == IDYES)
    {
        // The user wants to save
        MainWndOnFileSave();
    }

    return (result != IDCANCEL);
}

//
// UpdateDirtyIndicator
// If the edit control text is dirty, the title
// of the main window should show a leading indicator
// that tells the user this is the case. This function
// makes sure the title text is aligned with g_dirtyText.
//
void UpdateTitleDirtyIndicator(void)
{
    // Allocate buffers to hold the old and new window titles.
    LPWSTR oldWindowTitle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CB_WINDOW_TITLE);
    LPWSTR newWindowTitle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CB_WINDOW_TITLE);
    if(oldWindowTitle && newWindowTitle)
    {
        // Get the current main window title as the "old" window title
        if(GetWindowText(g_hwndMain, oldWindowTitle, CB_WINDOW_TITLE / sizeof(WCHAR)) != 0)
        {
            if(g_dirtyText)
            {
                // Format the new window title as the old one with a leading indicator.
                if(SUCCEEDED(StringCchPrintf(newWindowTitle, CB_WINDOW_TITLE, L"* %s", oldWindowTitle)))
                {
                    // Set the window title.
                    SetWindowText(g_hwndMain, newWindowTitle);
                }
            }
            else
            {
                // The text isn't dirty, so remove the indicator, if it's there.
                if(oldWindowTitle[0] == '*' && oldWindowTitle[1] == ' ')
                {
                    // Set the window title as the old title, but without the leading "* "
                    SetWindowText(g_hwndMain, oldWindowTitle+2);
                }
            }
        }
    }

    // Free memory if needed.
    if(oldWindowTitle)
    {
        HeapFree(GetProcessHeap(), 0, oldWindowTitle);
    }

    if(newWindowTitle)
    {
        HeapFree(GetProcessHeap(), 0, newWindowTitle);
    }

    return;
}

//
// Handles WM_COMMAND for the edit control
//
void EditControlOnCommand(int code)
{
    // When the text of the edit control changes, make it as dirty
    // (meaning it isn't in sync with the active file), and update
    // the main window title to show dirty indicator. We only need
    // to do this if it isn't already marked as dirty.
    if((code == EN_CHANGE) && !g_dirtyText)
    {
        g_dirtyText = TRUE;
        UpdateTitleDirtyIndicator();
    }

    return;
}

//
// MainWndOnFileNew
// Handles IDM_FILE_NEW by clearing the edit control
// and resetting state back to no active file.
//
void MainWndOnFileNew(void)
{
    // Clear the edit control
    SetWindowText(g_hwndEdit, L"");

    // Set the app title back to the default (no filename or dirty indicator)
    SetWindowText(g_hwndMain, APP_TITLE_W);

    // The text isn't dirty, since there is none.
    g_dirtyText = FALSE;

    // There is no active file.
    ZeroMemory(g_activeFile, sizeof(g_activeFile));

    return;
}

//
// MainWndOnViewWordWrap
// Handles IDM_VIEW_WORDWRAP by toggling the check box on the menu,
// and creating a new edit control with the new word wrap setting.
//
void MainWndOnViewWordWrap(void)
{
    UINT wordWrapState = GetMenuState(GetMenu(g_hwndMain), IDM_VIEW_WORDWRAP, MF_BYCOMMAND);

    // Check if the menu item is checked
    BOOL checked = wordWrapState & MF_CHECKED;

    if (checked)
    {
        // Word wrap is on, turn it off in the menu
        CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_WORDWRAP, MF_UNCHECKED);
    }
    else
    {
        // Word wrap is off, turn it on in the menu
        CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_WORDWRAP, MF_CHECKED);
    }

    // Create a new edit control with the new word wrap setting
    // This is necessary because the ES_AUTOHSCROLL style can't be changed.
    // Note that we are passing in !checked, because the new state is the opposite
    // of what it was when the user clicked the menu item.
    CreateEditControl(g_hwndMain, !checked);

    return;
}

//
// MainWndOnViewDarkMode
// Handles IDM_VIEW_DARKMODE by toggling the check box on the menu,
// and updating the edit control colors.
//
void MainWndOnViewDarkMode(void)
{
    HMENU hMenu = GetMenu(g_hwndMain);
    UINT darkModeMenuState = GetMenuState(hMenu, IDM_VIEW_DARKMODE, MF_BYCOMMAND);
    UINT wordWrapState = GetMenuState(hMenu, IDM_VIEW_WORDWRAP, MF_BYCOMMAND);

    if (darkModeMenuState & MF_CHECKED)
    {
        // Dark mode is on, turn it off in the menu
        CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_DARKMODE, MF_UNCHECKED);
    }
    else
    {
        // Dark mode is off, turn it on in the menu
        CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_DARKMODE, MF_CHECKED);
    }

    // Create a new edit control, preserving the current word wrap setting.
    // This is necessary because we need to trigger another WM_CTLCOLOREDIT,
    // which will set the new colors based on the new dark mode menu setting.
    CreateEditControl(g_hwndMain, (wordWrapState & MF_CHECKED));

    return;
}

//
// MainWndOnDpiChanged
// Handles WM_DPICHANGED for the main window
//
LRESULT MainWndOnDpiChanged(int dpiY)
{
    HFONT hFont = CreateScaledFont(dpiY);
    SendMessage(g_hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    return 0;
}

//
// MainWndOnCommand
// Handles WM_COMMAND for the main window
//
LRESULT MainWndOnCommand(HWND hwnd, int id, int code)
{
    switch(id)
    {
    case IDM_FILE_NEW:
        if(ConfirmSaveChanges())
        {
            MainWndOnFileNew();
        }
        break;
    case IDM_FILE_OPEN:
        if(ConfirmSaveChanges())
        {
            MainWndOnFileOpen();
        }
        break;
    case IDM_FILE_SAVE:
        MainWndOnFileSave();
        break;
    case IDM_FILE_SAVE_AS:
        MainWndOnFileSaveAs();
        break;
    case IDM_FILE_EXIT:
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        break;
    case IDM_VIEW_WORDWRAP:
        MainWndOnViewWordWrap();
        break;
    case IDM_VIEW_DARKMODE:
        MainWndOnViewDarkMode();
        break;
    case IDM_EDIT_UNDO:
        SendMessage(g_hwndEdit, EM_UNDO, 0, 0);
        break;
    case IDM_EDIT_CUT:
        SendMessage(g_hwndEdit, WM_CUT, 0, 0);
        break;
    case IDM_EDIT_COPY:
        SendMessage(g_hwndEdit, WM_COPY, 0, 0);
        break;
    case IDM_EDIT_PASTE:
        SendMessage(g_hwndEdit, WM_PASTE, 0, 0);
        break;
    case IDM_EDIT_DELETE:
        SendMessage(g_hwndEdit, WM_CLEAR, 0, 0);
        break;
    case IDM_EDIT_SELECT_ALL:
        SendMessage(g_hwndEdit, EM_SETSEL, 0, -1);
        break;
    case IDC_EDIT:
        EditControlOnCommand(code);
        break;
    }

    return 0;
}

//
// MainWndProc
// main windows procdure
//
LRESULT CALLBACK MainWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_CREATE:
        result = MainWndOnCreate(hwnd);
        break;
    case WM_SIZE:
        result = MainWndOnResize((int)LOWORD(lparam), (int)HIWORD(lparam));
        break;
    case WM_COMMAND:
        result = MainWndOnCommand(hwnd, (int)LOWORD(wparam), (int)HIWORD(wparam));
        break;
    case WM_CTLCOLOREDIT:
        result = MainWndOnControlColorEdit((HDC)wparam);
        break;
    case WM_DPICHANGED:
        result = MainWndOnDpiChanged(HIWORD(wparam));
        break;
    case WM_CLOSE:
        if(ConfirmSaveChanges())
        {
            DestroyWindow(hwnd); // if the main window is closed, destroy it
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); // quit the program by posting a WM_QUIT
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return result;
}

//
// MsgLoop
// message loop
//
int MsgLoop(void)
{
    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(g_hinst, MAKEINTRESOURCE(IDR_ACCELMAIN));

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if(!TranslateAccelerator(g_hwndMain, hAccelTable, &msg))
        {
            TranslateMessage(&msg); // translate WM_KEYDOWN to WM_CHAR
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}
