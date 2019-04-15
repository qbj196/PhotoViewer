#include "MainWnd.h"


HINSTANCE hMainInst;
HWND hMainWnd;
HWND hCtrlWnd;
HWND hPaneWnd;
HWND hViewWnd;

HFONT hFont;
HBRUSH hCtrlBrush;
HBRUSH hViewBrush;
HDC hCtrlDC;
HDC hPaneDC;
HDC hViewDC;
HDC hPaneBmpDC;

BOOL bHideCtrl;
BOOL bFull;
int cxScreen;
int cyScreen;


LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
void OnMainWindowPosChanged(LPARAM);
void OnMainDrop(WPARAM);
BOOL CreateMainWnd();
BOOL CreateBackgroundDC();
BOOL LoadBitmapFromResource();


int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;

	hMainInst = hInst;

	if (!BrFailedReturn(!CreateMainWnd(), TRUE))
	{
		if (lpCmdLine[0] != _T('\0'))
			OpenBitmapFile(lpCmdLine);

		ShowWindow(hMainWnd, nCmdShow);
		UpdateWindow(hMainWnd);

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (hFont)
		DeleteObject(hFont);
	if (hCtrlBrush)
		DeleteObject(hCtrlBrush);
	if (hViewBrush)
		DeleteObject(hViewBrush);
	if (hCtrlDC)
		DeleteDC(hCtrlDC);
	if (hPaneDC)
		DeleteDC(hPaneDC);
	if (hViewDC)
		DeleteDC(hViewDC);
	if (hPaneBmpDC)
		DeleteDC(hPaneBmpDC);
	OnViewRelease(TRUE);

	return 0;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_WINDOWPOSCHANGED:
		OnMainWindowPosChanged(lParam);
		break;
	case WM_DROPFILES:
		OnMainDrop(wParam);
		break;
	case WM_KEYDOWN:
		PostMessage(hViewWnd, WM_COMMAND, wParam, 0);
		break;
	case WM_MOUSEWHEEL:
		PostMessage(hViewWnd, WM_COMMAND, ID_ZOOM, wParam);
		break;
	case WM_DISPLAYCHANGE:
		cxScreen = LOWORD(lParam);
		cyScreen = HIWORD(lParam);
		if (BrFailedReturn(!CreateBackgroundDC(), TRUE))
			DestroyWindow(hMainWnd);
		break;
	case WM_DESTROY:
		KillTimer(hViewWnd, 1);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void OnMainWindowPosChanged(LPARAM lParam)
{
	WINDOWPOS *p;
	RECT rc;
	int w;
	int h;

	p = (WINDOWPOS *)lParam;

	GetClientRect(hMainWnd, &rc);
	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
	if (bFull)
	{
		MoveWindow(hCtrlWnd, (w - 360), (h - 62), 350, 52, TRUE);
		MoveWindow(hPaneWnd, 0, 0, 350, 52, TRUE);
		MoveWindow(hViewWnd, 0, 0, w, h, TRUE);
	}
	else
	{
		MoveWindow(hCtrlWnd, 0, (h - 52), w, 52, TRUE);
		MoveWindow(hPaneWnd, (w - 350) / 2, 0, 350, 52, TRUE);
		if (!bHideCtrl)
			h -= 52;
		MoveWindow(hViewWnd, 0, 0, w, h, TRUE);
	}
}

void OnMainDrop(WPARAM wParam)
{
	UINT ur;
	TCHAR szfile[1024];

	SetForegroundWindow(hMainWnd);

	ur = DragQueryFile((HDROP)wParam, 0, szfile, 1024);
	if (!BrFailedReturn(!ur, FALSE))
		OpenBitmapFile(szfile);
}

BOOL CreateMainWnd()
{
	BOOL br;
	NONCLIENTMETRICS ncm;
	WNDCLASSEX wcex;

	ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
	ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
	br = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	if (!br)
		return FALSE;

	hFont = CreateFontIndirect(&ncm.lfStatusFont);
	if (!hFont)
		return FALSE;

	hCtrlBrush = CreateSolidBrush(RGB(96, 96, 96));
	if (!hCtrlBrush)
		return FALSE;

	hViewBrush = CreateSolidBrush(RGB(160, 160, 160));
	if (!hViewBrush)
		return FALSE;

	cxScreen = GetSystemMetrics(SM_CXSCREEN);
	cyScreen = GetSystemMetrics(SM_CYSCREEN);
	br = CreateBackgroundDC();
	if (!br)
		return FALSE;

	br = LoadBitmapFromResource();
	if (!br)
		return FALSE;

	wcex.cbClsExtra		= 0;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.cbWndExtra		= 0;
	wcex.hbrBackground	= hViewBrush;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon			= LoadIcon(hMainInst, MAKEINTRESOURCE(IDI_MAIN));
	wcex.hIconSm		= wcex.hIcon;
	wcex.hInstance		= hMainInst;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.lpszClassName	= _T("PhotoViewer");
	wcex.lpszMenuName	= NULL;
	wcex.style			= 0;
	br = RegisterClassEx(&wcex);
	if (!br)
		return FALSE;

	hMainWnd = CreateWindowEx(WS_EX_ACCEPTFILES,
		_T("PhotoViewer"), _T("PhotoViewer"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hMainInst, NULL);
	if (!hMainWnd)
		return FALSE;

	br = CreateCtrlWnd();
	if (!br)
		return FALSE;

	br = CreateViewWnd();

	return br;
}

BOOL CreateBackgroundDC()
{
	BOOL br;
	HGDIOBJ hobj;
	COLORREF color;

	br = CreateCanvasDC(&hCtrlDC, hCtrlBrush, cxScreen, 52);
	if (!br)
		return FALSE;

	hobj = SelectObject(hCtrlDC, hFont);
	if (hobj == NULL || hobj == HGDI_ERROR)
		return FALSE;
	
	br = SetBkMode(hCtrlDC, TRANSPARENT);
	if (!br)
		return FALSE;

	color = SetTextColor(hCtrlDC, RGB(255, 255, 255));
	if (color == CLR_INVALID)
		return FALSE;
	
	br = CreateCanvasDC(&hPaneDC, hCtrlBrush, 350, 52);
	if (!br)
		return FALSE;

	br = CreateCanvasDC(&hViewDC, hViewBrush, cxScreen, cyScreen);
	if (!br)
		return FALSE;
	
	hobj = SelectObject(hViewDC, hFont);
	if (hobj == NULL || hobj == HGDI_ERROR)
		return FALSE;
	
	br = SetBkMode(hViewDC, TRANSPARENT);
	if (!br)
		return FALSE;

	br = SetGraphicsMode(hViewDC, GM_ADVANCED);
	if (!br)
		return FALSE;

	br = SetStretchBltMode(hViewDC, HALFTONE);

	return br;
}

BOOL LoadBitmapFromResource()
{
	BOOL br = FALSE;
	HBITMAP hbmp = NULL;
	HDC hdc = NULL;
	HDC hbmpdc = NULL;
	HGDIOBJ hobj;
	BITMAP bmp;
	BLENDFUNCTION ftn;

	hbmp = LoadBitmap(hMainInst, MAKEINTRESOURCE(IDB_PANE));
	if (!hbmp)
		goto end;

	hdc = GetDC(NULL);
	if (!hdc)
		goto end;

	hbmpdc = CreateCompatibleDC(hdc);
	if (!hbmpdc)
		goto end;

	hobj = SelectObject(hbmpdc, hbmp);
	if (hobj == NULL || hobj == HGDI_ERROR)
		goto end;

	br = GetObject(hbmp, sizeof(BITMAP), &bmp);
	if (!br)
		goto end;

	br = CreateCanvasDC(&hPaneBmpDC, hCtrlBrush, bmp.bmWidth, bmp.bmHeight);
	if (!br)
		goto end;

	ftn.AlphaFormat = AC_SRC_ALPHA;
	ftn.BlendFlags = 0;
	ftn.BlendOp = 0;
	ftn.SourceConstantAlpha = 255;
	br = GdiAlphaBlend(hPaneBmpDC, 0, 0, bmp.bmWidth, bmp.bmHeight,
		hbmpdc, 0, 0, bmp.bmWidth, bmp.bmHeight, ftn);

end:
	if (hbmpdc)
		DeleteDC(hbmpdc);
	if (hdc)
		ReleaseDC(NULL, hdc);
	if (hbmp)
		DeleteObject(hbmp);

	return br;
}