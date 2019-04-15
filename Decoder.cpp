#include "MainWnd.h"


#ifdef _WIC
void wicReleaseBitmap(BMP *pbmp, BOOL bexit);
HRESULT wicLoadBitmapFromFile(BMP *pbmp, TCHAR *pszfile);
HRESULT wicGetBitmap(BMP *pbmp);
HRESULT wicBitmapDecoder(BMP *pbmp);
HRESULT wicBitmapFrameDecode(BMP *pbmp);
HRESULT wicBitmapFormatConverter(BMP *pbmp);
HRESULT wicBitmapFlipRotator(BMP *pbmp);
HRESULT wicBitmapDecoderMetadata(BMP *pbmp);
HRESULT wicBitmapFrameDecodeMetadata(BMP *pbmp);
void wicGetFormat(BMP *pbmp);
void wicGetBitDepth(BMP *pbmp);
BOOL wicCreateBitmapDC(BMP *pbmp, UINT uw, UINT uh);
BOOL wicFinishBitmapDC(BMP *pbmp);


void wicReleaseBitmap(BMP *pbmp, BOOL bexit)
{
	if (pbmp->hdc)
	{
		DeleteDC(pbmp->hdc);
		pbmp->hdc = NULL;
	}
	if (pbmp->pidec)
	{
		pbmp->pidec->Release();
		pbmp->pidec = NULL;
	}
	if (bexit)
	{
		if (pbmp->pifty)
			pbmp->pifty->Release();
		CoUninitialize();
	}
}

HRESULT wicLoadBitmapFromFile(BMP *pbmp, TCHAR *pszfile)
{
	HRESULT hr;

	if (!pbmp->pifty)
	{
		hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
			return hr;

		hr = CoCreateInstance(CLSID_WICImagingFactory, 0, CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory, (LPVOID *)&pbmp->pifty);
		if (FAILED(hr))
			return hr;
	}

	hr = pbmp->pifty->CreateDecoderFromFilename(pszfile, NULL, GENERIC_READ,
		WICDecodeMetadataCacheOnDemand, &pbmp->pidec);
	if (FAILED(hr))
		return hr;

	hr = wicBitmapDecoder(pbmp);

	return hr;
}

HRESULT wicGetBitmap(BMP *pbmp)
{
	HRESULT hr;

	hr = wicBitmapFrameDecode(pbmp);
	if (SUCCEEDED(hr))
		hr = wicBitmapFormatConverter(pbmp);

	if (SUCCEEDED(hr))
		hr = wicBitmapFlipRotator(pbmp);

	if (pbmp->hbmpdc)
	{
		DeleteDC(pbmp->hbmpdc);
		pbmp->hbmpdc = NULL;
	}
	if (pbmp->piflip)
	{
		pbmp->piflip->Release();
		pbmp->piflip = NULL;
	}
	if (pbmp->piconv)
	{
		pbmp->piconv->Release();
		pbmp->piconv = NULL;
	}
	if (pbmp->pifrm)
	{
		pbmp->pifrm->Release();
		pbmp->pifrm = NULL;
	}

	return hr;
}

HRESULT wicBitmapDecoder(BMP *pbmp)
{
	HRESULT hr;

	hr = pbmp->pidec->GetContainerFormat(&pbmp->g);
	if (FAILED(hr))
		return hr;
	wicGetFormat(pbmp);

	hr = pbmp->pidec->GetFrameCount(&pbmp->ucount);
	if (FAILED(hr))
		return hr;

	if (pbmp->bgif)
		hr = wicBitmapDecoderMetadata(pbmp);

	return hr;
}

HRESULT wicBitmapFrameDecode(BMP *pbmp)
{
	HRESULT hr;

	hr = pbmp->pidec->GetFrame(pbmp->uindex, &pbmp->pifrm);
	if (FAILED(hr))
		return hr;

	hr = pbmp->pifrm->GetPixelFormat(&pbmp->g);
	if (FAILED(hr))
		return hr;
	wicGetBitDepth(pbmp);

	if (pbmp->bgif)
	{
		if (pbmp->ucount > 1)
			hr = wicBitmapFrameDecodeMetadata(pbmp);
	}
	else
	{
		hr = pbmp->pifrm->GetSize(&pbmp->uwidth, &pbmp->uheight);
	}

	return hr;
}

HRESULT wicBitmapFormatConverter(BMP *pbmp)
{
	HRESULT hr;

	hr = pbmp->pifty->CreateFormatConverter(&pbmp->piconv);
	if (FAILED(hr))
		return hr;

	hr = pbmp->piconv->Initialize(pbmp->pifrm, GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);

	return hr;
}

HRESULT wicBitmapFlipRotator(BMP *pbmp)
{
	HRESULT hr;
	UINT uw;
	UINT uh;
	UINT cbstde;
	UINT cbsize;
	BOOL br;
	int r;

	hr = pbmp->pifty->CreateBitmapFlipRotator(&pbmp->piflip);
	if (FAILED(hr))
		return hr;

	r = pbmp->r;
	if (r < 0)
		r += 4;
	hr = pbmp->piflip->Initialize(pbmp->piconv, (WICBitmapTransformOptions)r);
	if (FAILED(hr))
		return hr;

	hr = pbmp->piflip->GetSize(&uw, &uh);
	if (FAILED(hr))
		return hr;

	hr = UIntMult(uw, sizeof(DWORD), &cbstde);
	if (FAILED(hr))
		return hr;

	hr = UIntMult(cbstde, uh, &cbsize);
	if (FAILED(hr))
		return hr;

	br = wicCreateBitmapDC(pbmp, uw, uh);
	if (!br)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (SUCCEEDED(hr))
			hr = E_FAIL;
		return hr;
	}

	hr = pbmp->piflip->CopyPixels(NULL, cbstde, cbsize, (BYTE *)pbmp->lpbit);
	if (FAILED(hr))
		return hr;

	br = wicFinishBitmapDC(pbmp);
	if (!br)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if (SUCCEEDED(hr))
			hr = E_FAIL;
		return hr;
	}

	return S_OK;
}

HRESULT wicBitmapDecoderMetadata(BMP *pbmp)
{
	HRESULT hr;
	IWICMetadataQueryReader *pimqr = NULL;
	PROPVARIANT var;
	FLOAT fpar;

	PropVariantInit(&var);
	hr = pbmp->pidec->GetMetadataQueryReader(&pimqr);
	if(FAILED(hr))
		goto end;

	hr = pimqr->GetMetadataByName(L"/logscrdesc/Width", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->uwidth = var.uiVal;

	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/logscrdesc/Height", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->uheight = var.uiVal;
	
	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI1) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	if (var.bVal)
	{
		fpar = (var.bVal + 15.f) / 64.f;
		if (fpar > 1.f)
			pbmp->uheight = (UINT)(pbmp->uheight / fpar);
		else
			pbmp->uwidth = (UINT)(pbmp->uwidth * fpar);
	}

end:
	PropVariantClear(&var);
	if (pimqr)
		pimqr->Release();

	return hr;
}

HRESULT wicBitmapFrameDecodeMetadata(BMP *pbmp)
{
	HRESULT hr;
	IWICMetadataQueryReader *pimqr = NULL;
	PROPVARIANT var;

	PropVariantInit(&var);
	hr = pbmp->pifrm->GetMetadataQueryReader(&pimqr);
	if (FAILED(hr))
		goto end;
	
	hr = pimqr->GetMetadataByName(L"/imgdesc/Left", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->gif2.x = var.uiVal;
	
	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/imgdesc/Top", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->gif2.y = var.uiVal;
	
	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/imgdesc/Width", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->gif2.cx = pbmp->gif2.x + var.uiVal;
	
	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/imgdesc/Height", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI2) ? S_OK : E_FAIL;
	if(FAILED(hr))
		goto end;
	pbmp->gif2.cy = pbmp->gif2.y + var.uiVal;

	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/grctlext/Delay", &var);
	if(FAILED(hr))
		goto end;
	if (var.vt == VT_UI2)
		pbmp->gif2.xx = var.uiVal * 10;
	else
		pbmp->gif2.xx = 0;
	
	PropVariantClear(&var);
	hr = pimqr->GetMetadataByName(L"/grctlext/Disposal", &var);
	if(FAILED(hr))
		goto end;
	hr = (var.vt == VT_UI1) ? S_OK : E_FAIL;
	if (FAILED(hr))
		goto end;
	pbmp->gif2.yy = var.bVal;

end:
	PropVariantClear(&var);
	if (pimqr)
		pimqr->Release();

	return hr;
}

void wicGetFormat(BMP *pbmp)
{
	pbmp->bgif = FALSE;
	switch (pbmp->g.Data4[7])
	{
	case 0xe3:
		pbmp->lpfmt = _T("BMP");
		break;
	case 0xaf:
		pbmp->lpfmt = _T("PNG");
		break;
	case 0x21:
		pbmp->lpfmt = _T("ICO");
		break;
	case 0x57:
		pbmp->lpfmt = _T("JPEG");
		break;
	case 0xa3:
		pbmp->lpfmt = _T("TIFF");
		break;
	case 0xa5:
		pbmp->bgif = TRUE;
		pbmp->lpfmt = _T("GIF");
		break;
	case 0x4b:
		pbmp->lpfmt = _T("WMP");
		break;
	default:
		pbmp->lpfmt = _T("???");
		break;
	}
}

void wicGetBitDepth(BMP *pbmp)
{
	switch (pbmp->g.Data4[7])
	{
	case 0x01:
	case 0x05:
		pbmp->ubit = 1;
		break;
	case 0x02:
	case 0x06:
		pbmp->ubit = 2;
		break;
	case 0x03:
	case 0x07:
		pbmp->ubit = 4;
		break;
	case 0x04:
	case 0x08:
		pbmp->ubit = 8;
		break;
	case 0x13:
		if (pbmp->g.Data4[6] == 0x43)
		{
			pbmp->ubit = 48;
			break;
		}
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x3e:
		pbmp->ubit = 16;
		break;
	case 0x0c:
	case 0x0d:
	case 0x20:
		pbmp->ubit = 24;
		break;
	case 0x0e:
	case 0x0f:
	case 0x10:
	case 0x11:
	case 0x14:
	case 0x1c:
	case 0x21:
	case 0x2e:
	case 0x3d:
	case 0x3f:
	case 0x95:
	case 0xe9:
	case 0xba:
	case 0xe0:
		pbmp->ubit = 32;
		break;
	case 0x2c:
		if (pbmp->g.Data4[6] == 0x12)
		{
			pbmp->ubit = 32;
			break;
		}
	case 0x22:
	case 0x2f:
		pbmp->ubit = 40;
		break;
	case 0x30:
		if (pbmp->g.Data4[6] == 0x55)
		{
			pbmp->ubit = 64;
			break;
		}
	case 0x12:
	case 0x15:
	case 0x23:
	case 0x26:
	case 0x3b:
		pbmp->ubit = 48;
		break;
	case 0x24:
	case 0x31:
		pbmp->ubit = 56;
		break;
	case 0x16:
	case 0x17:
	case 0x1d:
	case 0x1f:
	case 0x25:
	case 0x27:
	case 0x32:
	case 0x34:
	case 0x3a:
	case 0x40:
	case 0x42:
	case 0x46:
		pbmp->ubit = 64;
		break;
	case 0x33:
		pbmp->ubit = 72;
		break;
	case 0x2d:
		if (pbmp->g.Data4[6] == 0xd4)
		{
			pbmp->ubit = 64;
			break;
		}
	case 0x28:
	case 0x35:
		pbmp->ubit = 80;
		break;
	case 0x18:
	case 0x29:
	case 0x36:
		pbmp->ubit = 96;
		break;
	case 0x2a:
		if (pbmp->g.Data4[6] == 0x53)
		{
			pbmp->ubit = 48;
			break;
		}
	case 0x37:
		pbmp->ubit = 112;
		break;
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1e:
	case 0x2b:
	case 0x38:
	case 0x41:
		pbmp->ubit = 128;
		break;
	case 0x39:
		pbmp->ubit = 144;
		break;
	default:
		pbmp->ubit = 0;
		break;
	}
}

BOOL wicCreateBitmapDC(BMP *pbmp, UINT uw, UINT uh)
{
	BOOL br = FALSE;
	HDC hdc = NULL;
	HBITMAP hbmp = NULL;
	BITMAPINFO bmi;
	HGDIOBJ hobj;

	hdc = GetDC(NULL);
	if (!hdc)
		goto end;

	pbmp->hbmpdc = CreateCompatibleDC(hdc);
	if (!pbmp->hbmpdc)
		goto end;

	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = (LONG)uw;
	bmi.bmiHeader.biHeight = -(LONG)uh;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pbmp->lpbit, NULL, 0);
	if (!hbmp)
		goto end;

	hobj = SelectObject(pbmp->hbmpdc, hbmp);
	if (hobj == NULL || hobj == HGDI_ERROR)
		goto end;

	br = TRUE;

end:
	if (hbmp)
		DeleteObject(hbmp);
	if (hdc)
		ReleaseDC(NULL, hdc);

	return br;
}

BOOL wicFinishBitmapDC(BMP *pbmp)
{
	BOOL br;
	RECT rc;
	WICRect wrc;
	BLENDFUNCTION ftn;

	if (pbmp->uindex == 0)
	{
		pbmp->gif.x = 0;
		pbmp->gif.y = 0;
		pbmp->gif.cx = pbmp->uwidth;
		pbmp->gif.cy = pbmp->uheight;
		pbmp->gif.yy = 0;
	}

	if (pbmp->ucount == 1)
	{
		pbmp->gif2.x = 0;
		pbmp->gif2.y = 0;
		pbmp->gif2.cx = pbmp->uwidth;
		pbmp->gif2.cy = pbmp->uheight;
		pbmp->gif2.yy = 0;
	}

	switch (pbmp->r)
	{
	case 1:
	case -3:
		rc.left		= pbmp->uheight - pbmp->gif.cy;
		rc.top		= pbmp->gif.x;
		rc.right	= pbmp->uheight - pbmp->gif.y;
		rc.bottom	= pbmp->gif.cx;
		wrc.X		= pbmp->uheight - pbmp->gif2.cy;
		wrc.Y		= pbmp->gif2.x;
		wrc.Width	= pbmp->uheight - pbmp->gif2.y;
		wrc.Height	= pbmp->gif2.cx;
		pbmp->w		= pbmp->uheight;
		pbmp->h		= pbmp->uwidth;
		break;
	case 3:
	case -1:
		rc.left		= pbmp->gif.y;
		rc.top		= pbmp->uwidth - pbmp->gif.cx;
		rc.right	= pbmp->gif.cy;
		rc.bottom	= pbmp->uwidth - pbmp->gif.x;
		wrc.X		= pbmp->gif2.y;
		wrc.Y		= pbmp->uwidth - pbmp->gif2.cx;
		wrc.Width	= pbmp->gif2.cy;
		wrc.Height	= pbmp->uwidth - pbmp->gif2.x;
		pbmp->w		= pbmp->uheight;
		pbmp->h		= pbmp->uwidth;
		break;
	case 2:
	case -2:
		rc.left		= pbmp->uwidth - pbmp->gif.cx;
		rc.top		= pbmp->uheight - pbmp->gif.cy;
		rc.right	= pbmp->uwidth - pbmp->gif.x;
		rc.bottom	= pbmp->uheight - pbmp->gif.y;
		wrc.X		= pbmp->uwidth - pbmp->gif2.cx;
		wrc.Y		= pbmp->uheight - pbmp->gif2.cy;
		wrc.Width	= pbmp->uwidth - pbmp->gif2.x;
		wrc.Height	= pbmp->uheight - pbmp->gif2.y;
		pbmp->w		= pbmp->uwidth;
		pbmp->h		= pbmp->uheight;
		break;
	default:
		rc.left		= pbmp->gif.x;
		rc.top		= pbmp->gif.y;
		rc.right	= pbmp->gif.cx;
		rc.bottom	= pbmp->gif.cy;
		wrc.X		= pbmp->gif2.x;
		wrc.Y		= pbmp->gif2.y;
		wrc.Width	= pbmp->gif2.cx;
		wrc.Height	= pbmp->gif2.cy;
		pbmp->w		= pbmp->uwidth;
		pbmp->h		= pbmp->uheight;
		break;
	}

	if (pbmp->bgif)
	{
		if (pbmp->uindex == 0)
		{
			br = CreateCanvasDC(&pbmp->hdc, NULL, pbmp->w, pbmp->h);
			if (!br)
				return FALSE;
		}

		if (pbmp->gif.yy == 2)
		{
			br = FillRect(pbmp->hdc, &rc, NULL);
			if (!br)
				return FALSE;
		}

		if (pbmp->ucount > 1)
		{
			pbmp->gif.x = pbmp->gif2.x;
			pbmp->gif.y = pbmp->gif2.y;
			pbmp->gif.cx = pbmp->gif2.cx;
			pbmp->gif.cy = pbmp->gif2.cy;
			pbmp->gif.yy = pbmp->gif2.yy;
		}
		else
			pbmp->gif2.xx = 0;
	}
	else
	{
		pbmp->gif2.xx = 0;
		wrc.X = 0;
		wrc.Y = 0;
		wrc.Width = pbmp->w;
		wrc.Height = pbmp->h;
		br = CreateCanvasDC(&pbmp->hdc, pbmp->hbr, pbmp->w, pbmp->h);
		if (!br)
			return FALSE;
	}

	wrc.Width -= wrc.X;
	wrc.Height -= wrc.Y;
	ftn.AlphaFormat = AC_SRC_ALPHA;
	ftn.BlendFlags = 0;
	ftn.BlendOp = 0;
	ftn.SourceConstantAlpha = 255;
	br = GdiAlphaBlend(pbmp->hdc, wrc.X, wrc.Y, wrc.Width, wrc.Height,
		pbmp->hbmpdc, 0, 0, wrc.Width, wrc.Height, ftn);

	return br;
}

#else

void gpReleaseBitmap(BMP *pbmp, BOOL bexit)
{
	if (pbmp->hdc)
	{
		DeleteDC(pbmp->hdc);
		pbmp->hdc = NULL;
	}
	if (pbmp->pgp)
	{
		GdipDisposeImage(pbmp->pgp);
		pbmp->pgp = NULL;
	}
	if (bexit)
	{
		if (pbmp->token)
			GdiplusShutdown(pbmp->token);
		CoUninitialize();
	}
}

GpStatus gpLoadBitmapFromFile(BMP *pbmp, TCHAR *pszfile)
{
	GpStatus s;
	GdiplusStartupInput input;
	GdiplusStartupOutput output;
	PixelFormat fmt;
	HRESULT hr;

	if (!pbmp->token)
	{
		hr = CoInitializeEx(0,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
			return GenericError;

		input.DebugEventCallback = NULL;
		input.GdiplusVersion = 1;
		input.SuppressBackgroundThread = FALSE;
		input.SuppressExternalCodecs = FALSE;
		s = GdiplusStartup(&pbmp->token, &input, &output);
		if (s != Ok)
			return s;
	}

	s = GdipLoadImageFromFile(pszfile, &pbmp->pgp);
	if (s != Ok)
		return s;

	s = GdipGetImagePixelFormat(pbmp->pgp, &fmt);
	if (s != Ok)
		return s;
	pbmp->ubit = GetPixelFormatSize(fmt);

	s = GdipGetImageRawFormat(pbmp->pgp, &pbmp->g);
	if (s != Ok)
		return s;

	pbmp->bgif = FALSE;
	switch (pbmp->g.Data1)
	{
	case 0xb96b3caa:
		pbmp->lpfmt = _T("MemBMP");
		break;
	case 0xb96b3cab:
		pbmp->lpfmt = _T("BMP");
		break;
	case 0xb96b3cac:
		pbmp->lpfmt = _T("EMF");
		break;
	case 0xb96b3cad:
		pbmp->lpfmt = _T("WMF");
		break;
	case 0xb96b3cae:
		pbmp->lpfmt = _T("JPEG");
		break;
	case 0xb96b3caf:
		pbmp->lpfmt = _T("PNG");
		break;
	case 0xb96b3cb0:
		pbmp->bgif = TRUE;
		pbmp->ubit = 8;
		pbmp->lpfmt = _T("GIF");
		break;
	case 0xb96b3cb1:
		pbmp->lpfmt = _T("TIFF");
		break;
	case 0xb96b3cb2:
		pbmp->lpfmt = _T("EXIF");
		break;
	case 0xb96b3cb5:
		pbmp->lpfmt = _T("ICO");
		break;
	default:
		pbmp->lpfmt = _T("???");
		break;
	}
	
	s = GdipImageGetFrameDimensionsList(pbmp->pgp, &pbmp->g, 1);
	if (s != Ok)
		return s;

	s = GdipImageGetFrameCount(pbmp->pgp, &pbmp->g, &pbmp->ucount);

	return s;
}

GpStatus gpGetBitmap(BMP *pbmp)
{
	GpStatus s;
	GpGraphics *pgh;
	PropertyItem *pitem;
	UINT usize;
	BOOL br;
	int r;

	s = GdipImageSelectActiveFrame(pbmp->pgp, &pbmp->g, pbmp->uindex);
	if (s != Ok)
		return s;

	s = GdipGetImageWidth(pbmp->pgp, &pbmp->uwidth);
	if (s != Ok)
		return s;
	pbmp->w = pbmp->uwidth;
	
	s = GdipGetImageHeight(pbmp->pgp, &pbmp->uheight);
	if (s != Ok)
		return s;
	pbmp->h = pbmp->uheight;

	pbmp->gif2.xx = 0;
	if (pbmp->bgif)
	{
		pbmp->r = 0;

		if (pbmp->ucount > 1)
		{
			s = GdipGetPropertyItemSize(pbmp->pgp,
				PropertyTagFrameDelay, &usize);
			if (s != Ok)
				return s;

			pitem = (PropertyItem *)malloc(usize);
			if (!pitem)
				return Win32Error;

			s = GdipGetPropertyItem(pbmp->pgp,
				PropertyTagFrameDelay, usize, pitem);
			if (s != Ok)
			{
				free(pitem);
				return s;
			}

			pbmp->gif2.xx = ((int *)pitem->value)[pbmp->uindex] * 10;
			free(pitem);
		}
	}
	else
	{
		r = pbmp->r;
		if (r < 0)
			r += 4;
		s = GdipImageRotateFlip(pbmp->pgp, (RotateFlipType)r);
		if (s != Ok)
			return s;

		if (r == 1 || r == 3)
		{
			pbmp->w = pbmp->uheight;
			pbmp->h = pbmp->uwidth;
			r = 4 - r;
		}
	}

	br = CreateCanvasDC(&pbmp->hdc, pbmp->bgif ? NULL : pbmp->hbr,
		pbmp->w, pbmp->h);
	if (!br)
		return Win32Error;

	s = GdipCreateFromHDC(pbmp->hdc, &pgh);
	if (s != Ok)
		return s;

	s = GdipDrawImageRectI(pgh, pbmp->pgp, 0, 0, pbmp->w, pbmp->h);
	if (s == Ok)
	{
		if (!pbmp->bgif)
			s = GdipImageRotateFlip(pbmp->pgp, (RotateFlipType)r);
	}

	if (pgh)
		GdipDeleteGraphics(pgh);

	return s;
}

void gpFailedReturn(GpStatus s, TCHAR *pszinfo, int cchinfo)
{
	TCHAR *psz;

	switch (s)
	{
	case Ok:
		psz = _T("Ok");
		break;
	case GenericError:
		psz = _T("GenericError");
		break;
	case InvalidParameter:
		psz = _T("InvalidParameter");
		break;
	case OutOfMemory:
		psz = _T("OutOfMemory");
		break;
	case ObjectBusy:
		psz = _T("ObjectBusy");
		break;
	case InsufficientBuffer:
		psz = _T("InsufficientBuffer");
		break;
	case NotImplemented:
		psz = _T("NotImplemented");
		break;
	case Win32Error:
		psz = _T("Win32Error");
		break;
	case WrongState:
		psz = _T("WrongState");
		break;
	case Aborted:
		psz = _T("Aborted");
		break;
	case FileNotFound:
		psz = _T("FileNotFound");
		break;
	case ValueOverflow:
		psz = _T("ValueOverflow");
		break;
	case AccessDenied:
		psz = _T("AccessDenied");
		break;
	case UnknownImageFormat:
		psz = _T("UnknownImageFormat");
		break;
	case FontFamilyNotFound:
		psz = _T("FontFamilyNotFound");
		break;
	case FontStyleNotFound:
		psz = _T("FontStyleNotFound");
		break;
	case NotTrueTypeFont:
		psz = _T("NotTrueTypeFont");
		break;
	case UnsupportedGdiplusVersion:
		psz = _T("UnsupportedGdiplusVersion");
		break;
	case GdiplusNotInitialized:
		psz = _T("GdiplusNotInitialized");
		break;
	case PropertyNotFound:
		psz = _T("PropertyNotFound");
		break;
	case PropertyNotSupported:
		psz = _T("PropertyNotSupported");
		break;
	default:
		psz = _T("(Unknown error!)");
		break;
	}

	StringCchCopy(pszinfo, cchinfo, _T("Error (Gdiplus) : "));
	StringCchCat(pszinfo, cchinfo - 19, psz);
}
#endif

BOOL CreateCanvasDC(HDC *phdc, HBRUSH hbr, int w, int h)
{
	BOOL br = FALSE;
	HDC hdc = NULL;
	HBITMAP hbmp = NULL;
	HGDIOBJ hobj;
	RECT rc;

	if (*phdc)
	{
		DeleteDC(*phdc);
		*phdc = NULL;
	}

	hdc = GetDC(NULL);
	if (!hdc)
		goto end;

	*phdc = CreateCompatibleDC(hdc);
	if (!*phdc)
		goto end;

	hbmp = CreateCompatibleBitmap(hdc, w, h);
	if (!hbmp)
		goto end;

	hobj = SelectObject(*phdc, hbmp);
	if (hobj == NULL || hobj == HGDI_ERROR)
		goto end;

	rc.left = 0;
	rc.top = 0;
	rc.right = w;
	rc.bottom = h;
	br = FillRect(*phdc, &rc, hbr);

end:
	if (hbmp)
		DeleteObject(hbmp);
	if (hdc)
		ReleaseDC(NULL, hdc);

	return br;
}

void HrFailedReturn(HRESULT hr, TCHAR *pszinfo, int cchinfo)
{
	StringCchPrintf(pszinfo, cchinfo,
			_T("Error (0x%08x) : (Can not found error info!)"), hr);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, hr, 0,
		pszinfo + 21, cchinfo - 22, 0);
}

void ReleaseBitmap(BMP *pbmp, BOOL bexit)
{
#ifdef _WIC
	wicReleaseBitmap(pbmp, bexit);
#else
	gpReleaseBitmap(pbmp, bexit);
#endif
}

BOOL LoadBitmapFromFile(BMP *pbmp, TCHAR *pszfile, TCHAR *pszinfo, int cchinfo)
{
#ifdef _WIC
	HRESULT hr;

	hr = wicLoadBitmapFromFile(pbmp, pszfile);
	if (FAILED(hr))
	{
		HrFailedReturn(hr, pszinfo, cchinfo);
		return FALSE;
	}

	return TRUE;
#else
	GpStatus s;

	s = gpLoadBitmapFromFile(pbmp, pszfile);
	if (s != Ok)
	{
		gpFailedReturn(s, pszinfo, cchinfo);
		return FALSE;
	}

	return TRUE;
#endif
}

BOOL GetBitmap(BMP *pbmp, TCHAR *pszinfo, int cchinfo)
{
#ifdef _WIC
	HRESULT hr;

	hr = wicGetBitmap(pbmp);
	if (FAILED(hr))
	{
		HrFailedReturn(hr, pszinfo, cchinfo);
		return FALSE;
	}

	return TRUE;
#else
	GpStatus s;

	s = gpGetBitmap(pbmp);
	if (s != Ok)
	{
		gpFailedReturn(s, pszinfo, cchinfo);
		return FALSE;
	}

	return TRUE;
#endif
}