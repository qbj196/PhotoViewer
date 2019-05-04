#include "MainWnd.h"


BMP Bmp;
UINT uLoop;
BOOL bFit;
BOOL bAnti;
BOOL bInfo;
BOOL bPaint;
BOOL bLBtnDown;
WINDOWPLACEMENT wpMain;
TCHAR szInfo[256];

int nFiles;
int nIndex;
TCHAR szFile[1024];
TCHAR **szList;


LRESULT CALLBACK ViewWndProc(HWND, UINT, WPARAM, LPARAM);
void PaintViewDC();
void OnViewPaint();
void OnViewCommand(WPARAM, LPARAM);
void OnViewTimer();
void BitmapLoad();
void BitmapFit();
void BitmapReal();
void BitmapTurn(BOOL);
void BitmapMove(LPARAM);
void BitmapZoom(WPARAM);
void BitmapNext();
void BitmapPrev();
void PageNext();
void PagePrev();
int GetFileList(TCHAR *, int);
void FreeFileList();
void OpenFileDialog();
void OpenFileLocation();
void SetWallpaper(DWORD);
void CopyFilePath(BOOL);
void ShowInfo();
void FullScreen();
void ShowFailedInfo();


BOOL CreateViewWnd()
{
	BOOL br;
	WNDCLASSEX wcex;

	Bmp.hbr = hViewBrush;

	wcex.cbClsExtra		= 0;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.cbWndExtra		= 0;
	wcex.hbrBackground	= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon			= NULL;
	wcex.hIconSm		= NULL;
	wcex.hInstance		= hMainInst;
	wcex.lpfnWndProc	= ViewWndProc;
	wcex.lpszClassName	= _T("PhotoViewer_ViewWnd");
	wcex.lpszMenuName	= NULL;
	wcex.style			= CS_PARENTDC | CS_VREDRAW | CS_HREDRAW;
	br = RegisterClassEx(&wcex);
	if (!br)
		return FALSE;

	hViewWnd = CreateWindowEx(0, _T("PhotoViewer_ViewWnd"), NULL,
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 1, 1, hMainWnd, NULL, hMainInst, NULL);
	if (!hViewWnd)
		return FALSE;

	return TRUE;
}

LRESULT CALLBACK ViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		OnViewPaint();
		break;
	case WM_SIZE:
		BitmapFit();
		bPaint = TRUE;
		break;
	case WM_TIMER:
		OnViewTimer();
		break;
	case WM_COMMAND:
		OnViewCommand(wParam, lParam);
		break;
	case WM_CONTEXTMENU:
		OnViewMenu(lParam);
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		bLBtnDown = TRUE;
		Bmp.pos.xx = GET_X_LPARAM(lParam);
		Bmp.pos.yy = GET_Y_LPARAM(lParam);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		bLBtnDown = FALSE;
		break;
	case WM_MOUSEMOVE:
		BitmapMove(lParam);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void OnViewRelease(BOOL bexit)
{
	if (bexit)
		FreeFileList();

	ReleaseBitmap(&Bmp, bexit);
}

void PaintViewDC()
{
	if (!Bmp.hdc)
	{
		EnableButton(ID_REAL, FALSE);
		EnableButton(ID_FIT, FALSE);
		EnableButton(ID_TURNL, FALSE);
		EnableButton(ID_TURNR, FALSE);
		return;
	}

	if (Bmp.pos.cx != Bmp.w && Bmp.pos.cy != Bmp.h)
		EnableButton(ID_REAL, TRUE);
	else
		EnableButton(ID_REAL, FALSE);

	StretchBlt(hViewDC, Bmp.pos.x, Bmp.pos.y, Bmp.pos.cx, Bmp.pos.cy,
		Bmp.hdc, 0, 0, Bmp.w, Bmp.h, SRCCOPY);
}

void OnViewPaint()
{
	PAINTSTRUCT ps;
	RECT rc;

	if (BeginPaint(hViewWnd, &ps))
	{
		if (GetClientRect(hViewWnd, &rc))
		{
			if (bPaint)
			{
				FillRect(hViewDC, &rc, Bmp.hbr);
				PaintViewDC();
				ShowInfo();
				bPaint = FALSE;
			}
			BitBlt(ps.hdc, 0, 0, rc.right, rc.bottom, hViewDC, 0, 0, SRCCOPY);
		}
	}
	EndPaint(hViewWnd, &ps);
}

void OnViewCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case ID_REAL:
		BitmapReal();
		break;
	case ID_FIT:
		bFit = TRUE;
		BitmapFit();
		bPaint = TRUE;
		InvalidateRect(hViewWnd, NULL, FALSE);
		break;
	case ID_PREV:
		BitmapPrev();
		break;
	case ID_NEXT:
		BitmapNext();
		break;
	case ID_TURNL:
		BitmapTurn(TRUE);
		break;
	case ID_TURNR:
		BitmapTurn(FALSE);
		break;
	case ID_HOME:
		nIndex = 0;
		BitmapLoad();
		break;
	case ID_END:
		nIndex = nFiles - 1;
		BitmapLoad();
		break;
	case ID_CENT:
		SetWallpaper(WPSTYLE_CENTER);
		break;
	case ID_TILE:
		SetWallpaper(WPSTYLE_TILE);
		break;
	case ID_STRE:
		SetWallpaper(WPSTYLE_STRETCH);
		break;
	case ID_KEEP:
		SetWallpaper(WPSTYLE_KEEPASPECT);
		break;
	case ID_CROP:
		SetWallpaper(WPSTYLE_CROPTOFIT);
		break;
	case ID_OPEN:
		OpenFileDialog();
		break;
	case ID_OLOC:
		OpenFileLocation();
		break;
	case ID_PGUP:
		PagePrev();
		break;
	case ID_PGDN:
		PageNext();
		break;
	case ID_UP:
		BitmapZoom(0x00780000);
		break;
	case ID_DOWN:
		BitmapZoom(0xff880000);
		break;
	case ID_ZOOM:
		BitmapZoom(lParam);
		break;
	case ID_ANTI:
		bAnti = !bAnti;
		SetStretchBltMode(hViewDC, bAnti ? COLORONCOLOR : HALFTONE);
		bPaint = TRUE;
		InvalidateRect(hViewWnd, NULL, FALSE);
		break;
	case ID_CFILE:
		CopyFilePath(FALSE);
		break;
	case ID_CPATH:
		CopyFilePath(TRUE);
		break;
	case ID_INFO:
		if (bInfo < 2)
			bInfo = 3;
		else if (bInfo < 4)
			bInfo = 4;
		if (bHideCtrl || bFull)
		{
			bPaint = TRUE;
			InvalidateRect(hViewWnd, NULL, FALSE);
		}
		else
			ShowInfo();
		break;
	case ID_HIDE:
		bHideCtrl = !bHideCtrl;
		ShowWindow(hCtrlWnd, bHideCtrl ? SW_HIDE : SW_SHOW);
		if (!bFull)
			PostMessage(hMainWnd, WM_SIZE, 0, 0);
		break;
	case ID_FULL:
		FullScreen();
		break;
	case ID_ESC:
		if (bFull)
			FullScreen();
		break;
	case ID_EXIT:
		DestroyWindow(hMainWnd);
		break;
	}
}

void OnViewMenu(LPARAM lParam)
{
	HMENU hm = NULL;
	HMENU hsm = NULL;
	UINT uflag;
	UINT unext;
	UINT uprev;
	int x;
	int y;

	uflag = MF_GRAYED;
	unext = MF_GRAYED;
	uprev = MF_GRAYED;
	if (Bmp.hdc)
	{
		uflag = MF_ENABLED;
		if (!Bmp.bgif)
		{
			if (Bmp.uindex < Bmp.ucount-1)
				unext = MF_ENABLED;
			if (Bmp.uindex > 0)
				uprev = MF_ENABLED;
		}
	}

	hsm = CreatePopupMenu();
	if (BrFailedReturn(!hsm, FALSE))
		goto end;
	AppendMenu(hsm, MF_ENABLED, ID_CROP, _T("填充"));
	AppendMenu(hsm, MF_ENABLED, ID_KEEP, _T("适应"));
	AppendMenu(hsm, MF_ENABLED, ID_STRE, _T("拉伸"));
	AppendMenu(hsm, MF_ENABLED, ID_TILE, _T("平铺"));
	AppendMenu(hsm, MF_ENABLED, ID_CENT, _T("居中"));

	hm = CreatePopupMenu();
	if (BrFailedReturn(!hm, FALSE))
		goto end;
	AppendMenu(hm, MF_ENABLED, ID_OPEN, _T("打开"));
	AppendMenu(hm, MF_ENABLED, ID_OLOC, _T("打开文件位置"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag | MF_POPUP, (UINT_PTR)hsm, _T("设置为桌面背景"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag, ID_TURNL, _T("向左旋转"));
	AppendMenu(hm, uflag, ID_TURNR, _T("向右旋转"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uprev, ID_PGUP, _T("上一页"));
	AppendMenu(hm, unext, ID_PGDN, _T("下一页"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag, ID_UP, _T("放大"));
	AppendMenu(hm, uflag, ID_DOWN, _T("缩小"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag, ID_ANTI, bAnti ? _T("抗锯齿") : _T("不抗锯齿"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag, ID_CFILE, _T("复制"));
	AppendMenu(hm, uflag, ID_CPATH, _T("复制文件路径"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, uflag, ID_INFO,bInfo>1?_T("隐藏文件信息"):_T("显示文件信息"));
	AppendMenu(hm, 0, ID_HIDE, bHideCtrl ?_T("显示控制窗口"):_T("隐藏控制窗口"));
	AppendMenu(hm, 0, ID_FULL, bFull ? _T("退出全屏") : _T("全屏"));
	AppendMenu(hm, MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, MF_ENABLED, ID_EXIT, _T("退出"));

	x = GET_X_LPARAM(lParam);
	y = GET_Y_LPARAM(lParam);
	TrackPopupMenu(hm, TPM_RIGHTBUTTON, x, y, 0, hViewWnd, NULL);

end:
	if (hsm)
		DestroyMenu(hsm);
	if (hm)
		DestroyMenu(hm);
}

void OnViewTimer()
{
	KillTimer(hViewWnd, 1);

	Bmp.uindex = uLoop;
	if (!GetBitmap(&Bmp, szInfo, 256))
	{
		ShowFailedInfo();
		return;
	}
	EnableButton(ID_TURNL, TRUE);
	EnableButton(ID_TURNR, TRUE);

	Bmp.gif.xx = 0;
	if (Bmp.bgif && Bmp.ucount>1)
	{
		Bmp.gif.xx = Bmp.gif2.xx;
		if (Bmp.gif.xx < 20)
			Bmp.gif.xx = 90;
		SetTimer(hViewWnd, 1, Bmp.gif.xx, NULL);
	}

	BitmapFit();
	if (bInfo == 1)
		bInfo = 4;
	else if (bInfo == 2)
		bInfo = 3;
	bPaint = TRUE;
	InvalidateRect(hViewWnd, NULL, FALSE);
	uLoop = ++ uLoop % Bmp.ucount;
}

void OpenBitmapFile(TCHAR *pszfile)
{
	TCHAR szname[260];
	TCHAR *p;
	int n;

	KillTimer(hViewWnd, 1);
	OnViewRelease(FALSE);
	FreeFileList();

	StringCchCopy(szFile, 1024, pszfile);
	PathRemoveBlanks(szFile);
	PathUnquoteSpaces(szFile);

	p = PathFindFileName(szFile);
	StringCchCopy(szname, 260, p);
	StringCchCopy(p, 2, _T("*\0"));

	n = GetFileList(szname, 0);
	if (BrFailedReturn(n < 1, FALSE))
		return;
	szList = (TCHAR **)malloc(sizeof(szList) * n);
	if (BrFailedReturn(!szList, FALSE))
		return;
	nFiles = GetFileList(szname, n);
	if (BrFailedReturn(nFiles < 1, FALSE))
		return;

	BitmapLoad();
}

void BitmapLoad()
{
	TCHAR sztitle[260];
	TCHAR *p;

	if (nFiles < 1)
		return;

	KillTimer(hViewWnd, 1);
	OnViewRelease(FALSE);

	if (nIndex > 0 && nIndex < nFiles)
		EnableButton(ID_PREV, TRUE);
	else
		EnableButton(ID_PREV, FALSE);

	if (nIndex > -1 && nIndex < nFiles-1)
		EnableButton(ID_NEXT, TRUE);
	else
		EnableButton(ID_NEXT, FALSE);

	StringCchPrintf(sztitle, 260, _T("[%d/%d] %s"),
		nIndex+1, nFiles, szList[nIndex]);
	SetWindowText(hMainWnd, sztitle);

	p = PathFindFileName(szFile);
	StringCchCopy(p, 1024 - (p - szFile), szList[nIndex]);

	uLoop = 0;
	bFit = TRUE;
	if (!LoadBitmapFromFile(&Bmp, szFile, szInfo, 256))
	{
		ShowFailedInfo();
		return;
	}

	bFit = TRUE;
	OnViewTimer();
}

void BitmapFit()
{
	RECT rc;
	int w, h;

	if (!bFit)
		return;
	if (!Bmp.hdc)
		return;
	if (!GetClientRect(hViewWnd, &rc))
		return;

	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
	if (w < Bmp.w)
	{
		Bmp.pos.x = 0;
		Bmp.pos.cx = w;
	}
	else
	{
		Bmp.pos.x = (w - Bmp.w) / 2;
		Bmp.pos.cx = Bmp.w;
	}

	Bmp.pos.cy = (Bmp.pos.cx * Bmp.h) / Bmp.w;
	if (h < Bmp.pos.cy)
	{
		Bmp.pos.cy = h;
		Bmp.pos.cx = (Bmp.pos.cy * Bmp.w) / Bmp.h;
		Bmp.pos.y = 0;
		Bmp.pos.x = (w - Bmp.pos.cx) / 2;
	}
	else
		Bmp.pos.y = (h - Bmp.pos.cy) / 2;
	EnableButton(ID_FIT, FALSE);
}

void BitmapReal()
{
	RECT rc;
	int w, h;

	if (!Bmp.hdc)
		return;
	if (!GetClientRect(hViewWnd, &rc))
		return;

	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
	Bmp.pos.cx = Bmp.w;
	Bmp.pos.cy = Bmp.h;
	Bmp.pos.x = (w - Bmp.pos.cx) / 2;
	Bmp.pos.y = (h - Bmp.pos.cy) / 2;
	if (w < Bmp.pos.cx || h < Bmp.pos.cy)
		EnableButton(ID_FIT, TRUE);
	else
		EnableButton(ID_FIT, FALSE);
	bFit = FALSE;
	bPaint = TRUE;
	InvalidateRect(hViewWnd, NULL, FALSE);
}

void BitmapTurn(BOOL bLeft)
{
	int t;

	if (bLeft)
	{
		t = Bmp.r - 1;
		if (t < -3)
			t = 0;
	}
	else
	{
		t = Bmp.r + 1;
		if (t > 3)
			t = 0;
	}
	Bmp.r = t;

	if (Bmp.bgif)
		uLoop = 0;
	else
		uLoop = Bmp.uindex;
	bFit = TRUE;
	OnViewTimer();
}

void BitmapMove(LPARAM lParam)
{
	int x, y, cx, cy;
	
	if (!bLBtnDown)
		return;
	if (!Bmp.hdc)
		return;

	x = GET_X_LPARAM(lParam);
	y = GET_Y_LPARAM(lParam);
	cx = x - Bmp.pos.xx;
	cy = y - Bmp.pos.yy;
	if (cx != 0 || cy != 0)
	{
		Bmp.pos.x += cx;
		Bmp.pos.y += cy;
		Bmp.pos.xx = x;
		Bmp.pos.yy = y;
		bFit = FALSE;
		EnableButton(ID_FIT, TRUE);
		if (bInfo == 3)
			bInfo = 5;
		bPaint = TRUE;
		InvalidateRect(hViewWnd, NULL, FALSE);
	}
}

void BitmapZoom(WPARAM wParam)
{
	RECT rc;
	int w;

	if (!Bmp.hdc)
		return;
	if (!GetClientRect(hViewWnd, &rc))
		return;

	if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
	{
		w = (int)(Bmp.pos.cx * 1.2);
	}
	else
	{
		w = (int)(Bmp.pos.cx / 1.2);
		if (w < 5)
			return;
	}

	Bmp.pos.cx = w;
	Bmp.pos.cy = (Bmp.pos.cx * Bmp.h) / Bmp.w;
	Bmp.pos.x = (rc.right - Bmp.pos.cx) / 2;
	Bmp.pos.y = (rc.bottom - Bmp.pos.cy) / 2;
	bFit = FALSE;
	EnableButton(ID_FIT, TRUE);
	bPaint = TRUE;
	InvalidateRect(hViewWnd, NULL, FALSE);
}

void BitmapNext()
{
	if (nIndex < nFiles-1)
	{
		nIndex++;
		BitmapLoad();
	}
}

void BitmapPrev()
{
	if (nIndex > 0)
	{
		nIndex--;
		BitmapLoad();
	}
}

void PageNext()
{
	if (Bmp.hdc && !Bmp.bgif)
	{
		if (Bmp.uindex < Bmp.ucount-1)
		{
			bFit = TRUE;
			OnViewTimer();
		}
	}
}

void PagePrev()
{
	if (Bmp.hdc && !Bmp.bgif)
	{
		if (Bmp.uindex > 0)
		{
			if (uLoop == 0)
				uLoop = Bmp.ucount;
			uLoop -= 2;
			bFit = TRUE;
			OnViewTimer();
		}
	}
}

int GetFileList(TCHAR *pszName, int nCounts)
{
	HANDLE hfind;
	WIN32_FIND_DATA fd;
	TCHAR *pszext;
	int i, index, len;

	hfind = FindFirstFile(szFile, &fd);
	if (hfind == INVALID_HANDLE_VALUE)
		return -1;

	i = 0;
	index = -1;
	do {
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (index == -1 && !lstrcmp(pszName, fd.cFileName))
			{
				index = i;
			}
			else
			{
				pszext = PathFindExtension(fd.cFileName);
				len = lstrlen(pszext);
				if (len > 3 && len < 6)
				{
					switch (pszext[1])
					{
					case _T('b'):
					case _T('B'):
						if (lstrcmpi(pszext, _T(".bmp")))
							continue;
						break;
					case _T('g'):
					case _T('G'):
						if (lstrcmpi(pszext, _T(".gif")))
							continue;
						break;
					case _T('i'):
					case _T('I'):
						if (lstrcmpi(pszext, _T(".ico")))
							continue;
						break;
					case _T('j'):
					case _T('J'):
						if (lstrcmpi(pszext, _T(".jpe")))
							if (lstrcmpi(pszext, _T(".jpeg")))
								if (lstrcmpi(pszext, _T(".jpg")))
									continue;
						break;
					case _T('p'):
					case _T('P'):
						if (lstrcmpi(pszext, _T(".png")))
							continue;
						break;
					case _T('t'):
					case _T('T'):
						if (lstrcmpi(pszext, _T(".tif")))
							if (lstrcmpi(pszext, _T(".tiff")))
								continue;
						break;
					default:
						continue;
						break;
					}
				}
				else
					continue;
			}

			if (nCounts != 0)
			{
				len = lstrlen(fd.cFileName) + 1;
				szList[i] = (TCHAR *)malloc(len * sizeof(TCHAR));
				if (!szList[i])
				{
					index = -1;
					break;
				}
				StringCchCopy(szList[i++], len, fd.cFileName);
				if (i >= nCounts)
					break;
			}
			else
				i++;
		}
	} while (FindNextFile(hfind, &fd));

	FindClose(hfind);

	if (index == -1)
	{
		if (nCounts != 0)
		{
			nFiles = i;
			FreeFileList();
		}
		return -1;
	}

	if (nCounts != 0)
		nIndex = index;

	return i;
}

void FreeFileList()
{
	int i;

	if (szList)
	{
		for (i=0; i<nFiles; i++)
		{
			if (szList[i])
			{
				free(szList[i]);
				szList[i] = NULL;
			}
		}
		free(szList);
		szList = NULL;
	}
	nFiles = 0;
	nIndex = 0;
}

void OpenFileDialog()
{
	OPENFILENAME ofn;
	TCHAR szfile[1024];

	szfile[0] = _T('\0');
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hMainWnd;
	ofn.lpstrFile = szfile;
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrFilter =
		_T("所有图片文件\0")
		_T("*.bmp;*.gif;*.ico;*.jpe;*.jpeg;*.jpg;*.png;*.tif;*.tiff\0")
		_T("所有文件\0") _T("*.*\0");
	if (GetOpenFileName(&ofn))
		OpenBitmapFile(szfile);
	SetForegroundWindow(hMainWnd);
}

void OpenFileLocation()
{
	HRESULT hr;
	IShellLink *pisl = NULL;
	ITEMIDLIST *pidl = NULL;
	TCHAR szfile[MAX_PATH];
	TCHAR *psz;

	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		IID_IShellLink, (LPVOID *)&pisl);
	if (FAILED(hr))
		goto end;

	psz = szFile;
	if (szFile[0] == _T('\0'))
	{
		GetModuleFileName(NULL, szfile, MAX_PATH);
		psz = szfile;
	}

	hr = pisl->SetPath(psz);
	if (FAILED(hr))
		goto end;

	hr = pisl->GetIDList(&pidl);
	if (FAILED(hr))
		goto end;

	hr = SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);

end:
	if (FAILED(hr))
	{
		HrFailedReturn(hr, szInfo, 256);
		ShowFailedInfo();
	}
	if (pisl)
		pisl->Release();
}

void SetWallpaper(DWORD dwStyle)
{
	HRESULT hr;
	IActiveDesktop *piad = NULL;
	WALLPAPEROPT wopt;

	hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
		IID_IActiveDesktop, (void **)&piad);
	if (FAILED(hr))
		goto end;

	hr = piad->SetWallpaper(szFile, 0);
	if (FAILED(hr))
		goto end;

	wopt.dwSize = sizeof(WALLPAPEROPT);
	wopt.dwStyle = dwStyle;
	hr = piad->SetWallpaperOptions(&wopt, 0);
	if (FAILED(hr))
		goto end;

	hr = piad->ApplyChanges(AD_APPLY_ALL);

end:
	if (FAILED(hr))
	{
		HrFailedReturn(hr, szInfo, 256);
		ShowFailedInfo();
	}
	if (piad)
		piad->Release();
}

void CopyFilePath(BOOL bCopyPath)
{
	BOOL br = FALSE;
	HGLOBAL hgbl = NULL;
	SIZE_T dwbytes;
	BYTE *plock;
	UINT uformat;
	DROPFILES df;

	dwbytes = lstrlen(szFile) + 1;
	dwbytes *= sizeof(TCHAR);
	if (!bCopyPath)
	{
		dwbytes += sizeof(TCHAR);
		dwbytes += sizeof(DROPFILES);
	}

	hgbl = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwbytes);
	if (!hgbl)
		goto end;

	plock = (BYTE *)GlobalLock(hgbl);
	if (!plock)
		goto end;

	if (bCopyPath)
	{
		uformat = CF_UNICODETEXT;
		memcpy(plock, szFile, dwbytes);
	}
	else
	{
		uformat = CF_HDROP;
		df.pFiles = sizeof(DROPFILES);
		df.fNC = FALSE;
		df.fWide = TRUE;
		df.pt.x = 0;
		df.pt.y = 0;
		memcpy(plock, &df, sizeof(DROPFILES));
		dwbytes = lstrlen(szFile) * sizeof(TCHAR);
		memcpy(plock+sizeof(DROPFILES), szFile, dwbytes);
	}

	br = GlobalUnlock(hgbl);
	if (!br)
		if (NO_ERROR != GetLastError())
			goto end;

	br = OpenClipboard(NULL);
	if (!br)
		goto end;
	
	br = EmptyClipboard();
	if (!br)
		goto end;
	
	br = (BOOL)SetClipboardData(uformat, hgbl);

end:
	CloseClipboard();
	if (BrFailedReturn(!br, FALSE))
		if (hgbl)
			GlobalFree(hgbl);
}

void ShowInfo()
{
	HDC hdc;
	RECT rc;
	BOOL b;

	if (bInfo == 0)
		return;

	b = bFull || bHideCtrl;
	if (b)
	{
		hdc = hViewDC;
		GetClientRect(hViewWnd, &rc);
	}
	else
	{
		hdc = hCtrlDC;
		GetClientRect(hCtrlWnd, &rc);
	}

	rc.left = 4;
	rc.top = rc.bottom - 48;
	if (bInfo == 1 || bInfo == 2)
	{
		if (!b)
			FillRect(hCtrlDC, &rc, hCtrlBrush);
		DrawText(hdc, szInfo, lstrlen(szInfo), &rc, 0);
	}
	else if (bInfo == 3 || bInfo == 5)
	{
		if (!b)
			FillRect(hCtrlDC, &rc, hCtrlBrush);

		rc.right = 74;
		StringCchPrintf(szInfo, 256, _T("宽度: %d\n高度: %d\n位深: %d"),
			Bmp.uwidth, Bmp.uheight, Bmp.ubit);
		DrawText(hdc, szInfo, lstrlen(szInfo), &rc, 0);

		rc.left = rc.right;
		rc.right += 74;
		StringCchPrintf(szInfo, 256, _T("格式: %s\n放大: %.2f\n旋转: %d"),
			Bmp.lpfmt, (Bmp.pos.cx * 1.0) / Bmp.w, Bmp.r * 90);
		DrawText(hdc, szInfo, lstrlen(szInfo), &rc, 0);
			
		rc.left = rc.right;
		rc.right += 90;
		StringCchPrintf(szInfo,256, _T("页数: %d|%d\n延时: %d|%d\n文件: %d|%d"),
			Bmp.uindex+1, Bmp.ucount, Bmp.gif.xx, Bmp.gif2.xx,nIndex+1,nFiles);
		DrawText(hdc, szInfo, lstrlen(szInfo), &rc, 0);

		bInfo = 3;
	}
	else if (bInfo == 4)
	{
		FillRect(hCtrlDC, &rc, hCtrlBrush);
		bInfo = 0;
	}

	if (!b)
		InvalidateRect(hCtrlWnd, NULL, FALSE);
}

void FullScreen()
{
	LONG_PTR style;
	WINDOWPLACEMENT wp;

	bFull = !bFull;
	style = GetWindowLongPtr(hMainWnd, GWL_STYLE);

	ShowWindow(hViewWnd, SW_HIDE);
	ShowWindow(hCtrlWnd, SW_HIDE);
	if (bFull)
	{
		style = style & ~WS_CAPTION & ~WS_THICKFRAME;
		GetWindowPlacement(hMainWnd, &wpMain);
		if (wpMain.showCmd == SW_SHOWMAXIMIZED)
		{
			SetWindowLongPtr(hMainWnd, GWL_STYLE, style);
			MoveWindow(hMainWnd, 0, 0, cxScreen, cyScreen, TRUE);
		}
		else
		{
			MoveWindow(hMainWnd, 0, 0, cxScreen, cyScreen, TRUE);
			SetWindowLongPtr(hMainWnd, GWL_STYLE, style);
		}
	}
	else
	{
		style = style | WS_CAPTION | WS_THICKFRAME;
		GetWindowPlacement(hMainWnd, &wp);
		wpMain.showCmd = wp.showCmd;
		SetWindowLongPtr(hMainWnd, GWL_STYLE, style);
		SetWindowPlacement(hMainWnd, &wpMain);
		if (wpMain.showCmd == SW_SHOWMAXIMIZED)
			PostMessage(hMainWnd, WM_SIZE, 0, 0);
	}
	ShowWindow(hViewWnd, SW_SHOW);
	ShowWindow(hCtrlWnd, SW_SHOW);
}

void ShowFailedInfo()
{
	if (bInfo == 2 || bInfo == 3)
		bInfo = 2;
	else
		bInfo = 1;

	bPaint = TRUE;
	InvalidateRect(hViewWnd, NULL, FALSE);
}

BOOL BrFailedReturn(BOOL br, BOOL bexit)
{
	HRESULT hr;

	if (br)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		HrFailedReturn(hr, szInfo, 256);

		if (bexit)
			MessageBox(hMainWnd, szInfo, _T("PhotoViewer"), MB_ICONERROR);
		else
			ShowFailedInfo();

		return TRUE;
	}

	return FALSE;
}