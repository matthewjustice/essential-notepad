/* -------------------------------------------------------------

main.c
    Essential Notepad - A basic Notepad implementation for Windows

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include "esnpad.h"

//
// globals
//
HINSTANCE g_hinst;  // main instance handle
HWND g_hwndMain;    // handle to main window
HWND g_hwndEdit = NULL;    // handle to edit control
HWND g_hwndStatus = NULL;  // handle to status control
TCHAR g_nameMainClass[] = TEXT("MainWinClass"); // name of the main window class
TCHAR g_appTitle[]  = TEXT("Essential Notepad"); // title of the application

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
    // Save the value of the main instance handle
    g_hinst = hInstance;

    // Initialize the window data and register the window class
    if (!InitApp())
    {
        MessageBox(NULL, TEXT("Error during app init"),
            g_appTitle, MB_ICONERROR);

        return -1;
    }

    // Create the window and display it
    if(!InitWindow(nShowCmd))
    {
        MessageBox(NULL, TEXT("Error during window init"),
            g_appTitle, MB_ICONERROR);

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
    g_hwndMain = CreateWindowEx(0, g_nameMainClass, g_appTitle,
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
    g_hwndEdit = CreateWindowEx(0, TEXT("Edit"), NULL,
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
// MainWndOnCommand
// Handles WM_COMMAND for the main window
//
LRESULT MainWndOnCommand(HWND hwnd, int id)
{
    switch(id)
    {
    case IDM_FILE_EXIT:
        SendMessage(hwnd, WM_CLOSE, 0, 0);
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
        result = MainWndOnCommand(hwnd, (int)LOWORD(wparam));
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
