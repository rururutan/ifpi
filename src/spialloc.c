/*
**  Susie import filter plug-in
**  spialloc.c - Memory-related functions
**
**  Public domain by MIYASAKA Masaru (Dec 15, 2002)
*/

#define STRICT					/* enable strict type checking */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "spibase.h"



/*
**		データ返却用メモリバッファを割り当てる
*/
int SpiAllocBuffer(HLOCAL *pHB, LPBYTE *pBuff, UINT uBytes)
{
	*pHB = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, uBytes);
	if (*pHB == NULL) return SPI_ERROR_ALLOCATE_MEMORY;

	if (pBuff != NULL) {
		*pBuff = LocalLock(*pHB);
		if (*pBuff == NULL) {
			SpiFreeBuffer(pHB);
			return SPI_ERROR_MEMORY;
		}
	}
	return SPI_ERROR_SUCCESS;
}

#ifdef SPI_SUPPORT_SPIREALLOC

/*
**		メモリバッファのサイズを変更する
**		Note) uBytes==0 の場合には未対応です！
*/
int SpiReAllocBuffer(HLOCAL *pHB, LPBYTE *pBuff, UINT uBytes)
{
	HLOCAL hNew;

	if (*pHB == NULL) return SpiAllocBuffer(pHB, pBuff, uBytes);

	LocalUnlock(*pHB);
	hNew = LocalReAlloc(*pHB, uBytes, LMEM_MOVEABLE | LMEM_ZEROINIT);
	if (hNew == NULL) return SPI_ERROR_ALLOCATE_MEMORY;
	*pHB = hNew;

	if (pBuff != NULL) {
		*pBuff = LocalLock(hNew);
		if (*pBuff == NULL) return SPI_ERROR_MEMORY;
	}
	return SPI_ERROR_SUCCESS;
}

#endif	/* SPI_SUPPORT_SPIREALLOC */

#ifdef SPI_ALLOCATE_ROWPOINTERS

/*
**		行ポインタ付きイメージバッファを割り当てる
*/
int SpiAllocImageBuffer(HLOCAL *pHBImg, LPBYTE **ppRowp, DWORD rowbytes,
                        DWORD height)
{
	DWORD imgsize;
	LPBYTE img;
	LPBYTE r, *p;
	int err;

	imgsize = rowbytes * height;
	err = SpiAllocBuffer(pHBImg, &img, imgsize);
	if (err != SPI_ERROR_SUCCESS) return err;

	*ppRowp = (LPBYTE *)LocalAlloc(LMEM_FIXED, sizeof(LPBYTE) * height);
	if (*ppRowp == NULL) {
		SpiFreeBuffer(pHBImg);
		return SPI_ERROR_ALLOCATE_MEMORY;
	}
	p = *ppRowp;
	r = img + imgsize;
	while (r -= rowbytes, r >= img) *(p++) = r;

	return SPI_ERROR_SUCCESS;
}

#endif	/* SPI_ALLOCATE_ROWPOINTERS */

/*
**		DIB 返却用メモリバッファの割り当てと構造体への値の設定を行なう
*/
#ifdef SPI_ALLOCATE_ROWPOINTERS
int SpiInitBitmap(HLOCAL *pHBInfo, LPBITMAPINFO *ppbmi, HLOCAL *pHBImg,
                  LPBYTE **ppRowp, DWORD width, DWORD height,
                  int clrdepth, int palettes, DWORD xdensity, DWORD ydensity)
#else
int SpiInitBitmap(HLOCAL *pHBInfo, LPBITMAPINFO *ppbmi, HLOCAL *pHBImg,
                  LPBYTE *ppBits, DWORD *lpRowb, DWORD width, DWORD height,
                  int clrdepth, int palettes, DWORD xdensity, DWORD ydensity)
#endif
{
	LPBITMAPINFOHEADER lpbmih;
	DWORD hedsize, rowbytes;
	int err;

	hedsize = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * (palettes - 1);
	err = SpiAllocBuffer(pHBInfo, (LPBYTE *)ppbmi, hedsize);
	if (err != SPI_ERROR_SUCCESS) return err;

	rowbytes = (width * clrdepth + 31) / 32 * 4;
#ifdef SPI_ALLOCATE_ROWPOINTERS			/* ↑4バイト単位へ切り上げる */
	err = SpiAllocImageBuffer(pHBImg, ppRowp, rowbytes, height);
#else
	*lpRowb = rowbytes;
	err = SpiAllocBuffer(pHBImg, ppBits, rowbytes * height);
#endif
	if (err != SPI_ERROR_SUCCESS) {
		SpiFreeBuffer(pHBInfo);
		return err;
	}
	if (clrdepth <= 8 && palettes == (1 << clrdepth)) palettes = 0;
	if (xdensity == 0 || ydensity == 0) xdensity = ydensity = 0;

	lpbmih = &(*ppbmi)->bmiHeader;
	lpbmih->biSize     = (DWORD)sizeof(BITMAPINFOHEADER);
	lpbmih->biWidth    = (LONG)width;
	lpbmih->biHeight   = (LONG)height;
	lpbmih->biPlanes   = (WORD)1;
	lpbmih->biBitCount = (WORD)clrdepth;
	lpbmih->biCompression   = BI_RGB;
	lpbmih->biSizeImage     = (DWORD)(rowbytes * height);
	lpbmih->biXPelsPerMeter = (LONG)xdensity;
	lpbmih->biYPelsPerMeter = (LONG)ydensity;
	lpbmih->biClrUsed       = (DWORD)palettes;
	lpbmih->biClrImportant  = (DWORD)0;

	return SPI_ERROR_SUCCESS;
}

