/*
**  Susie import filter plug-in
**  spientry.c - API layer functions
**
**  Public domain by MIYASAKA Masaru (Nov 23, 2004)
*/

#define STRICT					/* enable strict type checking */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wchar.h>
#include <stdio.h>

#include "spibase.h"



#ifdef SPI_IMPLEMENT_DLLMAIN

	/* グローバル変数 */
#ifdef SPI_IMPLEMENT_DLLHANDLE
HINSTANCE ghThisInst = NULL;	/* Handle to the DLL's instance. */
#endif

/*
** ----- Entry point for DLL loading and unloading -----------------------
*/
BOOL WINAPI
 DllMain(HINSTANCE hInstDLL, DWORD dwNotification, LPVOID lpReserved)
{
#ifdef SPI_IMPLEMENT_DLLHANDLE
	ghThisInst = hInstDLL;
#endif
	switch (dwNotification) {
	case DLL_PROCESS_ATTACH:
#ifdef SPI_IMPLEMENT_INITOPTIONS
		InitOptions();
#endif
		break;
#if 0
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
#endif
	}
	return TRUE;
}

#endif	/* SPI_IMPLEMENT_DLLMAIN */

/*
** ----- Plug-inに関する情報を得る ---------------------------------------
*/
int _export PASCAL
 GetPluginInfo(int infono, LPSTR buf, int buflen)
{
	LPCSTR str;
	int n;

	if (buf == NULL || infono < 0 || infono >= NumInfo) {
		if (buf) buf[0] = '\0';
		return 0;
	}

	str = PluginInfo[infono];
	n = _snprintf_s(buf, buflen, _TRUNCATE, "%s", str);
	if (n > buflen) {
		n = buflen;
	}
	if (n < 0) {
		n = 0;
	}

	return n;
}

int _export PASCAL
 GetPluginInfoW(int infono, LPWSTR buf, int buflen)
{
	LPCSTR str;
	int n;

	if (buf == NULL || infono < 0 || infono >= NumInfo) {
		if (buf) buf[0] = L'\0';
		return 0;
	}

	str = PluginInfo[infono];
	n = _snwprintf_s(buf, buflen, _TRUNCATE, L"%hs", str);
	if (n > buflen) {
		n = buflen;
	}
	if (n < 0) {
		n = 0;
	}

	return n;
}

#ifdef SPI_IMPLEMENT_CONFIGDLG

/*
** ----- Plug-inの設定ダイアログ -----------------------------------------
*/
int _export PASCAL
 ConfigurationDlg(HWND hOwner, int fnc)
{
	enum {						/* fnc の機能コード */
		SPI_CFGDLG_ABOUT = 0,	/* Plug-in の about ダイアログ表示 */
		SPI_CFGDLG_SETUP = 1	/* 設定ダイアログ表示 */
	};

	switch (fnc) {
#ifdef SPI_IMPLEMENT_ABOUTDLG
	case SPI_CFGDLG_ABOUT:
		ExecAboutDialog(hOwner);
		return SPI_ERROR_SUCCESS;
#endif
	case SPI_CFGDLG_SETUP:
		ExecSetupDialog(hOwner);
		return SPI_ERROR_SUCCESS;

	default:
		;
	}

	return SPI_ERROR_NOT_IMPLEMENTED;
}

#endif	/* SPI_IMPLEMENT_CONFIGDLG */

/*
** ----- 展開可能な(対応している)ファイル形式か調べる --------------------
*/
int _export PASCAL
 IsSupported(LPCSTR filename, void* dw)
{
	BYTE buf[2048];
	LPBYTE pbuf;
	DWORD rbytes;

	if ((DWORD_PTR)dw & ~(DWORD_PTR)0xffff) {
		rbytes = 2048;
		pbuf = (LPBYTE)dw;
	} else {
		if (!ReadFile((HANDLE)dw,buf,sizeof(buf),&rbytes,NULL)) return FALSE;
		pbuf = buf;
	}

	return IsSupportedFormat(pbuf, rbytes, filename);
}

int _export PASCAL
 IsSupportedW(LPCWSTR filename, void* variant)
{
	return IsSupported(NULL, variant);
}

/*
** ----- 画像ファイルに関する情報を得る ----------------------------------
*/
int _export PASCAL
 GetPictureInfo(LPCSTR buf, LONG_PTR len, unsigned int flag, PictureInfo *lpInfo)
{
	SPI_FILE f;
	int err;

	err = SpiOpen(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

	err = GetImageInfo(&f, lpInfo);

	SpiClose(&f);

	return err;
}

int _export PASCAL
 GetPictureInfoW(LPCWSTR buf, LONG_PTR len, unsigned int flag, PictureInfo *lpInfo)
{
	SPI_FILE f;
	int err;

	err = SpiOpenW(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

	err = GetImageInfo(&f, lpInfo);

	SpiClose(&f);

	return err;
}


/*
** ----- 画像を展開する --------------------------------------------------
*/
int _export PASCAL
 GetPicture(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo,
            HLOCAL *pHBm, SUSIE_PROGRESS lpProgCallback, LONG_PTR lData)
{
	SPI_FILE f;
	int err;

	err = SpiOpen(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

#ifdef SPI_IMPLEMENT_GETPREVIEW
	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData, FALSE);
#else
	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData);
#endif

	SpiClose(&f);

	return err;
}

int _export PASCAL
GetPictureW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo,
            HLOCAL *pHBm, SUSIE_PROGRESS lpProgCallback, LONG_PTR lData)
{
	SPI_FILE f;
	int err;

	err = SpiOpenW(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

#ifdef SPI_IMPLEMENT_GETPREVIEW
	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData, FALSE);
#else
	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData);
#endif

	SpiClose(&f);

	return err;
}


/*
** ----- プレビュー・カタログ表示用画像縮小展開ルーティン ----------------
*/
int _export PASCAL
 GetPreview(LPCSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo,
            HLOCAL *pHBm, SUSIE_PROGRESS lpProgCallback, LONG_PTR lData)
{
#ifdef SPI_IMPLEMENT_GETPREVIEW
	SPI_FILE f;
	int err;

	err = SpiOpen(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData, TRUE);

	SpiClose(&f);

	return err;
#else
	return SPI_ERROR_NOT_IMPLEMENTED;
#endif
}

int _export PASCAL
 GetPreviewW(LPCWSTR buf, LONG_PTR len, unsigned int flag, HLOCAL *pHBInfo,
            HLOCAL *pHBm, SUSIE_PROGRESS lpProgCallback, LONG_PTR lData)
{
#ifdef SPI_IMPLEMENT_GETPREVIEW
	SPI_FILE f;
	int err;

	err = SpiOpenW(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

	err = GetImage(&f, pHBInfo, pHBm, (SPIPROC)lpProgCallback, lData, TRUE);

	SpiClose(&f);

	return err;
#else
	return SPI_ERROR_NOT_IMPLEMENTED;
#endif
}

