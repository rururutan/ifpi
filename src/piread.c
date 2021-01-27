/*
**  piread.c - .pi 読み込みライブラリ ver.3.10a (Nov 24, 2004)
**
**  Copyright(C) 1999-2004 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
**
**  For conditions of distribution and use, see copyright notice in pilib.h
**
**  使用・配布条件については、pilib.h の中の著作権表示を見てください。
*/

#define PI_INTERNAL
#include "pilib.h"

	/* カラーコードテーブル(256色用)のビット数 (7～14)
	 * ビットバッファの保証ビット数 (bitsof(pi_bitbuf) - 7) 以下
	 * である必要がある(see pi_read_fill_bitbuf())。
	 * 実際のカラーコードは、その90%以上が9ビット長以下なので、
	 * ここでは 9 にしている。 */
#define PI_READ_CLRCODE_TABLE_BITS	9

	/* 連鎖数コードテーブルのビット数 (1～CHAR_BIT) */
#define PI_READ_LENCODE_TABLE_BITS	CHAR_BIT

	/* pi_read_color() のカラーテーブル更新の方法を切り替える閾値
	 * 最適値は、プロセッサやコンパイラ(コンパイルオプションの状態)、
	 * pi_memmove の実装によって かなり異なる。 */
#if !defined(PI_READ_COLOR_THRESHOLD) || \
    (PI_READ_COLOR_THRESHOLD < 1 || PI_READ_COLOR_THRESHOLD > 256)
#undef PI_READ_COLOR_THRESHOLD
#define PI_READ_COLOR_THRESHOLD	32
#endif


	/* プロトタイプ宣言 */
#ifdef PI_READ_SUPPORT_EXTINFO
static void FASTCALL pi_read_extinfo(pi_structp, pi_uint);
#endif
#ifdef PI_READ_SUPPORT_COMMENT
static void FASTCALL pi_read_comment(pi_structp, pi_int);
static void FASTCALL pi_read_free_comment(pi_structp);
#endif
static void FASTCALL pi_read_row_end(pi_structp);
static void FASTCALL pi_read_init_color_table(pi_structp);
static void FASTCALL pi_read_init_colorcode_table(pi_structp);
static void FASTCALL pi_read_init_lencode_table(pi_structp);
static void FASTCALL pi_read_init_rowbuf(pi_structp);
#ifndef PI_READ_OUTPUT_8BPP_FMT
static void FASTCALL pi_read_rowcpy4(pi_bytep, pi_bytep, pi_size_t);
#endif
static void FASTCALL pi_read_initial_colors(pi_structp);
static void FASTCALL pi_read_rotate_rowbuf(pi_structp);
static void FASTCALL pi_read_pixels(pi_structp);
#ifdef PI_READ_W2_SPEC_COMPLIANT_DECODING
static void FASTCALL pi_read_pixels_width2(pi_structp);
#endif
static pi_uint FASTCALL pi_read_pos(pi_structp, pi_bytep);
static pi_uint FASTCALL pi_read_color(pi_structp, pi_uint);
static pi_uint32 FASTCALL pi_read_len(pi_structp, pi_uint);
static void FASTCALL pi_read_fill_bitbuf(pi_structp);
static void FASTCALL pi_read_fill_buffer(pi_structp);
static pi_uint FASTCALL pi_read_byte(pi_structp);
static void FASTCALL pi_read_bytes(pi_structp, pi_bytep, pi_size_t);
static pi_voidp FASTCALL pi_alloc_memory(pi_structp, pi_size_t);
static void FASTCALL pi_free_memory(pi_structp, pi_voidp);
#ifdef PI_READ_SUPPORT_STDIO
static pi_size_t pi_read_iofunc_stdio(pi_structp);
#endif




/* ***********************************************************************
**		pi_struct 構造体の初期化・終了処理
*/

/*
**		構造体の初期化
*/
void pi_read_init(pi_structp pi_ptr)
{
#ifdef PI_INIT_STRUCT_BY_MEMSET
	pi_memset(pi_ptr, 0, sizeof(pi_struct));
#else
	static const pi_struct new_pi;	/* 開始時に 0 (NULL) に初期化される */
	*pi_ptr = new_pi;
#endif
#if (PI_OK != 0)
	PI_RESETERR(pi_ptr);	/* エラーのリセット */
#endif
}


/*
**		構造体に関連づけられているメモリを解放する(終了処理)
*/
void pi_read_end(pi_structp pi_ptr)
{
	pi_read_row_end(pi_ptr);
#ifdef PI_READ_SUPPORT_COMMENT
	pi_read_free_comment(pi_ptr);
#endif
}



/* ***********************************************************************
**		ファイルヘッダの読み込み
*/

#define pi_hi_uint16(v)   ((v)>>16 & 0xFFFF)
#define pi_lo_uint16(v)   ((v) & 0xFFFF)
#define pi_get_uint16(b)  ((pi_uint)(b)[0]<<8 | (pi_uint)(b)[1])
#define pi_get_uint32(b)  ((pi_uint32)(b)[0]<<24 | (pi_uint32)(b)[1]<<16 | \
                           (pi_uint32)(b)[2]<<8 | (pi_uint32)(b)[3])

/*
**		ヘッダを読み込む
*/
void pi_read_header(pi_structp pi_ptr, pi_uint flag)
{
	pi_byte buf[10];
	pi_uint n;

	PI_CHKERR_RETURN(pi_ptr);

	if (!(flag & PI_FLAG_MISSING_SIG)) {
#ifdef PI_READ_SKIP_MACBINARY
		n = 128 - 2;
		for (;;) {
			pi_uint c, d;
			c = pi_read_byte(pi_ptr);
			d = pi_read_byte(pi_ptr);
			if (c == 'P' && d == 'i') break;	/* Check OK */
			if (n == 0) PI_SETERR_RETURN(pi_ptr, PI_ERR_NOT_A_PI);
			for ( ; n > 0; n--) (void)pi_read_byte(pi_ptr);
		}
#else
		if (pi_read_byte(pi_ptr) != 'P' || pi_read_byte(pi_ptr) != 'i')
			PI_SETERR_RETURN(pi_ptr, PI_ERR_NOT_A_PI);
#endif
	}
#ifdef PI_READ_SUPPORT_COMMENT
	pi_read_comment(pi_ptr, (flag & PI_FLAG_NO_COMMENT));
#else
	while (pi_read_byte(pi_ptr) != '\x1A' && !PI_ISERR(pi_ptr)) ;
#endif
	while (pi_read_byte(pi_ptr) != 0) ;
	pi_read_bytes(pi_ptr, buf, 10);
	PI_CHKERR_RETURN(pi_ptr);
	pi_ptr->mode     = buf[0];					/* 画像モード */
	pi_ptr->aspect_x = buf[1];					/* アスペクト比 */
	pi_ptr->aspect_y = buf[2];
	pi_ptr->bitdepth = buf[3];					/* 色深度(4 or 8) */
	pi_memcpy(pi_ptr->machine, buf+4, 4);		/* 機種識別子 */

	if (pi_ptr->bitdepth != 4 && pi_ptr->bitdepth != 8)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_BITDEPTH);

	pi_ptr->colors = 1 << pi_ptr->bitdepth;		/* 色数(16 or 256) */

	n = pi_get_uint16(buf+8);		/* 機種依存情報 or 拡張情報 */
#ifdef PI_READ_SUPPORT_EXTINFO
	pi_read_extinfo(pi_ptr, n);
#else
	for ( ; n > 0; n--)
		(void)pi_read_byte(pi_ptr);
#endif
	pi_read_bytes(pi_ptr, buf, 4);
	pi_ptr->width  = pi_get_uint16(buf);		/* 画像の幅 */
	pi_ptr->height = pi_get_uint16(buf+2);		/* 画像の高さ */

	if (pi_ptr->width  == 0) PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_WIDTH);
	if (pi_ptr->height == 0) PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_HEIGHT);
}


/* -----------------------------------------------------------------------
**		.PI ファイル拡張情報の取得
*/

#ifdef PI_READ_SUPPORT_EXTINFO

/*
**		拡張情報を取得する
**
**		拡張情報は、以下に挙げる Short format / Normal format の
**		データブロックを連続して収めた形式になっている。
**		独自の拡張データをPiファイルに埋め込みたい場合は、1バイト目が
**		英小文字のデータID文字列を持つ Normal format を使うべきとのこと。
**		それ以外の形式のデータブロックは正式な拡張情報を収めるのに使う。
**
**		  |====== Short format  ======
**		  |Offset  Size    Description
**		  | 00h    BYTE    ID       ; データID(0x00..0x1F)
**		  | 01h  4 BYTEs   data     ; データ本体
**		  |
**		  |====== Normal format ======
**		  |Offset  Size    Description
**		  | 00h  4 BYTEs   IDstring ; データID文字列(印刷可能文字x4)
**		  |                         ; 1バイト目は英字であること
**		  | 04h    WORD    datasize ; データ本体のバイト数
**		  |                         ; (big-endien 2バイト整数)
**		  | 06h  N BYTEs   data     ; データ本体
**
*/
static void FASTCALL
 pi_read_extinfo(pi_structp pi_ptr, pi_uint len)
{
	pi_byte c, buf[6];
	pi_uint32 t;
	pi_uint n;

	pi_ptr->transcolor = PI_TRANSCOLOR_NONE;

	while (len >= 5) {
		len -= 5;
		pi_read_bytes(pi_ptr, buf, 5);
		c = buf[0];

		if (/*0x00 <= c &&*/ c <= 0x1F) {	/* Short format */
			t = pi_get_uint32(buf+1);

			switch (c) {
			case 0x01:			/* 画像の表示開始点(X,Y) */
				pi_ptr->offset_x = (pi_uint)pi_hi_uint16(t);
				pi_ptr->offset_y = (pi_uint)pi_lo_uint16(t);
				break;
			case 0x02:			/* 透明色になるパレット番号 (0～15/255) */
				if (t < pi_ptr->colors)
					pi_ptr->transcolor = (pi_int)t;
				break;
			case 0x03:			/* パレットの有効ビット数 (1～8) */
				if (t > 0 && t <= 8)
					pi_ptr->sigbits   = (pi_int)t;
				break;
			case 0x04:			/* パレットの使用個数 (1～16/256) */
				if (t > 0 && t <= pi_ptr->colors)
					pi_ptr->colorused = (pi_int)t;
				break;
			default:
				;
			}
		} else if (('A'<=c && c<='Z') || ('a'<=c && c<='z')) {
			if (len < 1) break;				/* Normal format */
			len -= 1;
			buf[5] = pi_read_byte(pi_ptr);
			n = pi_get_uint16(buf+4);
			if (len < n) break;
			len -= n;

			if (/*memcmp(buf,"keyw",4)==0*/ 0) {
				/* 必ず n バイト全部を読むこと */
				for ( ; n > 0; n--)
					(void)pi_read_byte(pi_ptr);
			} else {
				for ( ; n > 0; n--)
					(void)pi_read_byte(pi_ptr);
			}
		} else {
			break;
		}
	}
	for ( ; len > 0; len--) {		/* 拡張情報ではないデータ(機種依存情報) */
		(void)pi_read_byte(pi_ptr);
	}
}

#endif /* PI_READ_SUPPORT_EXTINFO */


/* -----------------------------------------------------------------------
**		内蔵コメントの処理
*/

#ifdef PI_READ_SUPPORT_COMMENT

/*
**		内蔵コメントをコメントバッファに読み込む
*/
static void FASTCALL
 pi_read_comment(pi_structp pi_ptr, pi_int discard)
{
	enum { BUFSIZE = 256 };
	pi_textp curr = &pi_ptr->comment;
	pi_uint count = 0;
	pi_charp text = NULL;	/* to suppress the compiler's warning */
	pi_char c;

	while ((c = pi_read_byte(pi_ptr)) != '\x1A' && !PI_ISERR(pi_ptr)) {
		if (discard) continue;
		if (count == 0) {
			curr->next = pi_alloc_memory(pi_ptr, sizeof(pi_text) + BUFSIZE);
			PI_CHKERR_RETURN(pi_ptr);
			curr = curr->next;
			curr->next = NULL;
			curr->size = count = BUFSIZE;
			curr->text = text = (pi_charp)(curr + 1);
			pi_ptr->comment.size += BUFSIZE;
		}
		*text++ = c;
		count--;
	}
	curr->size -= count;
	pi_ptr->comment.size -= count;
}


/*
**		内蔵コメントをコメントバッファからコピーする
*/
void pi_read_get_comment(pi_structp pi_ptr, pi_charp buf, pi_int freebuf)
{
	pi_textp curr;

	for (curr = pi_ptr->comment.next; curr != NULL; curr = curr->next) {
		pi_memcpy(buf, curr->text, curr->size);
		buf += curr->size;
	}
	*buf = '\0';

	if (freebuf)
		pi_read_free_comment(pi_ptr);
}


/*
**		コメントバッファのメモリを解放する
*/
static void FASTCALL
 pi_read_free_comment(pi_structp pi_ptr)
{
	pi_textp curr, next;

	for (curr = pi_ptr->comment.next; curr != NULL; curr = next) {
		next = curr->next;
		pi_free_memory(pi_ptr, curr);
	}
	pi_ptr->comment.next = NULL;
	pi_ptr->comment.size = 0;
}

#endif /* PI_READ_SUPPORT_COMMENT */



/* ***********************************************************************
**		パレットの読み込み
*/

	/* カラー構造体アクセス用マクロ(デフォルト定義) */
#ifndef pi_set_color
#define pi_set_color(p,r,g,b) \
            ((p)->red = (r), (p)->green = (g), (p)->blue = (b))
#endif

/*
**		パレットを読み込む
*/
void pi_read_palette(pi_structp pi_ptr, pi_colorp pal)
{
	static const pi_byte def4pal[2][2] = {
		{0x00, 0x77}, {0x00, 0xFF}	/* 16色デフォルトパレット */
	};
	static const pi_byte def8pal2[4] = {
		0x00, 0x55, 0xAA, 0xFF	/* 256色デフォルトパレット(B) */
	};
	static const pi_byte def8pal3[8] = {
		0x00, 0x24, 0x49, 0x6D,	/* 256色デフォルトパレット(R,G) */
		0x92, 0xB6, 0xDB, 0xFF
	};
	pi_byte buf[3];
	pi_uint i, j;

	PI_CHKERR_RETURN(pi_ptr);

	if (pi_ptr->mode & PI_MODE_NO_PALETTE) {
		/* デフォルトパレット使用 */
		if (pi_ptr->bitdepth == 8) {
			for (i = 0; i < 256; i++) {
				pi_set_color(pal, def8pal3[(i >> 2) & 0x07],
				                  def8pal3[(i >> 5) & 0x07],
				                  def8pal2[(i >> 0) & 0x03]);
				pal++;
			}
		} else {
			for (i = 0; i < 16; i++) {
				j = i >> 3;
				pi_set_color(pal, def4pal[j][(i >> 1) & 0x01],
				                  def4pal[j][(i >> 2) & 0x01],
				                  def4pal[j][(i >> 0) & 0x01]);
				pal++;
			}
		}
	} else {
		for (i = pi_ptr->colors; i > 0; i--) {
			pi_read_bytes(pi_ptr, buf, 3);
			pi_set_color(pal, buf[0], buf[1], buf[2]);
			pal++;
		}
	}
}



/* ***********************************************************************
**		イメージの読み出し準備＆後始末
*/

#define PI_POS_COLOR	(6)
#define PI_POS_EMPTY	(0x0F)
#define PI_LEN_EMPTY	(0)

#define pi_read_init_bitbuf(p)  ((p)->bitcnt = 0)
#define pi_read_init_poslen(p)  ((p)->prevpos = PI_POS_COLOR, \
            (p)->savedpos = PI_POS_EMPTY, (p)->savedlen = PI_LEN_EMPTY)

#define pi_read_free_rowbuf(p)      pi_free_memory(p,(p)->rowbuf)
#define pi_read_free_color_table(p) pi_free_memory(p,(p)->clrtable)
#define pi_read_free_colorcode_table(p) do { pi_free_memory(p,(p)->clrcode); \
            pi_free_memory(p,(p)->clrcodelen); } while (0)
#define pi_read_free_lencode_table(p)   do { pi_free_memory(p,(p)->lencode); \
            pi_free_memory(p,(p)->lencodelen); } while (0)


/*
**		イメージの読み出し開始(準備)
*/
void pi_read_row_init(pi_structp pi_ptr)
{
	pi_read_init_color_table(pi_ptr);
	pi_read_init_colorcode_table(pi_ptr);
	pi_read_init_lencode_table(pi_ptr);
	pi_read_init_rowbuf(pi_ptr);
	pi_read_init_poslen(pi_ptr);
	pi_read_init_bitbuf(pi_ptr);
}


/*
**		イメージの読み込み終了(後始末)
*/
static void FASTCALL
 pi_read_row_end(pi_structp pi_ptr)
{
	pi_read_free_color_table(pi_ptr);
	pi_read_free_colorcode_table(pi_ptr);
	pi_read_free_lencode_table(pi_ptr);
	pi_read_free_rowbuf(pi_ptr);
}


/* -----------------------------------------------------------------------
**		各種コードテーブル＆作業用バッファの初期化
*/

/*
**		カラーテーブルの初期化
**
**	==============================================
**	| 左隣の色 | カラーテーブル (16色用)
**	+----------+----------------------------------
**	|     0    | 0,F,E,D,C,B,A,9,8,7,6,5,4,3,2,1
**	|     1    | 1,0,F,E,D,C,B,A,9,8,7,6,5,4,3,2
**	|     2    | 2,1,0,F,E,D,C,B,A,9,8,7,6,5,4,3
**	|     3    | 3,2,1,0,F,E,D,C,B,A,9,8,7,6,5,4
**	|     4    | 4,3,2,1,0,F,E,D,C,B,A,9,8,7,6,5
**	|     5    | 5,4,3,2,1,0,F,E,D,C,B,A,9,8,7,6
**	|     6    | 6,5,4,3,2,1,0,F,E,D,C,B,A,9,8,7
**	|     7    | 7,6,5,4,3,2,1,0,F,E,D,C,B,A,9,8
**	|     8    | 8,7,6,5,4,3,2,1,0,F,E,D,C,B,A,9
**	|     9    | 9,8,7,6,5,4,3,2,1,0,F,E,D,C,B,A
**	|     A    | A,9,8,7,6,5,4,3,2,1,0,F,E,D,C,B
**	|     B    | B,A,9,8,7,6,5,4,3,2,1,0,F,E,D,C
**	|     C    | C,B,A,9,8,7,6,5,4,3,2,1,0,F,E,D
**	|     D    | D,C,B,A,9,8,7,6,5,4,3,2,1,0,F,E
**	|     E    | E,D,C,B,A,9,8,7,6,5,4,3,2,1,0,F
**	|     F    | F,E,D,C,B,A,9,8,7,6,5,4,3,2,1,0
**	==============================================
*/
static void FASTCALL
 pi_read_init_color_table(pi_structp pi_ptr)
{
	pi_bytep table;
	pi_uint i, j, m, n;

	PI_CHKERR_RETURN(pi_ptr);

	n = pi_ptr->colors;
	pi_ptr->clrtable = table = pi_alloc_memory(pi_ptr, (pi_size_t)n * n);
	PI_CHKERR_RETURN(pi_ptr);

	m = n - 1;
	for (i = 0; i < n; i++)
		for (j = i + n; j > i; j--)
			*table++ = (pi_byte)(j & m);
}


/*
**		カラーコードテーブルの初期化
**
**	======================================
**	|   色コード |符号長| 符号 (16色用)
**	+------------+------+-----------------
**	|    0～1    |   2  | 1x
**	|    2～3    |   3  | 00x
**	|    4～7    |   5  | 010xx
**	|    8～15   |   6  | 011xxx
**	======================================
**	|   色コード |符号長| 符号 (256色用)
**	+------------+------+-----------------
**	|    0～  1  |   2  | 1x
**	|    2～  3  |   3  | 00x
**	|    4～  7  |   5  | 010xx
**	|    8～ 15  |   7  | 0110xxx
**	|   16～ 31  |   9  | 01110xxxx
**	|   32～ 63  |  11  | 011110xxxxx
**	|   64～127  |  13  | 0111110xxxxxx
**	|  128～255  |  14  | 0111111xxxxxxx
**	======================================
*/
static void FASTCALL
 pi_read_init_colorcode_table(pi_structp pi_ptr)
{
	pi_bytep code, codelen;
	pi_uint i, b, s, c, bits, size;
	pi_byte l;

	PI_CHKERR_RETURN(pi_ptr);

	bits = (pi_ptr->bitdepth == 8) ? PI_READ_CLRCODE_TABLE_BITS : 6;
	size = 1 << bits;
	pi_ptr->clrcode    = code    = pi_alloc_memory(pi_ptr, size);
	pi_ptr->clrcodelen = codelen = pi_alloc_memory(pi_ptr, pi_ptr->colors);
	PI_CHKERR_RETURN(pi_ptr);

	s = pi_ptr->bitdepth - 2;

	for (i = 0; (c = i) < size; i++) {
		if ((c <<= 1) & size) {
			c >>= bits - 1;
			c  &= 1;
			l = 2;
		} else {
			for (b = 1; b <= s && ((c <<= 1) & size); b++) ;
			c  &= size - 1;
			c  |= size;
			c >>= bits - b;
			l = 2 * b + (b <= s);
		}
		code[i]    = c;
		codelen[c] = l;
	}
}


/*
**		連鎖数コードテーブルの初期化
**
**	===================================================
**	|   連鎖数   |符号長| 符号
**	+------------+------+---------+--------------------
**	|    1       |   1  | 0
**	|    2～   3 |   3  | 10x
**	|    4～   7 |   5  | 110xx
**	|    8～  15 |   7  | 1110xxx
**	|   16～  31 |   9  | 11110xxx x
**	|   32～  63 |  11  | 111110xx xxx
**	|   64～ 127 |  13  | 1111110x xxxxx
**	|  128～ 255 |  15  | 11111110 xxxxxxx
**	+------------+------+---------+--------------------
**	|  256～ 511 |  17  | 11111111 0 xxxxxxxx
**	|  512～1023 |  19  | 11111111 10x xxxxxxxx
**	| 1024～2047 |  21  | 11111111 110xx xxxxxxxx
**	| 2048～4095 |  23  | 11111111 1110xxx xxxxxxxx
**	| 4096～8191 |  25  | 11111111 11110xxxx xxxxxxxx
**	|    ....    |  ..  | ....
**	===================================================
*/
static void FASTCALL
 pi_read_init_lencode_table(pi_structp pi_ptr)
{
	enum { BITS = PI_READ_LENCODE_TABLE_BITS, SIZE = 1 << BITS };
	pi_bytep code, codelen;
	pi_uint i, b, c;

	PI_CHKERR_RETURN(pi_ptr);

	pi_ptr->lencode    = code    = pi_alloc_memory(pi_ptr, SIZE);
	pi_ptr->lencodelen = codelen = pi_alloc_memory(pi_ptr, SIZE / 2 + 1);
	PI_CHKERR_RETURN(pi_ptr);

	for (i = 0; (c = i) < SIZE; i++) {
		for (b = 0; (c <<= 1) & SIZE; b++) ;
		c  &= SIZE - 1;
		c  |= SIZE;
		c >>= BITS - b;
		c  &= SIZE - 1;		/* デコード不能(i == SIZE-1)の時は c = 0 とする */
		code[i]    = c;
		codelen[c] = 2 * b + 1;
	}
}


/*
**		行バッファの初期化
*/
static void FASTCALL
 pi_read_init_rowbuf(pi_structp pi_ptr)
{
	pi_bytep row;
	pi_uint i;

	PI_CHKERR_RETURN(pi_ptr);

	/*
	 *	４行分の行バッファを使うのは、.pi の処理単位(2pixels)が
	 *	行バッファの切れ目にまたがらないようにするため。こうすると、
	 *	画像の幅が奇数であるような画像もうまく扱うことができる。
	 *	残りの 6pixels は、行バッファの切れ目の影響を少なくする
	 *	ためのもの。
	 */
	pi_ptr->rowbuf = pi_alloc_memory(pi_ptr, (pi_size_t)pi_ptr->width * 4 + 6);
	PI_CHKERR_RETURN(pi_ptr);

	row = pi_ptr->rowbuf + 4;
	for (i = 4; i > 0; i--) {
		pi_ptr->rowptr[(i + 1) % 4] = row;	/* 1 0 3 2 */
		row += pi_ptr->width;
	}
	pi_ptr->currentp = pi_ptr->rowptr[3];
	pi_ptr->rownum   = 0;
}



/* ***********************************************************************
**		イメージ本体の読み込み
*/

#ifdef PI_READ_OUTPUT_8BPP_FMT
#define pi_read_output_row(p,r)  pi_memcpy((r),(p)->rowptr[0],(p)->width)
#else
#define pi_read_output_row(p,r)  do { \
            if ((p)->bitdepth == 8) pi_memcpy((r),(p)->rowptr[0],(p)->width); \
            else pi_read_rowcpy4((r),(p)->rowptr[0],(p)->width); } while (0)
#endif /* PI_READ_OUTPUT_8BPP_FMT */

#ifdef PI_READ_SUPPORT_READIMAGE

/*
**		イメージ全体を一度に読み込む
*/
void pi_read_image(pi_structp pi_ptr, pi_bytepp rowptr)
{
	pi_read_row_init(pi_ptr);
	while (pi_read_row(pi_ptr, *rowptr++)) ;
}

#endif /* PI_READ_SUPPORT_READIMAGE */

/*
**		イメージの一行を読み込む
**
**		行の読み込みに成功した場合は 1 を返す。エラーが発生した
**		場合や、これ以上読むべき行がない場合は 0 を返す。
*/
int pi_read_row(pi_structp pi_ptr, pi_bytep row)
{
	PI_CHKERR_RETURN_VAL(pi_ptr, 0);

	if (pi_ptr->rownum >= pi_ptr->height) return 0;		/* end */

	pi_read_rotate_rowbuf(pi_ptr);

#ifdef PI_READ_W2_SPEC_COMPLIANT_DECODING
	if (pi_ptr->width <= 2) {
		pi_read_pixels_width2(pi_ptr);
	} else {
		if (pi_ptr->rownum == 1)
			pi_read_initial_colors(pi_ptr);

		pi_read_pixels(pi_ptr);
	}
#else
	if (pi_ptr->rownum == 1)
		pi_read_initial_colors(pi_ptr);

	pi_read_pixels(pi_ptr);
#endif
	pi_read_output_row(pi_ptr, row);

#ifdef PI_SUPPORT_PROGRESS_CALLBACK
	if (pi_ptr->progfunc != NULL)
		if ((*pi_ptr->progfunc)(pi_ptr, pi_ptr->rownum, pi_ptr->height) != 0)
			PI_SETERR_RETURN_VAL(pi_ptr, PI_ERR_CALLBACK_CANCELED, 0);
#endif
	return !PI_ISERR(pi_ptr);
}

#ifndef PI_READ_OUTPUT_8BPP_FMT

/*
**		イメージの一行をコピーする(16色画像用)
*/
static void FASTCALL
 pi_read_rowcpy4(pi_bytep dst, pi_bytep src, pi_size_t cnt)
{
	for ( ; cnt > 1; cnt -= 2) {
		dst[0] = (src[0] << 4) | (src[1] & 0x0F);
		src += 2; dst += 1;
	}
	if (cnt > 0) {
		dst[0] = (src[0] << 4);
	}
}

#endif /* PI_READ_OUTPUT_8BPP_FMT */

/*
**		初期カラー(2色)をファイルから読み、上２行分の行バッファを初期化する
*/
static void FASTCALL
 pi_read_initial_colors(pi_structp pi_ptr)
{
	pi_bytep p = pi_ptr->currentp;
	pi_uint c, d, i;

	c = pi_read_color(pi_ptr, 0);
	d = pi_read_color(pi_ptr, c);

	for (i = pi_ptr->width; i > 0; i--) {
		*(--p) = d;
		*(--p) = c;
		/*
		 *	上２行分の行バッファが連続している必要あり。
		 *	(see pi_read_init_rowbuf)
		 */
	}
}


/*
**		行バッファのローテート
*/
static void FASTCALL
 pi_read_rotate_rowbuf(pi_structp pi_ptr)
{
	static const pi_ptrdif_t posx[] = { -2,-4,+0,+0,+1,-1 };
	static const pi_int      posy[] = {  0, 0, 1, 2, 1, 1 };
	pi_bytep row0;
	pi_uint32 w;
	pi_uint i;

	row0 = pi_ptr->rowptr[3];
	pi_ptr->rowptr[3] = pi_ptr->rowptr[2];
	pi_ptr->rowptr[2] = pi_ptr->rowptr[1];
	pi_ptr->rowptr[1] = pi_ptr->rowptr[0];
	pi_ptr->rowptr[0] = row0;
	pi_ptr->rowend = row0 + pi_ptr->width;
	pi_ptr->rownum++;

	for (i = 0; i < 6; i++)
		pi_ptr->posdiff[i] = pi_ptr->rowptr[posy[i]] - row0 + posx[i];

		/* 重ね合わせを行なうため、２ピクセル多く読む。 */
	if (pi_ptr->rowptr[0] > pi_ptr->rowptr[3] &&
	    pi_ptr->rownum < pi_ptr->height) {
		pi_ptr->rowend += 2;
	}
		/* 行バッファの切れ目の前後(6pixels)を重ね合わせる。 */
	if (pi_ptr->rowptr[1] > pi_ptr->rowptr[0]) {
		pi_ptr->currentp -= (w = pi_ptr->width * 4);
#ifdef PI_READ_W2_SPEC_COMPLIANT_DECODING
		/* memmove で統一しても良いんだけど、memcpy を
		 * インライン化してくれるコンパイラが多いので...。*/
		pi_memcpy(pi_ptr->rowbuf,  (pi_ptr->rowbuf + w), 6);
#else
		/* 幅が1pixelの場合にコピー元とコピー先が重なるので、
		 * memmove を使っている。*/
		pi_memmove(pi_ptr->rowbuf, (pi_ptr->rowbuf + w), 6);
#endif
	}
}


/* -----------------------------------------------------------------------
**		ピクセルデータの読み込み
*/

	/* ピクセルのコピーを行なうマクロ */
#ifdef PI_WORD_OPERATION_ON_PIXEL
/* プロセッサによっては、奇数アドレスに対して16bitアクセスを行なうと遅く
 * なる場合があるため、この方法が常に速いとは限らない。
 * さらに、x86以外のプロセッサでは、アドレスのアラインメントが合わない
 * メモリアクセスそのものができないものもある。 */
#define pi_cpypixel(d,s)  (((pi_uint16p)(d))[0] = ((pi_uint16p)(s))[0])
#else
#define pi_cpypixel(d,s)  ((d)[0] = (s)[0], (d)[1] = (s)[1])
#endif

	/* ビットバッファ関係マクロ */
#define pi_getbits(p,n)   \
            (((p)->bitcnt < (pi_uint)(n)) ? pi_read_fill_bitbuf(p):(void)0, \
             (pi_uint)((p)->bitbuf >> ((p)->bitcnt -= (n))) & pi_bit1(n))
#define pi_peekbits(p,n)  \
            (((p)->bitcnt < (pi_uint)(n)) ? pi_read_fill_bitbuf(p):(void)0, \
             (pi_uint)((p)->bitbuf >> ((p)->bitcnt -  (n))) & pi_bit1(n))
#define pi_dropbits(p,n)  ((p)->bitcnt -= (n))
#define pi_bit1(n)        (((pi_uint)1 << (n)) - 1)

#define pi_read_save_len(p,l)  ((p)->savedlen = (l))
#define pi_read_save_pos(p,s)  ((p)->savedpos = (s))


/*
**		ピクセルデータを読み、行バッファを埋める
*/
static void FASTCALL
 pi_read_pixels(pi_structp pi_ptr)
{
	register pi_bytep curp = pi_ptr->currentp;
	pi_bytep endp = pi_ptr->rowend;
	pi_uint pos;

#ifndef PI_READ_W2_SPEC_COMPLIANT_DECODING
	/* 幅が2pixel以下の場合に起こりえる */
	if (curp >= endp) return;
#endif
	for (;;) {
		pos = pi_read_pos(pi_ptr, curp);
		if (pos != PI_POS_COLOR) {
			register pi_uint32 len = pi_read_len(pi_ptr, 0);
			register pi_bytep srcp = curp + pi_ptr->posdiff[pos];
			do {
				pi_cpypixel(curp, srcp);
				srcp += 2; curp += 2;
				if (curp >= endp) goto endl1;
			} while (--len > 0);
			continue;
		/* --- */
		endl1:
			if (--len > 0) {
				pi_read_save_pos(pi_ptr, pos);
				pi_read_save_len(pi_ptr, len);
			}
			break;
		} else {
			pi_uint color = curp[-1];
			do {
				*curp++ = color = pi_read_color(pi_ptr, color);
				*curp++ = color = pi_read_color(pi_ptr, color);
				if (curp >= endp) goto endl2;
			} while (pi_getbits(pi_ptr, 1));
			continue;
		/* --- */
		endl2:
			if (pi_ptr->rownum < pi_ptr->height &&
			    pi_getbits(pi_ptr, 1)) {
				pi_read_save_pos(pi_ptr, PI_POS_COLOR);
			}
			break;
		}
	}
	pi_ptr->currentp = curp;
}

#ifdef PI_READ_W2_SPEC_COMPLIANT_DECODING

/*
**		ピクセルデータを読み、行バッファを埋める(width <= 2)
**
**		piwrite.c の中の「幅が2pixel以下の画像の処理法」と
**		pi_write_pixels_width2 の解説(コメント)も見てください。
**		ここでは、piwrite.c と同様に、gimp-jp-plugins-20010820
**		に含まれる Pi プラグイン (pi.c, PI plug-in for GIMP) と
**		互換になるようにしてあります。
*/
static void FASTCALL
 pi_read_pixels_width2(pi_structp pi_ptr)
{
	pi_bytep curp = pi_ptr->currentp;
	pi_uint color = (pi_ptr->rownum == 1) ? 0 : curp[-1];

	while (curp < pi_ptr->rowend)
		*curp++ = color = pi_read_color(pi_ptr, color);

	pi_ptr->currentp = curp;
}

#endif /* PI_READ_W2_SPEC_COMPLIANT_DECODING */

/*
**		位置コードを読む
*/
static pi_uint FASTCALL
 pi_read_pos(pi_structp pi_ptr, pi_bytep curp)
{
	enum { TABLE_BITS = 3 };
	static const pi_byte poscode[]    = { 0, 0, 1, 1, 2, 2, 3, 4 };
	static const pi_byte poscodelen[] = { 2, 2, 2, 3, 3 };
	pi_uint pos;

	if ((pos = pi_ptr->savedpos) != PI_POS_EMPTY) {
		pi_ptr->savedpos = PI_POS_EMPTY;
		return pos;
	}

	pos = poscode[pi_peekbits(pi_ptr, TABLE_BITS)];
	pi_dropbits(pi_ptr, poscodelen[pos]);

	if (pos == pi_ptr->prevpos) {
		pos = PI_POS_COLOR;
		pi_ptr->prevpos = pos;
	} else {
		pi_ptr->prevpos = pos;
		if (pos != 0 || curp[-1] != curp[-2]) pos++;
	}

	return pos;
}


/*
**		色を読む
*/
static pi_uint FASTCALL
 pi_read_color(pi_structp pi_ptr, pi_uint prev)
{
	enum { TABLE_BITS = PI_READ_CLRCODE_TABLE_BITS,
	       THRESHOLD  = PI_READ_COLOR_THRESHOLD };
	pi_uint n, l;
	pi_bytep tbl, p, q;
	pi_byte c;

	n = pi_ptr->clrcode[pi_peekbits(pi_ptr,
	                    (pi_ptr->bitdepth == 8) ? TABLE_BITS : 6)];
	l = pi_ptr->clrcodelen[n];
	if (TABLE_BITS < 14 && l > TABLE_BITS) {
		pi_dropbits(pi_ptr, TABLE_BITS);
		l -= TABLE_BITS;
		n |= pi_peekbits(pi_ptr, l);
	}
	pi_dropbits(pi_ptr, l);

	tbl = pi_ptr->clrtable + pi_ptr->colors * prev;

	if (n == 0) {
		c = tbl[0];
	} else if (n < THRESHOLD) {
		p = tbl + n;
		q = p - 1;
		c = *p;
		do { *p-- = *q--; } while (--n > 0);
		*p = c;
	} else {
		c = tbl[n];
		pi_memmove(tbl + 1, tbl, n);
		tbl[0] = c;
	}

	return c;
}


/*
**		連鎖数を読む
*/
static pi_uint32 FASTCALL
 pi_read_len(pi_structp pi_ptr, pi_uint bits)
{
	enum { TABLE_BITS = PI_READ_LENCODE_TABLE_BITS };
	pi_uint32 n;
	pi_uint l;

	if ((n = pi_ptr->savedlen) != PI_LEN_EMPTY) {
		pi_ptr->savedlen = PI_LEN_EMPTY;
		return n;
	}

	n = pi_ptr->lencode[pi_peekbits(pi_ptr, TABLE_BITS)];

	if (n == 0) {
		pi_dropbits(pi_ptr, TABLE_BITS);
		/* 11111111... というビットパターンが続くと、再帰呼び出しを
		 * 延々と続けてしまうので、途中でストップさせる必要がある。*/
		if ((bits += TABLE_BITS) < bitsof(pi_uint32))
			n = pi_read_len(pi_ptr, bits) << TABLE_BITS;
		else	/* ↓最終結果を UINT32_MAX に飽和させる代わりに、*/
			n = (~0);		/* 最終結果の上位ビットを１にする。  */
		n |= pi_getbits(pi_ptr, TABLE_BITS);
	} else {
		l = pi_ptr->lencodelen[n];
		if (l > TABLE_BITS) {
			pi_dropbits(pi_ptr, TABLE_BITS);
			l -= TABLE_BITS;
			n |= pi_peekbits(pi_ptr, l);
		}
		pi_dropbits(pi_ptr, l);
	}

	return n;
}



/* ***********************************************************************
**		データ入力関数 (ビットバッファ経由)
*/

/*
**		ビットバッファに新たなデータを読み込む
*/
static void FASTCALL
 pi_read_fill_bitbuf(pi_structp pi_ptr)
{
	register pi_bytep  iobufptr = pi_ptr->iobufptr;
	register pi_size_t iobufcnt = pi_ptr->iobufcnt;
	register pi_bitbuf bitbuf   = pi_ptr->bitbuf;
	register pi_uint   bitcnt   = pi_ptr->bitcnt;

	while (bitcnt < bitsof(pi_bitbuf) - 7) {
		if (iobufcnt == 0) {
			/*
			 * ビットバッファの大きさが40ビットを超える場合、
			 * .pi ファイルの末尾にあるダミーの32ビットだけでは
			 * ビットバッファを満たすことができない場合が出てくる
			 * (EOFエラーとなる)。そのため、入力バッファが空に
			 * なった時点で十分な有効ビットが既にあるならば、
			 * ビットバッファを満たさずにそのままリターンする。
			 * 実際にこれが問題になるのは bitsof(pi_bitbuf) > 40
			 * の場合であるが、ここでは１バイト分余裕を見ている。
			 */
			if (bitsof(pi_bitbuf) > 32 && bitcnt >= 32 - 7) break;
			pi_ptr->iobufptr = iobufptr;
			pi_ptr->iobufcnt = iobufcnt;
			pi_read_fill_buffer(pi_ptr);
			iobufptr = pi_ptr->iobufptr;
			iobufcnt = pi_ptr->iobufcnt;
		}
		bitcnt += 8;
		iobufcnt--;
		bitbuf = (bitbuf << 8) | *iobufptr++;
	}
	pi_ptr->iobufptr = iobufptr;
	pi_ptr->iobufcnt = iobufcnt;
	pi_ptr->bitbuf   = bitbuf;
	pi_ptr->bitcnt   = bitcnt;
}



/* ***********************************************************************
**		データ入力関数 (バイト単位)
*/

/*
**		読み込みバッファに新たなデータを読み込む
*/
static void FASTCALL
 pi_read_fill_buffer(pi_structp pi_ptr)
{
#ifdef PI_SUPPORT_LONGJMP
	if ((*pi_ptr->iofunc)(pi_ptr) == 0)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_END_OF_FILE);
#else
	static const pi_byte nulldata[16] = { 0 };

	if (pi_ptr->iofunc == NULL || (*pi_ptr->iofunc)(pi_ptr) == 0) {
		pi_ptr->iofunc = NULL;
		pi_ptr->iobufptr = (pi_bytep)nulldata;
		pi_ptr->iobufcnt = sizeof(nulldata);
		PI_SETERR_RETURN(pi_ptr, PI_ERR_END_OF_FILE);
	}
#endif
}


/*
**		読み込みバッファから 1 バイト読む
*/
static pi_uint FASTCALL
 pi_read_byte(pi_structp pi_ptr)
{
	if (pi_ptr->iobufcnt == 0) pi_read_fill_buffer(pi_ptr);
	return pi_ptr->iobufcnt--, *(pi_ptr->iobufptr++);
}


/*
**		読み込みバッファから n バイト読む
*/
static void FASTCALL
 pi_read_bytes(pi_structp pi_ptr, pi_bytep buf, pi_size_t cnt)
{
	pi_size_t n;

	for (;;) {
		if (pi_ptr->iobufcnt > 0) {
			n = (pi_ptr->iobufcnt < cnt) ? pi_ptr->iobufcnt : cnt;
			pi_memcpy(buf, pi_ptr->iobufptr, n);
			pi_ptr->iobufptr += n;
			pi_ptr->iobufcnt -= n;
			buf += n;
			cnt -= n;
			if (cnt == 0) break;
		}
		pi_read_fill_buffer(pi_ptr);
	}
}



/* ***********************************************************************
**		メモリの確保＆解放
*/

/*
**		メモリの確保(チェック付き)
*/
static pi_voidp FASTCALL
 pi_alloc_memory(pi_structp pi_ptr, pi_size_t size)
{
	pi_voidp ptr = pi_malloc(size);

	if (ptr == NULL)
		PI_SETERR_RETURN_VAL(pi_ptr, PI_ERR_OUT_OF_MEMORY, NULL);

	return ptr;
}


/*
**		メモリの解放
*/
static void FASTCALL
 pi_free_memory(pi_structp pi_ptr, pi_voidp ptr)
{
	if (ptr != NULL) pi_free(ptr);
}



/* ***********************************************************************
**		Ｃ言語標準 I/O による入力サポート関数群
*/

#ifdef PI_READ_SUPPORT_STDIO

/*
**		データ読み込みの準備
*/
void pi_read_init_io(pi_structp pi_ptr, FILE *fp)
{
	pi_stdiop ioptr;

	pi_ptr->ioptr = ioptr = pi_alloc_memory(pi_ptr, sizeof(pi_stdio));
	PI_CHKERR_RETURN(pi_ptr);

	pi_ptr->iofunc = pi_read_iofunc_stdio;
	pi_ptr->iobufptr = ioptr->iobuf;
	pi_ptr->iobufcnt = 0;
	ioptr->fp = fp;
}


/*
**		データ読み込み終了(後始末)
*/
void pi_read_end_io(pi_structp pi_ptr)
{
	pi_free_memory(pi_ptr, pi_ptr->ioptr);
}


/*
**		データ読み込み関数(stdio)
*/
static pi_size_t
 pi_read_iofunc_stdio(pi_structp pi_ptr)
{
	pi_stdiop ioptr = pi_ptr->ioptr;

	pi_ptr->iobufcnt = fread(ioptr->iobuf, 1, sizeof(ioptr->iobuf), ioptr->fp);
	pi_ptr->iobufptr = ioptr->iobuf;

	if (ferror(ioptr->fp))
		PI_SETERR_RETURN_VAL(pi_ptr, PI_ERR_FILE_READ, pi_ptr->iobufcnt);

	return pi_ptr->iobufcnt;
}

#endif /* PI_READ_SUPPORT_STDIO */

