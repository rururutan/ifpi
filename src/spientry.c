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

#include "spibase.h"



#ifdef SPI_IMPLEMENT_DLLMAIN

	/* �O���[�o���ϐ� */
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
** ----- Plug-in�Ɋւ�����𓾂� ---------------------------------------
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
	for (n = 0; n < buflen && (*(buf++) = *(str++)) != '\0'; n++) ;
	/*
	 * Susie Plug-in �̎d�l��ł́A������̍Ō�� '\0' ��
	 * �K�v�Ȃ��悤�����APlug-in �𗘗p���鑼�̃\�t�g�̒��ɂ́A
	 * ������̍Ō�� '\0' �����݂���Ƃ������Ƃ�O��ɂ���
	 * �݌v����Ă�����̂�����悤�ł���B
	 * ������A�o�b�t�@�ɗ]�T������ꍇ�́A�Ō�� '\0' ��
	 * �t���Ă����������ǂ��B
	 */

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
** ----- Plug-in�̐ݒ�_�C�A���O -----------------------------------------
*/
int _export PASCAL
 ConfigurationDlg(HWND hOwner, int fnc)
{
	enum {						/* fnc �̋@�\�R�[�h */
		SPI_CFGDLG_ABOUT = 0,	/* Plug-in �� about �_�C�A���O�\�� */
		SPI_CFGDLG_SETUP = 1	/* �ݒ�_�C�A���O�\�� */
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
** ----- �W�J�\��(�Ή����Ă���)�t�@�C���`�������ׂ� --------------------
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
** ----- �摜�t�@�C���Ɋւ�����𓾂� ----------------------------------
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
** ----- �摜��W�J���� --------------------------------------------------
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
** ----- �v���r���[�E�J�^���O�\���p�摜�k���W�J���[�e�B�� ----------------
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

