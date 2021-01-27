/*
**  pilib.h - .pi 読み込み/書き出しライブラリ ver.3.10 (Sep 6, 2004
**
**  Copyright (C) 1999-2004 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
**
**  This software is provided 'as-is', without any express or implied warranty.
**  In no event will the authors be held liable for any damages arising from
**  the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**
**  2. Altered source versions must be plainly marked as such, and must not
**     be misrepresented as being the original software.
**
**  3. This notice may not be removed or altered from any source distribution.
**
**  ----
**
**  本ソフトウェアは「現状のまま」で、明示であるか暗黙であるかを問わず、何らの
**  保証もなく提供されます。 本ソフトウェアの使用によって生じるいかなる損害に
**  ついても、作者は一切の責任を負わないものとします。
**
**  以下の制限に従う限り、商用アプリケーションを含めて、本ソフトウェアを任意の
**  目的に使用し、自由に改変して再頒布することをすべての人に許可します。
**
**  1. 本ソフトウェアの出自について虚偽の表示をしてはなりません。
**     あなたがオリジナルのソフトウェアを作成したと主張してはなりません。
**     あなたが本ソフトウェアを製品内で使用する場合、製品の文書に謝辞を入れて
**     いただければ幸いですが、必須ではありません。
**
**  2. ソースを変更した場合は、そのことを明示しなければなりません。オリジナル
**     のソフトウェアであるという虚偽の表示をしてはなりません。
**
**  3. ソースの頒布物から、この表示を削除したり、表示の内容を変更したりしては
**     なりません
**
**  ----
**
**  NOTE: Comments are written in Japanese. Sorry.
*/

#ifndef PILIB_H
#define PILIB_H

/* -----------------------------------------------------------------------
**		Basic functions
*/

#include <limits.h>		/* for CHAR_BIT */
#include <stddef.h>		/* for size_t & ptrdiff_t */

#if 1	/* Use Win32-API functions */

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define pi_memmove			MoveMemory
#define pi_memcpy			CopyMemory
#define pi_memset(d,c,l)	FillMemory((d),(l),(c))
#define pi_malloc(s)		((pi_voidp)LocalAlloc(LMEM_FIXED,(s)))
#define pi_free(s)			LocalFree((HLOCAL)(s))

#else	/* Use standad C library functions */

#include <stdlib.h>
#include <string.h>

#define pi_memmove			memmove
#define pi_memcpy			memcpy
#define pi_memset			memset
#define pi_malloc			malloc
#define pi_free				free

#endif

/* -----------------------------------------------------------------------
**		Configurations
*/

	/* Susieエラーコード (from spibase.h)*/
#ifndef SPI_ERROR_SUCCESS
#define SPI_ERROR_NOT_IMPLEMENTED	(-1)	/* その機能は未実装 */
#define SPI_ERROR_SUCCESS			0	/* 正常終了 */
#define SPI_ERROR_CANCEL_EXPAND		1	/* コールバック関数が非0を返した */
#define SPI_ERROR_UNKNOWN_FORMAT	2	/* 未知のフォーマット */
#define SPI_ERROR_BROKEN_DATA		3	/* データが壊れている */
#define SPI_ERROR_ALLOCATE_MEMORY	4	/* メモリーが確保出来ない */
#define SPI_ERROR_MEMORY			5	/* メモリーエラー（Lock出来ない等）*/
#define SPI_ERROR_FILE_READ			6	/* ファイルリードエラー */
#define SPI_ERROR_WINDOW			7	/* 窓が開けない (非公開) */
#define SPI_ERROR_INTERNAL			8	/* 内部エラー */
#define SPI_ERROR_FILE_WRITE		9	/* 書き込みエラー (非公開) */
#define SPI_ERROR_END_OF_FILE		10	/* ファイル終端 (非公開) */
#endif

	/* エラーコードのオーバーライド定義 */
#define PI_OK						SPI_ERROR_SUCCESS
#define PI_ERR_NOT_A_PI				SPI_ERROR_UNKNOWN_FORMAT
#define PI_ERR_INVALID_BITDEPTH		SPI_ERROR_BROKEN_DATA
#define PI_ERR_INVALID_WIDTH		SPI_ERROR_BROKEN_DATA
#define PI_ERR_INVALID_HEIGHT		SPI_ERROR_BROKEN_DATA
#define PI_ERR_FILE_WRITE			SPI_ERROR_FILE_WRITE
#define PI_ERR_FILE_READ			SPI_ERROR_FILE_READ
#define PI_ERR_END_OF_FILE			SPI_ERROR_END_OF_FILE
#define PI_ERR_OUT_OF_MEMORY		SPI_ERROR_ALLOCATE_MEMORY
#define PI_ERR_CALLBACK_CANCELED	SPI_ERROR_CANCEL_EXPAND

	/* パレット構造体のオーバーライド定義
		(= BMP ファイルの RGBQUAD 構造体) */
typedef RGBQUAD pi_color;
#define PI_COLOR_DEFINED
#define pi_set_color(p,r,g,b) \
            ((p)->rgbRed = (r), (p)->rgbGreen = (g), (p)->rgbBlue = (b))
#define pi_get_color(p,r,g,b) \
            (*(r) = (p)->rgbRed, *(g) = (p)->rgbGreen, *(b) = (p)->rgbBlue)

	/* 各種動作設定 */
#undef PI_SUPPORT_LONGJMP				/* longjmp() を使ってエラー処理をする */
#undef PI_SUPPORT_PROGRESS_CALLBACK 	/* コールバック関数を使用可能にする */

#undef PI_READ_SKIP_MACBINARY			/* 先頭の MacBinary を読み飛ばす */
#undef PI_READ_SUPPORT_STDIO			/* Ｃ標準ストリーム入力のサポート */
#define PI_READ_SUPPORT_EXTINFO			/* ヘッダの拡張情報を取得する */
#define PI_READ_SUPPORT_COMMENT			/* 内蔵テキストを取得する */
#undef PI_READ_SUPPORT_READIMAGE		/* pi_read_image() を使用可能にする */
#undef PI_READ_OUTPUT_8BPP_FMT			/* 常に 8bit/pixel 形式で出力する */

#undef PI_WRITE_SUPPORT_STDIO			/* Ｃ標準ストリーム出力のサポート */
#define PI_WRITE_SUPPORT_EXTINFO 		/* ヘッダの拡張情報を設定する */
#define PI_WRITE_SUPPORT_COMMENT 		/* 内蔵テキストを設定する */
#undef PI_WRITE_SUPPORT_WRITEIMAGE		/* pi_write_image() を使用可能にする */
#undef PI_WRITE_INPUT_8BPP_FMT			/* 常に 8bit/pixel 形式で入力する */

#define PI_CALL_USING_FASTCALL			/* 内部関数に fastcall 呼び出しを使う */
#define PI_INIT_STRUCT_BY_MEMSET 		/* pi_memset() で構造体を初期化する */
#define PI_WORD_OPERATION_ON_PIXEL		/* 16ビット単位でピクセルを扱う */
#define PI_WRITE_PUTBITS_FUNC			/* 関数版のビット出力ルーチンを使う */

	/* 幅が2pixel以下の画像を、Pi仕様書(PITECH.TXT)に書かれているとおりに
	 * エンコード／デコードするには、以下のマクロを #define します。
	 * 詳しくは piwrite.c の中の「幅が2pixel以下の画像の処理法」を見て
	 * ください。特に理由のない限り #undef にしておくことをお薦めします。*/
#undef PI_READ_W2_SPEC_COMPLIANT_DECODING
#undef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING



/* ***********************************************************************
**		Pi format encoding/decoding library (piwrite.c/piread.c)
*/

	/* デフォルトのエラーコード(再定義可能) */
#ifndef PI_OK
#define PI_OK						0		/* 正常 */
#define PI_ERR_NOT_A_PI				1		/* .PIファイルではない */
#define PI_ERR_INVALID_BITDEPTH		2		/* 無効な色深度(4,8 以外) */
#define PI_ERR_INVALID_WIDTH		3		/* 無効な幅(0 or 65535超) */
#define PI_ERR_INVALID_HEIGHT		4		/* 無効な高さ(0 or 65535超) */
#define PI_ERR_FILE_WRITE			5		/* ファイル書き込みエラー */
#define PI_ERR_FILE_READ			6		/* ファイル読み込みエラー */
#define PI_ERR_END_OF_FILE			7		/* ファイル終端を検出 */
#define PI_ERR_OUT_OF_MEMORY		8		/* メモリ不足 */
#define PI_ERR_CALLBACK_CANCELED	9		/* 行コールバック関数による中止 */
#endif

	/* 各種フラグなど */
#define PI_FLAG_NO_COMMENT		0x0001		/* コメントを取得しない */
#define PI_FLAG_MISSING_SIG		0x0002		/* ファイル先頭の "Pi" がない */
#define PI_MODE_NO_PALETTE		0x80		/* デフォルトパレット使用画像 */
#define PI_TRANSCOLOR_NONE		(-1)		/* 透明色がない場合の指示値 */

	/* エラー処理用マクロ */
#ifdef PI_SUPPORT_LONGJMP
	/* setjmp() と longjmp() による方法 */
#include <setjmp.h>		/* setjmp() & longjmp() */
#define PI_ISERR(p)     (0)
#define PI_RESETERR(p)
#define PI_CHKERR_RETURN(p)
#define PI_CHKERR_RETURN_VAL(p,r)
#define PI_SETERR_RETURN(p,e)        longjmp((p)->jmpbuf,(e))
#define PI_SETERR_RETURN_VAL(p,e,r)  longjmp((p)->jmpbuf,(e))
#else
	/* 変数 error の値を見ながら順次 return していく方法 */
#define PI_ISERR(p)     ((p)->error!=PI_OK)
#define PI_RESETERR(p)  ((p)->error=PI_OK)
#define PI_CHKERR_RETURN(p) \
            do { if (PI_ISERR(p)) return; } while (0)
#define PI_CHKERR_RETURN_VAL(p,r) \
            do { if (PI_ISERR(p)) return (r); } while (0)
#define PI_SETERR_RETURN(p,e) \
            do { if (!PI_ISERR(p)) (p)->error=(e); return; } while (0)
#define PI_SETERR_RETURN_VAL(p,e,r) \
            do { if (!PI_ISERR(p)) (p)->error=(e); return (r); } while (0)
#endif /* PI_SUPPORT_LONGJMP */

	/* 一行当たりのバイト数を計算するマクロ */
#ifdef PI_READ_OUTPUT_8BPP_FMT
#define pi_read_get_rowbytes(p) ((p)->width)
#else
#define pi_read_get_rowbytes(p) \
            (((p)->bitdepth == 8) ? (p)->width : ((pi_uint32)(p)->width+1)/2)
#endif

	/* piread.c / piwrite.c でのみ参照されるマクロ */
#ifdef PI_INTERNAL

	/* FASTCALL 呼び出し規約 */
#undef FASTCALL
#ifdef PI_CALL_USING_FASTCALL
#if defined(__GNUC__) && defined(__i386__)			/* gcc (on i386) */
#define FASTCALL  __attribute__((regparm(3)))
#elif defined(_MSC_VER) || defined(__BORLANDC__)	/* MS-C or BCC */
#define FASTCALL  __fastcall
#endif
#endif /* PI_CALL_USING_FASTCALL */
#ifndef FASTCALL
#define FASTCALL
#endif

	/* 変数/型のビット数 */
#define bitsof(x)	(sizeof(x) * CHAR_BIT)

#endif /* PI_INTERNAL */

	/* typedef's */
typedef char           pi_char;		/* signed or unsigned */
typedef unsigned char  pi_byte;
typedef int            pi_int;
typedef unsigned int   pi_uint;
typedef short          pi_int16;
typedef unsigned short pi_uint16;
typedef long           pi_int32;
typedef unsigned long  pi_uint32;

typedef unsigned int pi_bitbuf;		/* unsigned かつ 16bit 以上 */
typedef size_t       pi_size_t;
typedef ptrdiff_t    pi_ptrdif_t;

typedef void      * pi_voidp;
typedef pi_char   * pi_charp;
typedef pi_byte   * pi_bytep;
typedef pi_int    * pi_intp;
typedef pi_uint   * pi_uintp;
typedef pi_int16  * pi_int16p;
typedef pi_uint16 * pi_uint16p;
typedef pi_int32  * pi_int32p;
typedef pi_uint32 * pi_uint32p;
typedef pi_bytep  * pi_bytepp;

typedef struct pi_struct   pi_struct;
typedef struct pi_struct * pi_structp;
typedef struct pi_text     pi_text;
typedef struct pi_text   * pi_textp;
typedef struct pi_stdio    pi_stdio;
typedef struct pi_stdio  * pi_stdiop;

typedef pi_size_t (*pi_iofncp)(pi_structp);
typedef pi_int (*pi_progfncp)(pi_structp, pi_uint, pi_uint);

	/* パレット用カラー構造体(デフォルト定義) */
#ifndef PI_COLOR_DEFINED
#define PI_COLOR_DEFINED
typedef struct pi_color {
	pi_byte		red;
	pi_byte		green;
	pi_byte		blue;
} pi_color;
#endif
typedef pi_color * pi_colorp;

	/* Ｃ言語標準 I/O による入出力サポート用構造体 */
#ifdef PI_READ_SUPPORT_STDIO
#include <stdio.h>
struct pi_stdio {
	FILE		*fp;			/* 入出力ストリーム */
	pi_byte		iobuf[4096];	/* 入出力バッファ */
};
#endif

	/* 内蔵コメント保持用構造体 */
#ifdef PI_READ_SUPPORT_COMMENT
struct pi_text {
	pi_textp	next;		/* 次へのポインタ */
	pi_uint32	size;		/* 内蔵テキストのサイズ */
	pi_charp	text;		/* 内蔵テキスト */
};
#endif

	/* pi_struct 構造体 */
struct pi_struct {
		/* エラー処理用変数 */
#ifdef PI_SUPPORT_LONGJMP
	jmp_buf		jmpbuf;				/* setjmp() / longjmp() 用コンテキスト */
#else
	pi_int		error;				/* エラーコード */
#endif
		/* .PIファイルヘッダ情報＆各種画像情報 */
	pi_uint		width, height;		/* 画像のサイズ */
	pi_uint		colors;				/* 色数(16 or 256) */
	pi_byte		bitdepth;			/* 色深度(4 or 8) */
	pi_byte		aspect_x, aspect_y;	/* アスペクト比 */
	pi_byte		mode;				/* 画像モード */
	pi_byte		machine[4];			/* 機種識別子(エンコーダ名) */

		/* I/O 関係 */
	pi_iofncp	iofunc;				/* I/O 関数 */
	pi_voidp	ioptr;				/* I/O 関数用汎用ポインタ */
	pi_bytep	iobufptr;			/* I/O バッファのポインタ */
	pi_size_t	iobufcnt;			/* I/O バッファの残りバイト数 */
	pi_bitbuf	bitbuf;				/* ビットバッファ */
	pi_uint		bitcnt;				/* ビットバッファの残りビット数 */

		/* 作業用領域・作業用変数 */
	pi_bytep	clrtable;			/* カラーテーブル */
	pi_bytep	clrcode;			/* カラーコードテーブル */
	pi_bytep	clrcodelen;			/* カラーコードの長さ */
	pi_bytep	lencode;			/* 連鎖数コードテーブル */
	pi_bytep	lencodelen;			/* 連鎖数コードの長さ */

	pi_uint		prevpos;			/* 一つ前の位置コード */
	pi_uint		savedpos;			/* 現在の位置コード */
	pi_uint32	savedlen;			/* 現在の連鎖数 */

	pi_bytep	rowbuf;				/* 行バッファ */
	pi_bytep	rowptr[4];			/* 行の先頭アドレス */
	pi_bytep	rowend;				/* 行の終端アドレス */
	pi_bytep	currentp;			/* 現在の行の注目点 */
	pi_uint		rownum;				/* 現在の行の行番号 (1～height) */
	pi_ptrdif_t	posdiff[6];			/* 位置コードに対応する相対アドレス */

		/* .PIファイルヘッダの拡張情報 */
#if defined(PI_READ_SUPPORT_EXTINFO) || defined(PI_WRITE_SUPPORT_EXTINFO)
	pi_uint		offset_x, offset_y;	/* 画像の表示開始点(オフセット) */
	pi_int		transcolor;			/* 透明色になるパレット番号 (0～15/255) */
	pi_int		sigbits;			/* パレットの有効ビット数 (1～8) */
	pi_int		colorused;			/* パレットの使用個数 (1～16/256) */
#endif
		/* 内蔵テキスト情報 */
#if defined(PI_READ_SUPPORT_COMMENT) || defined(PI_WRITE_SUPPORT_COMMENT)
	pi_text		comment;			/* 内蔵テキストバッファ */
#endif
		/* コールバック関数 */
#ifdef PI_SUPPORT_PROGRESS_CALLBACK
	pi_progfncp	progfunc;			/* コールバック関数 */
	pi_voidp	progptr;			/* コールバック関数用汎用ポインタ */
#endif
};

	/* プロトタイプ宣言 (piread.c) */
void pi_read_init(pi_structp);
void pi_read_end(pi_structp);
void pi_read_header(pi_structp, pi_uint);
#ifdef PI_READ_SUPPORT_COMMENT
void pi_read_get_comment(pi_structp, pi_charp, pi_int);
#endif
void pi_read_palette(pi_structp, pi_colorp);
void pi_read_row_init(pi_structp);
int pi_read_row(pi_structp, pi_bytep);
#ifdef PI_READ_SUPPORT_READIMAGE
void pi_read_image(pi_structp, pi_bytepp);
#endif
#ifdef PI_READ_SUPPORT_STDIO
void pi_read_init_io(pi_structp, FILE *);
void pi_read_end_io(pi_structp);
#endif

	/* プロトタイプ宣言 (piwrite.c) */
void pi_write_init(pi_structp);
void pi_write_end(pi_structp);
void pi_write_header(pi_structp);
void pi_write_palette(pi_structp, pi_colorp /*, pi_int*/);
void pi_write_row_init(pi_structp);
int pi_write_row(pi_structp, pi_bytep);
#ifdef PI_WRITE_SUPPORT_WRITEIMAGE
void pi_write_image(pi_structp, pi_bytepp);
#endif
#ifdef PI_WRITE_SUPPORT_STDIO
void pi_write_init_io(pi_structp, FILE *);
void pi_write_end_io(pi_structp);
#endif

#endif /* PILIB_H */
