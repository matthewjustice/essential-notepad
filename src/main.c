/* -------------------------------------------------------------

main.c
    Essential Notepad - A basic Notepad implementation for Windows

by: Matthew Justice

---------------------------------------------------------------*/

#include <windows.h>

//
// function prototypes
//
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitApp();
BOOL InitWindow(int);
int MsgLoop(void);

//
// globals
//
HINSTANCE g_hinst;  // main instance handle
HWND g_hwndMain;    // handle to main window
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
    // a window class structure describes the window
    WNDCLASSEX wc;
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
// MainWndProc
// main windows procdure
//
LRESULT CALLBACK MainWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd); // if the main window is closed, destroy it
        break;
    case WM_DESTROY:
        PostQuitMessage(0); // quit the program by posting a WM_QUIT
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return 0;
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
