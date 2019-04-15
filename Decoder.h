#pragma once


#define _WIC

#ifdef _WIC
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#else
#include <GdiPlus.h>
using namespace Gdiplus;
using namespace DllExports;
#pragma comment(lib, "gdiplus.lib")
#endif


typedef struct {
	int x;
	int y;
	int cx;
	int cy;
	int xx;
	int yy;
} POS;

typedef struct {
#ifdef _WIC
	IWICImagingFactory *pifty;
	IWICBitmapDecoder *pidec;
	IWICBitmapFrameDecode *pifrm;
	IWICFormatConverter *piconv;
	IWICBitmapFlipRotator *piflip;
	HDC hbmpdc;
#else
	ULONG_PTR token;
	GpImage *pgp;
#endif
	HDC hdc;
	HBRUSH hbr;
	GUID g;
	POS gif;
	POS gif2;
	POS pos;
	BOOL bgif;
	UINT ubit;
	UINT uindex;
	UINT ucount;
	UINT uwidth;
	UINT uheight;
	int r;
	int w;
	int h;
	LPTSTR lpfmt;
	LPVOID lpbit;
} BMP;


BOOL CreateCanvasDC(HDC *phdc, HBRUSH hbr, int w, int h);
void HrFailedReturn(HRESULT hr, TCHAR *pszinfo, int cchinfo);

void ReleaseBitmap(BMP *pbmp, BOOL bexit);
BOOL LoadBitmapFromFile(BMP *pbmp, TCHAR *pszfile, TCHAR *pszinfo,int cchinfo);
BOOL GetBitmap(BMP *pbmp, TCHAR *pszinfo, int cchinfo);
