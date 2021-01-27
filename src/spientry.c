/*
**  Susie import filter plug-in
**  spientry.c - API layer functions
**
**  Public domain by MIYASAKA Masaru (Nov 23, 2004)
*/

#define STRICT					/* enable strict type checking */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

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

	if (infono < 0 || infono >= NumInfo) return 0;

	str = PluginInfo[infono];
	for (n = 0; n < buflen && (*(buf++) = *(str++)) != '\0'; n++) ;
	/*
	 * Susie Plug-in の仕様上では、文字列の最後に '\0' は
	 * 必要ないようだが、Plug-in を利用する他のソフトの中には、
	 * 文字列の最後に '\0' が存在するということを前提にして
	 * 設計されているものもあるようである。
	 * だから、バッファに余裕がある場合は、最後に '\0' を
	 * 付けておいた方が良い。
	 */

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
 IsSupported(LPSTR filename, DWORD dw)
{
	BYTE buf[2048];
	LPBYTE pbuf;
	DWORD rbytes;

	if (dw & 0xFFFF0000) {
		rbytes = 2048;
		pbuf = (LPBYTE)dw;
	} else {
		if (!ReadFile((HANDLE)dw,buf,sizeof(buf),&rbytes,NULL)) return FALSE;
		pbuf = buf;
	}

	return IsSupportedFormat(pbuf, rbytes, filename);
}


/*
** ----- 画像ファイルに関する情報を得る ----------------------------------
*/
int _export PASCAL
 GetPictureInfo(LPSTR buf, long len, unsigned int flag, PictureInfo *lpInfo)
{
	SPI_FILE f;
	int err;

	err = SpiOpen(&f, buf, len, flag);
	if (err != SPI_ERROR_SUCCESS) return err;

	err = GetImageInfo(&f, lpInfo);

	SpiClose(&f);

	return err;
}


/*
** ----- 画像を展開する --------------------------------------------------
*/
int _export PASCAL
 GetPicture(LPSTR buf, long len, unsigned int flag, HANDLE *pHBInfo,
            HANDLE *pHBm, FARPROC lpProgCallback, long lData)
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


/*
** ----- プレビュー・カタログ表示用画像縮小展開ルーティン ----------------
*/
int _export PASCAL
 GetPreview(LPSTR buf, long len, unsigned int flag, HANDLE *pHBInfo,
            HANDLE *pHBm, FARPROC lpProgCallback, long lData)
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

