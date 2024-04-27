/*
**  Susie import filter plug-in
**  spibase.h - Basic function library
**
**  Public domain by MIYASAKA Masaru (Feb 14, 2006)
*/

#ifndef SPIBASE_H
#define SPIBASE_H

/* -----------------------------------------------------------------------
**		Configurations
*/

	/* spientry.c */
/* #undef SPI_IMPLEMENT_DLLMAIN */		/* DllMain() を定義する */
#undef SPI_IMPLEMENT_DLLHANDLE		/* ghThisInst を定義する */
#undef SPI_IMPLEMENT_INITOPTIONS	/* DllMain() でオプションを初期化する */
#undef SPI_IMPLEMENT_CONFIGDLG		/* ConfigurationDlg() を定義する */
#undef SPI_IMPLEMENT_ABOUTDLG		/* 独自の About ダイアログを実装する */
#undef SPI_IMPLEMENT_GETPREVIEW 	/* GetPreview() 用のコードを実装する */

	/* spiio.c */
#define SPI_SUPPORT_BUFFERING		/* 読み込みデータのバッファリングを行う */
#undef SPI_SUPPORT_SPIGETBYTE		/* SpiGetByte() を利用可能にする */
#undef SPI_SUPPORT_SPIREAD			/* SpiRead() を利用可能にする */
#undef SPI_SUPPORT_NULLREAD 		/* SpiRead() での空読み動作を有効にする */
#undef SPI_SUPPORT_SPISEEK			/* SpiSeek() を利用可能にする */
#undef SPI_SUPPORT_EOFFLAG			/* EOF を表すフラグを利用可能にする */
#undef SPI_OPTIMIZE_SPIREAD 		/* 最適化された SpiRead() を使用する */
#undef SPI_OPTIMIZE_SPISEEK 		/* 最適化された SpiSeek() を使用する */

	/* spialloc.c */
#undef SPI_ALLOCATE_ROWPOINTERS 	/* 行ポインタ配列を割り当てる */
#undef SPI_SUPPORT_SPIREALLOC		/* SpiReAllocBuffer() を利用可能にする */

#define SPI_BUFSIZ			(32*1024)	/* 読み込みバッファサイズ */

#if defined(_MSC_VER) && (_MSC_VER < 1400) && defined(NO_CRTENV)
/* VC2005 以降の場合、memmove() がSIMD命令を使っていて、SIMD命令の
 * サポートチェックなどを行なう必要があるため、スタートアップルーチン
 * のリンクが必要となる。コンパイラに付属の memmove() に代わって、
 * SIMD命令を使わない独自の memmove() をリンクするようにすれば、
 * VC2005 以降の場合でもスタートアップルーチンを外すことはできるはず。
 */
#define SPI_IMPLEMENT_DLLMAIN		/* DllMain() を定義する */
#define DllMain	_DllMainCRTStartup	/* スタートアップルーチンをリンクしない */
#endif


/* -----------------------------------------------------------------------
**		Utility macro definitions
*/

#if defined(_WIN32) && !defined(_export)
# if defined(_MSC_VER) || defined(__BORLANDC__)
#  define _export   __declspec(dllexport)
# else
#  define _export
# endif
#endif
		/* 'fastcall' calling convention */
#ifndef FASTCALL
# if defined(__GNUC__) && defined(__i386__)
#  define FASTCALL  __attribute__((regparm(3)))
# elif defined(_MSC_VER) || defined(__BORLANDC__)
#  define FASTCALL  __fastcall
# else
#  define FASTCALL
# endif
#endif
		/* for functions that never return */
#ifndef NORETURN
# if defined(__GNUC__)
#  define NORETURN  __attribute__((noreturn))
# elif defined(_MSC_VER)
#  define NORETURN  __declspec(noreturn)
# else
#  define NORETURN
# endif
#endif
		/* 'inline' functions in C */
#ifndef INLINE
# if defined(__GNUC__)
#  define INLINE  __inline__
# elif defined(_MSC_VER) || defined(__BORLANDC__)
#  define INLINE  __inline
# else
#  define INLINE
# endif
#endif


/* -----------------------------------------------------------------------
**		Susie import filter plug-in - API layer (spientry.c)
*/

	/* エラーコード */
#define SPI_ERROR_NOT_IMPLEMENTED	(-1)	/* その機能は未実装 */
#define SPI_ERROR_SUCCESS			0	/* 正常終了 */
#define SPI_ERROR_CANCEL_EXPAND		1	/* コールバック関数が非0を返した */
#define SPI_ERROR_UNKNOWN_FORMAT	2	/* 未知のフォーマット */
#define SPI_ERROR_BROKEN_DATA		3	/* データが壊れている */
#define SPI_ERROR_ALLOCATE_MEMORY	4	/* メモリーが確保出来ない */
#define SPI_ERROR_MEMORY			5	/* メモリーエラー（Lock出来ない等） */
#define SPI_ERROR_FILE_READ			6	/* ファイルリードエラー */
#define SPI_ERROR_WINDOW			7	/* 窓が開けない (非公開) */
#define SPI_ERROR_INTERNAL			8	/* 内部エラー */
#define SPI_ERROR_FILE_WRITE		9	/* 書き込みエラー (非公開) */
#define SPI_ERROR_END_OF_FILE		10	/* ファイル終端 (非公開) */

	/* 画像情報構造体 */
#pragma pack(1)
typedef struct PictureInfo {
	long   left, top;			/* 画像を展開する位置 */
	long   width;				/* 画像の幅(pixel) */
	long   height;				/* 画像の高さ(pixel) */
	WORD   x_density;			/* 画素の水平方向密度 */
	WORD   y_density;			/* 画素の垂直方向密度 */
	short  colorDepth;			/* 画素当たりのbit数 */
#ifdef _WIN64
	char   dummy[2];			/* アラインメント */
#endif
	HLOCAL hInfo;				/* 画像内のテキスト情報 */
} PictureInfo;
#pragma pack()

	/* コールバック関数(typedef) */
#ifdef __GNUC__
typedef int CALLBACK (*SPIPROC) (int, int, LONG_PTR);
#else
typedef int (CALLBACK *SPIPROC) (int, int, LONG_PTR);
#endif

	/* グローバル変数 */
#if defined(SPI_IMPLEMENT_DLLMAIN) && defined(SPI_IMPLEMENT_DLLHANDLE)
extern HINSTANCE ghThisInst;	/* Handle to the DLL's instance. */
#endif

// コールバック
typedef int (__stdcall *SUSIE_PROGRESS)(int nNum, int nDenom, LONG_PTR lData);

	/* プロトタイプ宣言 */
#ifdef SPI_IMPLEMENT_DLLMAIN
# if defined(__RSXNT__)
#  define DllMain LibMain
# elif defined(__BORLANDC__)
#  define DllMain DllEntryPoint
# endif
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
#endif
#ifdef SPI_IMPLEMENT_CONFIGDLG
int _export PASCAL ConfigurationDlg(HWND, int);
#endif
int _export PASCAL GetPluginInfo(int, LPSTR, int);
int _export PASCAL GetPluginInfoW(int, LPWSTR, int);
int _export PASCAL IsSupported(LPCSTR, void*);
int _export PASCAL IsSupportedW(LPCWSTR, void*);
int _export PASCAL GetPictureInfo(LPCSTR, LONG_PTR, unsigned int, PictureInfo *);
int _export PASCAL GetPictureInfoW(LPCWSTR, LONG_PTR, unsigned int, PictureInfo *);
int _export PASCAL GetPicture(LPCSTR, LONG_PTR, unsigned int, HLOCAL *, HLOCAL *,
                              SUSIE_PROGRESS, LONG_PTR);
int _export PASCAL GetPictureW(LPCWSTR, LONG_PTR, unsigned int, HLOCAL *, HLOCAL *,
                              SUSIE_PROGRESS, LONG_PTR);
int _export PASCAL GetPreview(LPCSTR, LONG_PTR, unsigned int, HLOCAL *, HLOCAL *,
                              SUSIE_PROGRESS, LONG_PTR);
int _export PASCAL GetPreviewW(LPCWSTR, LONG_PTR, unsigned int, HLOCAL *, HLOCAL *,
                              SUSIE_PROGRESS, LONG_PTR);


/* -----------------------------------------------------------------------
**		Susie import filter plug-in - File I/O (spiio.c)
*/

#ifndef SPI_BUFSIZ
# define SPI_BUFSIZ			8192	/* デフォルトバッファサイズ */
#endif
#ifndef SPI_EOF
# define SPI_EOF			(-256)	/* End Of File (any negative value) */
#endif

	/* ストリーム状態フラグ */
#define SPI_IOTYPE_NONE		0x00	/* I/O タイプ：入力未初期化 */
#define SPI_IOTYPE_FILE		0x01	/* I/O タイプ：ファイル入力 */
#define SPI_IOTYPE_MEMORY	0x02	/* I/O タイプ：メモリ入力 */
#define SPI_IOTYPE_MASK		0x0F
#define SPI_IOFLAG_ERROR	0x10	/* エラーを検出 */
#define SPI_IOFLAG_EOF		0x20	/* EOFを検出 */

	/* 状態フラグ関連マクロ */
#define SpiIoType(f)		((f)->flags & SPI_IOTYPE_MASK)
#define SpiIsError(f)		((f)->flags & SPI_IOFLAG_ERROR)
#define SpiSetError(f)		((f)->flags |= SPI_IOFLAG_ERROR)
#define SpiClearError(f)	((f)->flags &= ~(SPI_IOFLAG_ERROR|SPI_IOFLAG_EOF))
#ifdef SPI_SUPPORT_EOFFLAG
# define SpiIsEOF(f)		((f)->flags & SPI_IOFLAG_EOF)
# define SpiSetEOF(f)		((f)->flags |= SPI_IOFLAG_EOF)
# define SpiClearEOF(f)		((f)->flags &= ~SPI_IOFLAG_EOF)
#else
# define SpiIsEOF(f)		(0)
# define SpiSetEOF(f)
# define SpiClearEOF(f)
#endif

	/* Susie plug-in 用入力ストリーム構造体 */
typedef struct {
	UINT   flags;				/* ストリーム状態フラグ */
	HANDLE fhandle;				/* ファイル入力のファイルハンドル */
	LPCSTR  fname;				/* ファイル入力の入力ファイル名 */
	LONG_PTR   foffset;			/* ファイル入力の読み込み開始オフセット */
#ifdef SPI_SUPPORT_BUFFERING
	LONG   ffilptr;				/* ファイル入力のファイルポインタ */
#endif
	LPBYTE mbuffer;				/* メモリ入力のバッファ */
	LPBYTE mptr;				/* メモリ入力のポインタ */
	LONG_PTR   mcount;			/* メモリ入力の残りバイト数 */
	LONG_PTR   msize;			/* メモリ入力のデータサイズ */
} SPI_FILE;

	/* プロトタイプ宣言 */
int SpiOpen(SPI_FILE *, LPCSTR, LONG_PTR, unsigned int);
int SpiOpenW(SPI_FILE *, LPCWSTR, LONG_PTR, unsigned int);
void SpiClose(SPI_FILE *);
#ifdef SPI_SUPPORT_BUFFERING
void SpiFillBuf(SPI_FILE *);
#endif
#ifdef SPI_SUPPORT_SPIREAD
DWORD SpiRead(LPVOID, DWORD, SPI_FILE *);
#endif
#ifdef SPI_SUPPORT_SPISEEK
LONG SpiSeek(SPI_FILE *, LONG, DWORD);
#endif
#ifdef SPI_SUPPORT_SPIGETBYTE
# ifdef SPI_SUPPORT_BUFFERING
# define SpiGetByte(f) \
           ((--(f)->mcount >= 0) ? (INT)(*(f)->mptr++) : SpiFillBufX(f))
INT SpiFillBufX(SPI_FILE *);
# else
INT SpiGetByte(SPI_FILE *);
# endif
#endif


/* -----------------------------------------------------------------------
**		Susie import filter plug-in - Memory related (spialloc.c)
*/

#define SpiFreeRowptr(p)		LocalFree((HLOCAL)(p))
#define SpiUnlockBuffer(ph)		LocalUnlock(*(ph))
#define SpiReAllocBufferX(ph,s)	(*(ph) = LocalReAlloc(*(ph),(s),LMEM_MOVEABLE))
#define SpiFreeBuffer(ph)		(*(ph) = LocalFree(*(ph)))
            /* ↑LocalFree は、成功した場合は NULL を返す。失敗した場合は
             *   引数の値をそのまま返す。引数の値が NULL ならば何もせずに
             *   NULL を返す。
             */

	/* PictureInfo 構造体に値を設定する */
#define SpiSetPictureInfo(pi,w,h,c,xd,yd,l,t,tx) \
          do { UINT _xd=(xd), _yd=(yd);  if (_xd==0 || _yd==0) _xd=_yd=0; \
               (pi)->left      = (long)(l);  (pi)->top       = (long)(t); \
               (pi)->width     = (long)(w);  (pi)->height    = (long)(h); \
               (pi)->x_density = (WORD)_xd;  (pi)->y_density = (WORD)_yd; \
               (pi)->colorDepth = (short)(c); (pi)->hInfo = (HLOCAL)(tx); \
          } while (0)

	/* プロトタイプ宣言 */
int SpiAllocBuffer(HLOCAL *, LPBYTE *, UINT);
#ifdef SPI_SUPPORT_SPIREALLOC
int SpiReAllocBuffer(HLOCAL *, LPBYTE *, UINT);
#endif
#ifdef SPI_ALLOCATE_ROWPOINTERS
int SpiAllocImageBuffer(HLOCAL *, LPBYTE **, DWORD, DWORD);
int SpiInitBitmap(HLOCAL *, LPBITMAPINFO *, HLOCAL *, LPBYTE **,
                  DWORD, DWORD, int, int, DWORD, DWORD);
#else
int SpiInitBitmap(HLOCAL *, LPBITMAPINFO *, HLOCAL *, LPBYTE *, DWORD *,
                  DWORD, DWORD, int, int, DWORD, DWORD);
#endif


/* -----------------------------------------------------------------------
**		External definitions
*/

	/* グローバル変数 */
extern const LPCSTR PluginInfo[];	/* プラグイン情報 */
extern const int    NumInfo;		/* プラグイン情報の情報数 */

	/* プロトタイプ宣言 */
#ifdef SPI_IMPLEMENT_DLLMAIN
void InitOptions(void);
#endif
#ifdef SPI_IMPLEMENT_CONFIGDLG
void ExecSetupDialog(HWND);
# ifdef SPI_IMPLEMENT_ABOUTDLG
void ExecAboutDialog(HWND);
# endif
#endif
int IsSupportedFormat(LPBYTE, DWORD, LPCSTR);
int GetImageInfo(SPI_FILE *, PictureInfo *);
#ifdef SPI_IMPLEMENT_GETPREVIEW
int GetImage(SPI_FILE *, HANDLE *, HANDLE *, SPIPROC, LONG_PTR, BOOL);
#else
int GetImage(SPI_FILE *, HANDLE *, HANDLE *, SPIPROC, LONG_PTR);
#endif


#endif	/* SPIBASE_H */
