/*
**  Susie import filter plug-in
**  spiio.c - File I/O functions
**
**  Public domain by MIYASAKA Masaru (Dec 15, 2002)
*/

#define STRICT					/* enable strict type checking */
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "spibase.h"

#if defined(__RSXNT__) && !defined(CopyMemory)
#include <string.h>
#define CopyMemory(d,s,l)	memcpy((d),(s),(l))
#endif

#ifdef SPI_SUPPORT_NULLREAD
#define IsBufNULL(p)	((p) == NULL)
#else
#define IsBufNULL(p)	(0)
#endif

#if defined(SPI_SUPPORT_SPISEEK) && defined(SPI_OPTIMIZE_SPISEEK)
#define SetFilPtr(f,v)	((f)->ffilptr = (LONG)(v))
#define AdvFilPtr(f,v)	((f)->ffilptr += (LONG)(v))
#else
#define SetFilPtr(f,v)
#define AdvFilPtr(f,v)
#endif



#ifdef SPI_SUPPORT_BUFFERING

/* ***********************************************************************
**		バッファリング付き入力ストリーム関数
*/

/*
**		入力ストリームのオープン
*/
int SpiOpen(SPI_FILE *fp, LPSTR buf, long len, unsigned int flag)
{
	HANDLE hFile;
	LPBYTE pBuff;

	fp->flags = SPI_IOTYPE_NONE;

	if (buf == NULL || len < 0) return SPI_ERROR_FILE_READ;

	switch (flag & 0x07) {
	case 0:			/* 入力がディスクファイル */
		hFile = CreateFile(buf, GENERIC_READ, FILE_SHARE_READ, NULL,
		                   OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
					return SPI_ERROR_FILE_READ;
		if (len != 0)
			SetFilePointer(hFile, len, NULL, FILE_BEGIN);
		pBuff = (LPBYTE)LocalAlloc(LMEM_FIXED, SPI_BUFSIZ);
		if (pBuff == NULL) {
			CloseHandle(hFile);
			return SPI_ERROR_ALLOCATE_MEMORY;
		}
		fp->flags   = SPI_IOTYPE_FILE;
		fp->fhandle = hFile;
		fp->fname   = buf;
		fp->foffset = len;
		fp->mbuffer = pBuff;
		fp->mptr    = pBuff;
		fp->mcount  = 0;
		fp->msize   = 0;
		SetFilPtr(fp, 0);		/* fp->ffilptr = 0; */
		break;

	case 1:			/* 入力がメモリバッファ */
		fp->flags   = SPI_IOTYPE_MEMORY;
		fp->mbuffer = (LPBYTE)buf;
		fp->mptr    = (LPBYTE)buf;
		fp->mcount  = len;
		fp->msize   = len;
		break;

	default:		/* 不明な入力タイプ */
		return SPI_ERROR_NOT_IMPLEMENTED;
	}

	return SPI_ERROR_SUCCESS;
}


/*
**		入力ストリームのクローズ
*/
void SpiClose(SPI_FILE *fp)
{
	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:
		CloseHandle(fp->fhandle);
		LocalFree((HLOCAL)fp->mbuffer);
		break;

	case SPI_IOTYPE_MEMORY:
		/* Nothing to do */
		break;

	default:
		;
	}
	fp->flags = SPI_IOTYPE_NONE;
}


/*
**		入力バッファに新しいデータを読み込む
*/
void SpiFillBuf(SPI_FILE *fp)
{
	DWORD r;

	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:		/* ファイル入力 */
		if (!ReadFile(fp->fhandle,fp->mbuffer,SPI_BUFSIZ,&r,NULL)) {
			SpiSetError(fp);	/* r = 0; (Set by ReadFile) */
		} else if (r == 0) {
			SpiSetEOF(fp);
		}
		fp->mptr   = fp->mbuffer;
		fp->mcount = (LONG)r;
		fp->msize  = (LONG)r;
		AdvFilPtr(fp, r);		/* fp->ffilptr += r; */
		break;

	case SPI_IOTYPE_MEMORY:		/* メモリ入力 */
		SpiSetEOF(fp);
		if (fp->mcount > 0) {	/* Seek to EOF */
			fp->mptr   = fp->mbuffer + fp->msize;
			fp->mcount = 0;
		}
		break;

	default:					/* 入力未初期化など */
		;
	}
}

#ifdef SPI_SUPPORT_SPIGETBYTE

/*
**		SpiGetByte() 専用 データ読み込み関数
*/
INT SpiFillBufX(SPI_FILE *fp)
{
	fp->mcount++;		/* Undo decrementing of 'mcount' */
	if (SpiIsEOF(fp) || SpiIsError(fp)) return SPI_EOF;
	if (SpiFillBuf(fp), fp->mcount <= 0) return SPI_EOF;
	return fp->mcount--, *(fp->mptr++);
}

#endif	/* SPI_SUPPORT_SPIGETBYTE */

#ifdef SPI_SUPPORT_SPIREAD

/*
**		入力ストリームから指定バイト数だけデータを読む
**		SPI_SUPPORT_NULLREAD が定義されている場合には、
**		buf に NULL を指定できる(指定されたバイト数だけ空読みする)
**
**		Note) 単純にシークすると、ディスクキャッシュが有効に
**		      働かないため、特にフロッピーから読んだときに
**		      遅くなってしまう場合がある。
*/
DWORD SpiRead(LPVOID buf, DWORD rbytes, SPI_FILE *fp)
{
	LPBYTE dst = buf;
	DWORD left = rbytes;
	DWORD r;

	if (left == 0) return 0;

	for (;;) {
		if (fp->mcount > 0) {
			r = ((DWORD)fp->mcount < left) ? (DWORD)fp->mcount : left;
			if (!IsBufNULL(dst)) {
				CopyMemory(dst, fp->mptr, r);
				dst += r;
			}
			fp->mptr   += r;
			fp->mcount -= r;
			left -= r;
			if (left == 0) break;
		}
#ifdef SPI_OPTIMIZE_SPIREAD
		if (!IsBufNULL(dst) && SpiIoType(fp)==SPI_IOTYPE_FILE &&
		    left > SPI_BUFSIZ) {
			r = (left / SPI_BUFSIZ) * SPI_BUFSIZ;
			if (!ReadFile(fp->fhandle,dst,r,&r,NULL)) {
				SpiSetError(fp);	/* r = 0; (Set by ReadFile) */
				break;
			}
			if (r == 0) {
				SpiSetEOF(fp);
				break;
			}
			AdvFilPtr(fp, r);		/* fp->ffilptr += r; */
			dst  += r;
			left -= r;
			if (left == 0) break;
		}
#endif	/* ----- */
		SpiFillBuf(fp);
		if (fp->mcount <= 0) break;		/* EOF or Error */
	}

	return rbytes - left;
}

#endif	/* SPI_SUPPORT_SPIREAD */

#ifdef SPI_SUPPORT_SPISEEK

/*
**		入力ストリームをシークする
*/
LONG SpiSeek(SPI_FILE *fp, LONG offset, DWORD method)
{
	SpiClearEOF(fp);

	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:		/* ファイル入力 */
#ifdef SPI_OPTIMIZE_SPISEEK
		switch (method) {
			case FILE_CURRENT: offset -= fp->mcount;  break;
			case FILE_BEGIN:   offset -= fp->ffilptr;
			                   method = FILE_CURRENT; break;
		}
		/* 移動先がバッファの中にあるなら、ポインタを移動すればよい */
		if (method==FILE_CURRENT && -offset>=0 && -offset<=fp->msize) {
			fp->mptr   = fp->mbuffer + fp->msize + offset;
			fp->mcount = -offset;
			offset += fp->ffilptr;
			break;
		}
#else	/* ----- */
		switch (method) {
			case FILE_CURRENT: offset -= fp->mcount;  break;
			case FILE_BEGIN:   offset += fp->foffset; break;
		}
#endif	/* ----- */
		offset = (LONG)SetFilePointer(fp->fhandle, offset, NULL, method);
		if (offset != -1) {
			offset -= fp->foffset;
			SetFilPtr(fp, offset);		/* fp->ffilptr = offset; */
			fp->mptr   = fp->mbuffer;
			fp->mcount = 0;
			fp->msize  = 0;
		}
		break;

	case SPI_IOTYPE_MEMORY:		/* メモリ入力 */
		switch (method) {
			case FILE_CURRENT: offset -= fp->mcount; /*FALLTHROUGH*/
			case FILE_END:     offset += fp->msize;  /*FALLTHROUGH*/
			case FILE_BEGIN:   break;
			default:           return -1;	/* unknown seek method */
		}
		if (offset < 0) return -1;			/* negative seek error */
		fp->mptr   = fp->mbuffer + offset;
		fp->mcount = fp->msize   - offset;
		break;

	default:				/* 入力未初期化など */
		return -1;
	}

	return offset;
}

#endif	/* SPI_SUPPORT_SPISEEK */


#else	/* SPI_SUPPORT_BUFFERING */

/* ***********************************************************************
**		バッファリングなし入力ストリーム関数
*/

/*
**		入力ストリームのオープン
*/
int SpiOpen(SPI_FILE *fp, LPSTR buf, long len, unsigned int flag)
{
	HANDLE hFile;

	fp->flags = SPI_IOTYPE_NONE;

	if (buf == NULL || len < 0) return SPI_ERROR_FILE_READ;

	switch (flag & 0x07) {
	case 0:			/* 入力がディスクファイル */
		hFile = CreateFile(buf, GENERIC_READ, FILE_SHARE_READ, NULL,
		                   OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
					return SPI_ERROR_FILE_READ;
		if (len != 0)
			SetFilePointer(hFile, len, NULL, FILE_BEGIN);
		fp->flags   = SPI_IOTYPE_FILE;
		fp->fhandle = hFile;
		fp->fname   = buf;
		fp->foffset = len;
		break;

	case 1:			/* 入力がメモリバッファ */
		fp->flags   = SPI_IOTYPE_MEMORY;
		fp->mbuffer = (LPBYTE)buf;
		fp->mptr    = (LPBYTE)buf;
		fp->mcount  = len;
		fp->msize   = len;
		break;

	default:		/* 不明な入力タイプ */
		return SPI_ERROR_NOT_IMPLEMENTED;
	}

	return SPI_ERROR_SUCCESS;
}


/*
**		入力ストリームのクローズ
*/
void SpiClose(SPI_FILE *fp)
{
	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:
		CloseHandle(fp->fhandle);
		break;

	case SPI_IOTYPE_MEMORY:
		/* Nothing to do */
		break;

	default:
		;
	}
	fp->flags = SPI_IOTYPE_NONE;
}

#ifdef SPI_SUPPORT_SPIGETBYTE

/*
**		入力ストリームから１バイトを読む
**
**		Note) この関数は、バッファリングあり入力関数群との
**		      互換性を維持するためだけのものです。
*/
INT SpiGetByte(SPI_FILE *fp)
{
	DWORD n;
	BYTE c;

	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:		/* ファイル入力 */
		if (!ReadFile(fp->fhandle,&c,1,&n,NULL)) {
			SpiSetError(fp);
			return SPI_EOF;
		}
		if (n == 0) {
			SpiSetEOF(fp);
			return SPI_EOF;
		}
		break;

	case SPI_IOTYPE_MEMORY:		/* メモリ入力 */
		if (fp->mcount <= 0) {
			SpiSetEOF(fp);
			return SPI_EOF;
		}
		c = *(fp->mptr++);
		fp->mcount--;
		break;

	default:				/* 入力未初期化など */
		return SPI_EOF;
	}

	return c;
}

#endif	/* SPI_SUPPORT_SPIGETBYTE */

#ifdef SPI_SUPPORT_SPIREAD

/*
**		入力ストリームから指定バイト数だけデータを読む
**		SPI_SUPPORT_NULLREAD が定義されている場合には、
**		buf に NULL を指定できる(指定されたバイト数だけ空読みする)
**
**		Note) 単純にシークすると、ディスクキャッシュが有効に
**		      働かないため、特にフロッピーから読んだときに
**		      遅くなってしまう場合がある。
*/
DWORD SpiRead(LPVOID buf, DWORD rbytes, SPI_FILE *fp)
{
#ifdef SPI_SUPPORT_NULLREAD
	BYTE nulbuf[1024];
	DWORD r, left;
#endif
	DWORD n;

	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:		/* ファイル入力 */
#ifdef SPI_SUPPORT_NULLREAD
		if (buf == NULL) {
			buf = nulbuf;
			n = sizeof(nulbuf);
		} else {
			n = rbytes;
		}
		for (left = rbytes; left > 0; left -= r) {
			if (left < n) n = left;
			if (!ReadFile(fp->fhandle,buf,n,&r,NULL)) {
				SpiSetError(fp);	/* r = 0; (Set by ReadFile) */
				break;
			}
			if (r != n) {
				SpiSetEOF(fp);
				left -= r;
				break;
			}
		}
		rbytes -= left;
#else	/* ----- */
		if (!ReadFile(fp->fhandle,buf,(n=rbytes),&rbytes,NULL)) {
			SpiSetError(fp);	/* rbytes = 0; (Set by ReadFile) */
		} else if (rbytes != n) {
			SpiSetEOF(fp);
		}
#endif	/* ----- */
		break;

	case SPI_IOTYPE_MEMORY:		/* メモリ入力 */
		if (fp->mcount <= 0) { SpiSetEOF(fp); return 0; }
		if ((DWORD)fp->mcount < rbytes) {
			rbytes = (DWORD)fp->mcount;
			SpiSetEOF(fp);
		}
		if (!IsBufNULL(buf))
			CopyMemory(buf, fp->mptr, rbytes);
		fp->mptr   += rbytes;
		fp->mcount -= rbytes;
		break;

	default:				/* 入力未初期化など */
		return 0;
	}

	return rbytes;
}

#endif	/* SPI_SUPPORT_SPIREAD */

#ifdef SPI_SUPPORT_SPISEEK

/*
**		入力ストリームをシークする
*/
LONG SpiSeek(SPI_FILE *fp, LONG offset, DWORD method)
{
	SpiClearEOF(fp);

	switch (SpiIoType(fp)) {
	case SPI_IOTYPE_FILE:		/* ファイル入力 */
		if (method == FILE_BEGIN) offset += fp->foffset;
		offset = (LONG)SetFilePointer(fp->fhandle, offset, NULL, method);
		if (offset != -1) offset -= fp->foffset;
		break;

	case SPI_IOTYPE_MEMORY:		/* メモリ入力 */
		switch (method) {
			case FILE_CURRENT: offset -= fp->mcount; /*FALLTHROUGH*/
			case FILE_END:     offset += fp->msize;  /*FALLTHROUGH*/
			case FILE_BEGIN:   break;
			default:           return -1;	/* unknown seek method */
		}
		if (offset < 0) return -1;			/* negative seek error */
		fp->mptr   = fp->mbuffer + offset;
		fp->mcount = fp->msize   - offset;
		break;

	default:				/* 入力未初期化など */
		return -1;
	}

	return offset;
}

#endif	/* SPI_SUPPORT_SPISEEK */

#endif	/* SPI_SUPPORT_BUFFERING */

