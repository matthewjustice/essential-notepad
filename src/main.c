/* -------------------------------------------------------------

main.c
    Essential Notepad - A basic Notepad implementation for Windows

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
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
    // create the main window
    g_hwndMain = CreateWindowEx(0, g_nameMainClass, APP_TITLE_W,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
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
// MainWndOnCreate
// Handles WM_CREATE for the main window
//
LRESULT MainWndOnCreate(HWND hwnd) {

    // Create the edit control window
    g_hwndEdit = CreateWindowEx(0, L"Edit", NULL,
        WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOHSCROLL|ES_AUTOVSCROLL,
        0, 0, 100, 100, hwnd, (HMENU)IDC_EDIT, g_hinst, NULL);

    if(!g_hwndEdit)
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
// MainWndOnCommand
// Handles WM_COMMAND for the main window
//
LRESULT MainWndOnCommand(HWND hwnd, int id, int code)
{
    switch(id)
    {
    case IDM_FILE_OPEN:
        MainWndOnFileOpen();
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
    case WM_CLOSE:
        DestroyWindow(hwnd); // if the main window is closed, destroy it
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

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg); // translate WM_KEYDOWN to WM_CHAR
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
