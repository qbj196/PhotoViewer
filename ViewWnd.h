#pragma once


BOOL CreateViewWnd();
void OnViewRelease(BOOL bexit);
void OnViewMenu(LPARAM lParam);
void OpenBitmapFile(TCHAR *pszfile);
BOOL BrFailedReturn(BOOL br, BOOL bexit);
