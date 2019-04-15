#pragma once


#include <Windows.h>
#include <tchar.h>
#include <WindowsX.h>
#include <WinInet.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include "Resource.h"
#include "CtrlWnd.h"
#include "Decoder.h"
#include "ViewWnd.h"


#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")


extern HINSTANCE hMainInst;
extern HWND hMainWnd;
extern HWND hCtrlWnd;
extern HWND hPaneWnd;
extern HWND hViewWnd;

extern HBRUSH hCtrlBrush;
extern HBRUSH hViewBrush;
extern HDC hCtrlDC;
extern HDC hPaneDC;
extern HDC hViewDC;
extern HDC hPaneBmpDC;

extern BOOL bHideCtrl;
extern BOOL bFull;
extern int cxScreen;
extern int cyScreen;

#define ID_REAL		1001
#define ID_FIT		1002
#define ID_PREV		VK_LEFT
#define ID_FULL		VK_RETURN
#define ID_NEXT		VK_RIGHT
#define ID_TURNL	'L'
#define ID_TURNR	'R'

#define ID_HOME		VK_HOME
#define ID_END		VK_END

#define ID_CENT		1021
#define ID_TILE		1022
#define ID_STRE		1023
#define ID_KEEP		1024
#define ID_CROP		1025

#define ID_OPEN		1005
#define ID_OLOC		1006
#define ID_PGUP		VK_PRIOR
#define ID_PGDN		VK_NEXT
#define ID_UP		VK_UP
#define ID_DOWN		VK_DOWN
#define ID_ZOOM		1007
#define ID_ANTI		1008
#define ID_CFILE	1009
#define ID_CPATH	1010
#define ID_INFO		1011
#define ID_HIDE		1012
#define ID_ESC		VK_ESCAPE
#define ID_EXIT		1013
