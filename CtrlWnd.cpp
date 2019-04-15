#include "MainWnd.h"


typedef struct {
	HWND hwnd;
	int dx;
	int dy;
	int sx;
	int sy;
	int cx;
	int cy;
	int id;
} BTN;

BTN Btn[7] = {
	NULL, 26,13,  0, 98,27,25,ID_REAL,
	NULL, 65,13,  0,123,27,25,ID_FIT,
	NULL,104,13,  0, 48,51,25,ID_PREV,
	NULL,155, 2,  0,  0,44,48,ID_FULL,
	NULL,199,13,  0, 73,51,25,ID_NEXT,
	NULL,262,13,108, 98,25,27,ID_TURNL,
	NULL,299,13,108,123,25,27,ID_TURNR};

BOOL bTrack;


LRESULT CALLBACK CtrlWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CreatePaneWnd();
LRESULT CALLBACK PaneWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CreatePaneBtn();
LRESULT CALLBACK PaneBtnProc(HWND, UINT, WPARAM, LPARAM);
void OnCtrlPaint();
void OnPanePaint();
void OnBtnMouseMove(HWND);
void DrawButtonBitmap(HWND, int, BOOL);


BOOL CreateCtrlWnd()
{
	BOOL br;
	WNDCLASSEX wcex;
	
	wcex.cbClsExtra		= 0;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.cbWndExtra		= 0;
	wcex.hbrBackground	= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hInstance		= hMainInst;
	wcex.lpfnWndProc	= CtrlWndProc;
	wcex.lpszClassName	= _T("PhotoViewer_CtrlWnd");
	wcex.lpszMenuName	= NULL;
	wcex.style			= CS_PARENTDC;
	br = RegisterClassEx(&wcex);
	if (!br)
		return FALSE;

	hCtrlWnd = CreateWindowEx(0, _T("PhotoViewer_CtrlWnd"), NULL,
		WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
		0, 0, 1, 1, hMainWnd, NULL, hMainInst, NULL);
	if (!hCtrlWnd)
		return FALSE;

	br = CreatePaneWnd();

	return br;
}

LRESULT CALLBACK CtrlWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		OnCtrlPaint();
		break;
	case WM_CONTEXTMENU:
		OnViewMenu(lParam);
		break;
	case WM_LBUTTONDOWN:
		PostMessage(bFull?hCtrlWnd:hMainWnd,WM_SYSCOMMAND,SC_MOVE|HTCAPTION,0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

BOOL CreatePaneWnd()
{
	BOOL br;
	WNDCLASSEX wcex;
	
	wcex.cbClsExtra		= 0;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.cbWndExtra		= 0;
	wcex.hbrBackground	= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hInstance		= hMainInst;
	wcex.lpfnWndProc	= PaneWndProc;
	wcex.lpszClassName	= _T("PhotoViewer_PaneWnd");
	wcex.lpszMenuName	= NULL;
	wcex.style			= CS_PARENTDC;
	br = RegisterClassEx(&wcex);
	if (!br)
		return FALSE;

	hPaneWnd = CreateWindowEx(0, _T("PhotoViewer_PaneWnd"), NULL,
		WS_CHILD | WS_VISIBLE,
		0, 0, 1, 1, hCtrlWnd, NULL, hMainInst, NULL);
	if (!hPaneWnd)
		return FALSE;
	
	br = CreatePaneBtn();

	return br;
}

LRESULT CALLBACK PaneWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		OnPanePaint();
		break;
	case WM_LBUTTONDOWN:
		PostMessage(bFull?hCtrlWnd:hMainWnd,WM_SYSCOMMAND,SC_MOVE|HTCAPTION,0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

BOOL CreatePaneBtn()
{
	BOOL br;
	WNDCLASSEX wcex;
	int i;
	
	wcex.cbClsExtra		= 0;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.cbWndExtra		= 0;
	wcex.hbrBackground	= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hInstance		= hMainInst;
	wcex.lpfnWndProc	= PaneBtnProc;
	wcex.lpszClassName	= _T("PhotoViewer_PaneBtn");
	wcex.lpszMenuName	= NULL;
	wcex.style			= CS_PARENTDC;
	br = RegisterClassEx(&wcex);
	if (!br)
		return FALSE;

	for (i=0; i<7; i++)
	{
		Btn[i].hwnd = CreateWindowEx(0, _T("PhotoViewer_PaneBtn"), NULL,
			WS_CHILD | WS_VISIBLE | WS_DISABLED,
			Btn[i].dx, Btn[i].dy, Btn[i].cx, Btn[i].cy,
			hPaneWnd, NULL, hMainInst, NULL);
		if (!Btn[i].hwnd)
			return FALSE;

		BitBlt(hPaneDC, Btn[i].dx, Btn[i].dy, Btn[i].cx, Btn[i].cy,
			hPaneBmpDC, Btn[i].sx, Btn[i].sy, SRCCOPY);
	}

	EnableWindow(Btn[3].hwnd, TRUE);

	return TRUE;
}

LRESULT CALLBACK PaneBtnProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ENABLE:
		DrawButtonBitmap(hWnd, (int)wParam, FALSE);
		break;
	case WM_MOUSEMOVE:
		OnBtnMouseMove(hWnd);
		break;
	case WM_MOUSEHOVER:
		DrawButtonBitmap(hWnd, 2, FALSE);
		break;
	case WM_MOUSELEAVE:
		bTrack = FALSE;
		if (IsWindowEnabled(hWnd))
			DrawButtonBitmap(hWnd, 1, FALSE);
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		DrawButtonBitmap(hWnd, 3, FALSE);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		DrawButtonBitmap(hWnd, 2, TRUE);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void OnCtrlPaint()
{
	PAINTSTRUCT ps;
	RECT rc;

	if (BeginPaint(hCtrlWnd, &ps))
		if (GetClientRect(hCtrlWnd, &rc))
			BitBlt(ps.hdc, 0, 0, rc.right, 52, hCtrlDC, 0, 0, SRCCOPY);
	EndPaint(hCtrlWnd, &ps);
}

void OnPanePaint()
{
	PAINTSTRUCT ps;

	if (BeginPaint(hPaneWnd, &ps))
		BitBlt(ps.hdc, 0, 0, 350, 52, hPaneDC, 0, 0, SRCCOPY);
	EndPaint(hPaneWnd, &ps);
}

void OnBtnMouseMove(HWND hWnd)
{
	TRACKMOUSEEVENT tme;

	if (!bTrack)
	{
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 10;
		tme.hwndTrack = hWnd;
		if (TrackMouseEvent(&tme))
			bTrack = TRUE;
	}
}

void DrawButtonBitmap(HWND hWnd, int n, BOOL bPost)
{
	int i;
	int sx;

	for (i=0; i<7; i++)
	{
		if (hWnd == Btn[i].hwnd)
		{
			sx = Btn[i].cx * n + Btn[i].sx;
			BitBlt(hPaneDC, Btn[i].dx, Btn[i].dy, Btn[i].cx, Btn[i].cy,
				hPaneBmpDC, sx, Btn[i].sy, SRCCOPY);
			InvalidateRect(hPaneWnd, NULL, FALSE);
			if (bPost)
				PostMessage(hViewWnd, WM_COMMAND, Btn[i].id, 0);
			break;
		}
	}
}

void EnableButton(int id, BOOL bEnable)
{
	int i;
	BOOL br;

	for(i=0; i<7; i++)
	{
		if (id == Btn[i].id)
		{
			br = IsWindowEnabled(Btn[i].hwnd);
			if (br != bEnable)
				EnableWindow(Btn[i].hwnd, bEnable);
			break;
		}
	}
}