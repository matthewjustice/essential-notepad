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

// Resource constants
#define IDR_MENUMAIN       200
#define IDR_ACCELMAIN      201
#define IDM_FILE_NEW       300
#define IDM_FILE_OPEN      301
#define IDM_FILE_SAVE      302
#define IDM_FILE_SAVE_AS   303
#define IDM_FILE_EXIT      304

// Function prototypes - main
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitApp();
BOOL InitWindow(int);
int MsgLoop(void);

// Function prototypes - file.c
void MainWndOnOpenFile(void);

#endif // _ESNPAD_H_
