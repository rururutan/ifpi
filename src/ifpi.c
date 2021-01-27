/*
**  Pi to DIB filter for Susie32
**
**  Copyright(C) 1999-2006 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
*/

#define STRICT			/* enable strict type checking */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "spibase.h"
#include "pilib.h"

#define IFPI_VERSION	"0.21"		/* バージョン */


	/* グローバル変数 */
const int NumInfo = 4;			/* プラグイン情報の情報数 */
const LPCSTR PluginInfo[] = {	/* プラグイン情報 */
	"00IN",
	"Pi to DIB filter ver." IFPI_VERSION " (C) Miyasaka, Masaru",
	"*.pi",
	"Pi"
};


	/* プロトタイプ宣言 */
int IsSupportedFormat(LPBYTE, DWORD, LPSTR);
int GetImageInfo(SPI_FILE *, PictureInfo *);
int GetImage(SPI_FILE *, HANDLE *, HANDLE *, SPIPROC, LONG_PTR);
static void pi_read_init_io(pi_structp, SPI_FILE *);
static pi_size_t pi_read_filbuf_spi(pi_structp);



/*
**		.pi ファイルかどうかチェックする
*/
int IsSupportedFormat(LPBYTE buf, DWORD rbytes, LPSTR filename)
{
	if (rbytes < 18) return FALSE;	/* Piファイルの(最小)ヘッダサイズ */
	return (buf[0] == 'P') && (buf[1] == 'i');
}


/*
**		.pi ファイルの各種情報を得る
*/
int GetImageInfo(SPI_FILE *fp, PictureInfo *lpInfo)
{
	pi_struct pi;
#ifdef PI_READ_SUPPORT_COMMENT
	pi_bytep text;
#endif
	pi_read_init(&pi);
	pi_read_init_io(&pi, fp);
	pi_read_header(&pi, 0);
	if (pi.error != PI_OK) goto error;

#ifdef PI_READ_SUPPORT_EXTINFO
	SpiSetPictureInfo(lpInfo, pi.width, pi.height, pi.bitdepth, pi.aspect_x,
	                  pi.aspect_y, pi.offset_x, pi.offset_y, NULL);
#else
	SpiSetPictureInfo(lpInfo, pi.width, pi.height, pi.bitdepth, pi.aspect_x,
	                  pi.aspect_y, 0, 0, NULL);
#endif
#ifdef PI_READ_SUPPORT_COMMENT
	if (pi.comment.size != 0) {
		pi.error = SpiAllocBuffer(&lpInfo->hInfo, &text, pi.comment.size + 1);
		if (pi.error != SPI_ERROR_SUCCESS) goto error;
		pi_read_get_comment(&pi, (pi_charp)text, TRUE);
		SpiUnlockBuffer(&lpInfo->hInfo);
	}
#endif
error:
	pi_read_end(&pi);

	return pi.error;
}


/*
**		.pi ファイルの画像を展開する
*/
int GetImage(SPI_FILE *fp, HANDLE *pHBInfo, HANDLE *pHBImg,
             SPIPROC lpProgCallback, LONG_PTR lData)
{
	enum { NCALL = 64 };	/* コールバック関数を呼ぶ回数 */
	pi_struct pi;
	LPBITMAPINFO lpbmi;
	LPBYTE lpbits, lprow;
	DWORD rowbytes;
	int prog, prev;

	pi_read_init(&pi);
	pi_read_init_io(&pi, fp);
	pi_read_header(&pi, PI_FLAG_NO_COMMENT);
	if (pi.error != PI_OK) goto error;

	pi.error = SpiInitBitmap(pHBInfo, &lpbmi, pHBImg, &lpbits, &rowbytes,
	                         pi.width, pi.height, pi.bitdepth, pi.colors,
	                         pi.aspect_x, pi.aspect_y);
	if (pi.error != SPI_ERROR_SUCCESS) goto error;

	pi_read_palette(&pi, lpbmi->bmiColors);
	pi_read_row_init(&pi);

	lprow = lpbits + rowbytes * pi.height;
	prev = -1;
	do {
		if (lpProgCallback != NULL &&
		    (prog = pi.rownum * NCALL / pi.height) != prev &&
		    lpProgCallback((prev = prog), NCALL, lData)) {
			pi.error = SPI_ERROR_CANCEL_EXPAND;
			break;
		}
	} while (pi_read_row(&pi, (lprow -= rowbytes)));

	if (pi.error == PI_OK) {
		SpiUnlockBuffer(pHBInfo);
		SpiUnlockBuffer(pHBImg);
	} else {
		SpiFreeBuffer(pHBInfo);
		SpiFreeBuffer(pHBImg);
	}
error:
	pi_read_end(&pi);

	return pi.error;
}


/*
**		データ読み込みの準備
*/
static void pi_read_init_io(pi_structp pi_ptr, SPI_FILE *fp)
{
	pi_ptr->iofunc = pi_read_filbuf_spi;
	pi_ptr->ioptr  = fp;
	pi_ptr->iobufptr = fp->mptr;
	pi_ptr->iobufcnt = fp->mcount;
}


/*
**		データ読み込み関数
*/
static pi_size_t pi_read_filbuf_spi(pi_structp pi_ptr)
{
	SPI_FILE *fp = pi_ptr->ioptr;

	SpiFillBuf(fp);

	pi_ptr->iobufptr = fp->mptr;
	pi_ptr->iobufcnt = fp->mcount;

	if (SpiIsError(fp))
		PI_SETERR_RETURN_VAL(pi_ptr, PI_ERR_FILE_READ, pi_ptr->iobufcnt);

	return pi_ptr->iobufcnt;
}

