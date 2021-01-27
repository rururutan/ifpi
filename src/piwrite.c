/*
**  piwrite.c - .pi 書き出しライブラリ ver.3.10 (Sep 6, 2004)
**
**  Copyright(C) 1999-2004 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
**
**  For conditions of distribution and use, see copyright notice in pilib.h
**
**  使用・配布条件については、pilib.h の中の著作権表示を見てください。
*/

#define PI_INTERNAL
#include "pilib.h"

	/* カラーコードテーブルのビット数 (7～CHAR_BIT) */
#define PI_WRITE_CLRCODE_TABLE_BITS		CHAR_BIT

	/* 連鎖数コードテーブルのビット数 (1～CHAR_BIT) */
#define PI_WRITE_LENCODE_TABLE_BITS		CHAR_BIT


	/* プロトタイプ宣言 */
static void FASTCALL pi_set_uint16(pi_bytep ptr, pi_uint);
#ifdef PI_WRITE_SUPPORT_EXTINFO
static void FASTCALL pi_write_extinfo(pi_structp);
static void FASTCALL pi_set_uint32(pi_bytep ptr, pi_uint32);
#endif
#ifdef PI_WRITE_SUPPORT_COMMENT
static void FASTCALL pi_write_comment(pi_structp);
#endif
static void FASTCALL pi_write_row_end(pi_structp);
static void FASTCALL pi_write_init_color_table(pi_structp);
static void FASTCALL pi_write_init_colorcode_table(pi_structp);
static void FASTCALL pi_write_init_lencode_table(pi_structp);
static void FASTCALL pi_write_init_rowbuf(pi_structp);
static void FASTCALL pi_write_rowcpy4(pi_bytep, pi_bytep, pi_size_t);
static void FASTCALL pi_write_initial_colors(pi_structp);
static void FASTCALL pi_write_rotate_rowbuf(pi_structp);
static void FASTCALL pi_write_pixels(pi_structp);
#ifdef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING
static void FASTCALL pi_write_pixels_width2(pi_structp);
#endif
static void FASTCALL pi_write_pending_data(pi_structp);
static pi_uint FASTCALL pi_write_find_pixels(pi_structp, pi_uint, pi_bytep);
static pi_uint FASTCALL pi_write_pos(pi_structp, pi_uint);
static void FASTCALL pi_write_color(pi_structp, pi_uint, pi_uint);
static void FASTCALL pi_write_len(pi_structp, pi_uint32);
#ifdef PI_WRITE_PUTBITS_FUNC
static void FASTCALL pi_write_bits(pi_structp, pi_uint, pi_uint);
#endif
static void FASTCALL pi_write_flush_bitbuf(pi_structp);
static void FASTCALL pi_write_flush_buffer(pi_structp);
static void FASTCALL pi_write_bytes(pi_structp, pi_bytep, pi_size_t);
static pi_voidp FASTCALL pi_alloc_memory(pi_structp, pi_size_t);
static void FASTCALL pi_free_memory(pi_structp, pi_voidp);
#ifdef PI_WRITE_SUPPORT_STDIO
static pi_size_t pi_write_iofunc_stdio(pi_structp);
#endif




/* ***********************************************************************
**		pi_struct 構造体の初期化・終了処理
*/

/*
**		構造体の初期化
*/
void pi_write_init(pi_structp pi_ptr)
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
#ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_ptr->transcolor = PI_TRANSCOLOR_NONE;
#endif
}


/*
**		構造体に関連づけられているメモリを解放する(終了処理)
*/
void pi_write_end(pi_structp pi_ptr)
{
	pi_write_row_end(pi_ptr);
}



/* ***********************************************************************
**		ファイルヘッダの書き込み
*/

/*
**		ヘッダを書き込む
*/
void pi_write_header(pi_structp pi_ptr)
{
	static const pi_byte templete[18] = "Pi\x1a\0\0\0\0\0    ";
	pi_byte hed[18];

	PI_CHKERR_RETURN(pi_ptr);

	if (pi_ptr->bitdepth != 4 && pi_ptr->bitdepth != 8)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_BITDEPTH);
	if (pi_ptr->width  == 0 || pi_ptr->width  > 0xFFFF)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_WIDTH);
	if (pi_ptr->height == 0 || pi_ptr->height > 0xFFFF)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_INVALID_HEIGHT);

	pi_ptr->colors = 1 << pi_ptr->bitdepth;		/* 色数(16 or 256) */

	pi_memcpy(hed, templete, sizeof(templete));

	if (pi_ptr->machine[0] != '\0')
		pi_memcpy(hed+8, pi_ptr->machine, 4);	/* 機種識別子(エンコーダ名) */
	if (pi_ptr->aspect_x != 0 && pi_ptr->aspect_y != 0) {
		hed[5] = pi_ptr->aspect_x;
		hed[6] = pi_ptr->aspect_y;					/* アスペクト比 */
	}
	hed[4] = (pi_ptr->mode & PI_MODE_NO_PALETTE);	/* 画像モード */
	hed[7] = pi_ptr->bitdepth;						/* 色深度(4 or 8) */
	pi_set_uint16(hed+14, pi_ptr->width);			/* 画像の幅 */
	pi_set_uint16(hed+16, pi_ptr->height);			/* 画像の高さ */

#ifdef PI_WRITE_SUPPORT_COMMENT
# ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_write_bytes(pi_ptr, hed+0, 2);
	pi_write_comment(pi_ptr);			/* 内蔵コメント */
	pi_write_bytes(pi_ptr, hed+2, 10);
	pi_write_extinfo(pi_ptr);			/* 拡張情報 */
	pi_write_bytes(pi_ptr, hed+14, 4);
# else
	pi_write_bytes(pi_ptr, hed+0, 2);
	pi_write_comment(pi_ptr);			/* 内蔵コメント */
	pi_write_bytes(pi_ptr, hed+2, 16);
# endif
#else	/* !PI_WRITE_SUPPORT_COMMENT */
# ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_write_bytes(pi_ptr, hed+0, 12);
	pi_write_extinfo(pi_ptr);			/* 拡張情報 */
	pi_write_bytes(pi_ptr, hed+14, 4);
# else
	pi_write_bytes(pi_ptr, hed+0, 18);
# endif
#endif /* PI_WRITE_SUPPORT_COMMENT */
}


/*
**	メモリへ big-endien 形式 2バイト無符号整数を書く
*/
static void FASTCALL
 pi_set_uint16(pi_bytep ptr, pi_uint val)
{
	ptr[0] = (pi_byte)(val >> 8 & 0xFF);
	ptr[1] = (pi_byte)(val      & 0xFF);
}


/* -----------------------------------------------------------------------
**		.PI ファイル拡張情報の書き込み
*/

#ifdef PI_WRITE_SUPPORT_EXTINFO

#define pi_make_uint32(h,l) (((pi_uint32)(h)<<16) | ((pi_uint32)(l)&0xFFFF))

/*
**		拡張情報を書き込む
**
**		拡張情報のフォーマットについては piread.c の
**		pi_read_extinfo() のコメントを参照のこと。
*/
static void FASTCALL
 pi_write_extinfo(pi_structp pi_ptr)
{
	enum { TAG_SIZE = 5, TAG_NUM = 4 };
	pi_byte hed[2 + TAG_SIZE * TAG_NUM];
	pi_bytep base = hed + 2;
	pi_uint32 offset;
	pi_uint hedsize;

	if ((offset = pi_make_uint32(pi_ptr->offset_x, pi_ptr->offset_y)) != 0) {
		base[0] = 0x01;		/* 画像の表示開始点(X,Y) */
		pi_set_uint32(base+1, offset);
		base += TAG_SIZE;
	}
	if (pi_ptr->transcolor >= 0 &&
	    pi_ptr->transcolor < (pi_int)pi_ptr->colors) {
		base[0] = 0x02;		/* 透明色になるパレット番号 (0～15/255) */
		pi_set_uint32(base+1, pi_ptr->transcolor);
		base += TAG_SIZE;
	}
	if (pi_ptr->sigbits > 0 && pi_ptr->sigbits <= 8) {
		base[0] = 0x03;		/* パレットの有効ビット数 (1～8) */
		pi_set_uint32(base+1, pi_ptr->sigbits);
		base += TAG_SIZE;
	}
	if (pi_ptr->colorused > 0 &&
	    pi_ptr->colorused <= (pi_int)pi_ptr->colors) {
		base[0] = 0x04;		/* パレットの使用個数 (1～16/256) */
		pi_set_uint32(base+1, pi_ptr->colorused);
		base += TAG_SIZE;
	}
	hedsize = base - hed;
	pi_set_uint16(hed, hedsize - 2);	/* 拡張情報のサイズ */
	pi_write_bytes(pi_ptr, hed, hedsize);
}


/*
**	メモリへ big-endien 形式 4バイト無符号整数を書く
*/
static void FASTCALL
 pi_set_uint32(pi_bytep ptr, pi_uint32 val)
{
	ptr[0] = (pi_byte)(val >> 24 & 0xFF);
	ptr[1] = (pi_byte)(val >> 16 & 0xFF);
	ptr[2] = (pi_byte)(val >>  8 & 0xFF);
	ptr[3] = (pi_byte)(val       & 0xFF);
}

#endif /* PI_WRITE_SUPPORT_EXTINFO */


/* -----------------------------------------------------------------------
**		内蔵コメントの書き込み
*/

#ifdef PI_WRITE_SUPPORT_COMMENT

/*
**		内蔵コメントを書き込む
*/
static void FASTCALL
 pi_write_comment(pi_structp pi_ptr)
{
	pi_bytep text = (pi_bytep)pi_ptr->comment.text;
	pi_bytep p;

	if (text == NULL) return;

	for (p = text; *p != '\0'; p++) ;
	pi_write_bytes(pi_ptr, text, p - text);
}

#endif /* PI_WRITE_SUPPORT_COMMENT */



/* ***********************************************************************
**		パレットの書き込み
*/

	/* カラー構造体アクセス用マクロ(デフォルト定義) */
#ifndef pi_get_color
#define pi_get_color(p,r,g,b) \
            (*(r) = (p)->red, *(g) = (p)->green, *(b) = (p)->blue)
#endif

/*
**		パレットを書き込む
*/
void pi_write_palette(pi_structp pi_ptr, pi_colorp pal /*, pi_int palnum*/)
{
	pi_byte buf[3];
	pi_uint i;

	PI_CHKERR_RETURN(pi_ptr);

	if (pi_ptr->mode & PI_MODE_NO_PALETTE) return;
/*	if (palnum < 0) palnum = (pi_int)pi_ptr->colors;*/

	for (i = pi_ptr->colors; i > 0; i--) {
	/*	if (--palnum < 0) {
			buf[0] = buf[1] = buf[2] = 0;
		} else*/ {
			pi_get_color(pal, buf+0, buf+1, buf+2);
			pal++;
		}
		pi_write_bytes(pi_ptr, buf, 3);
	}
}



/* ***********************************************************************
**		イメージの書き込み準備＆後始末
*/

	/* 位置コードのビットフラグ */
#define PI_POS_NONE		0x00
#define PI_POS_0A		0x01	/* ２ドット左の２ドット(１単位前が同色) */
#define PI_POS_0B		0x02	/* ４ドット左の２ドット(１単位前が異色) */
#define PI_POS_0		0x03
#define PI_POS_1		0x04	/* １ライン上の２ドット */
#define PI_POS_2		0x08	/* ２ライン上の２ドット */
#define PI_POS_3		0x10	/* １ライン上の１ドット右の２ドット */
#define PI_POS_4		0x20	/* １ライン上の１ドット左の２ドット */
#define PI_POS_ALL		0x3F

/*
 * 幅が2pixel以下の画像の処理法：
 * 
 * Pi仕様書(PITECH.TXT)によると、幅が2pixel以下の画像をPiに圧縮する場合、
 * 「横が２ドット以下（含）の場合は特殊ケースとして、色データのみを上から
 * 横方向へ全ドット分符号化します。」とあります。ですが、幅が2pixel以下の
 * 画像をこのPi仕様書に書かれているとおりにエンコード／デコードしている
 * ソフトは実際にはほとんどありません(pi_write_pixels_width2() のコメント
 * も参照してください)。そもそも、幅が2pixel以下の場合でもちょっと工夫を
 * すれば、特殊扱いせずに通常の方法でエンコード／デコードできるので、この
 * piwrite.c では以下のような方法を採っています。
 *
 * 基本的には、通常のエンコードの手順に従います。ただ、周囲から同じピクセル
 * を探す際、他の位置や現在の注目点と重なったりバッファの外を指す可能性の
 * ある位置を最初から除外します。
 *
 * ●幅が2pixelの場合
 *
 *     位置0A → 位置1(1ライン上の2ドット)と一致する（除外）
 *     位置0B → 位置2(2ライン上の2ドット)と一致する（除外）
 *     位置1  → 問題なし
 *     位置2  → 問題なし
 *     位置3  → 2ドット目が現在の注目点の1ドット目と重なる（除外）
 *     位置4  → 問題なし
 * 
 *   位置0については、必ずしも除外する必要はないのですが、わざわざ
 *   検索する意味がないため、このコードでは除外しています。
 * 
 * ●幅が1pixelの場合
 *
 *   この場合、Pi仕様書の「始めの（左上の）２ドット」とは、1ライン目と
 *   2ライン目の2ドットということになります。
 *
 *     位置0A → 位置2(2ライン上の2ドット)と一致する（除外）
 *     位置0B → 4ライン上/3ライン上(バッファの外)を指す（除外）
 *     位置1  → 2ドット目が現在の注目点の1ドット目と重なる（除外）
 *     位置2  → 問題なし
 *     位置3  → 現在の注目点と完全に重なる（除外）
 *     位置4  → 位置2(2ライン上の2ドット)と一致する（除外）
 *
 *   位置0については、位置0Bがバッファの外を指すため、除外する必要が
 *   あります（位置0Bのみを除外することはできないため）。したがって
 *   幅が1pixelの場合は、位置2のみを検索対象にします。
 *
 * 以上のような方法を採る事で、幅が2pixel以下の場合を特殊扱いしていない
 * 展開コードでも、多くの場合（幅1pixel/2pixelという境界条件下でも正しく
 * 動くコードになっていれば）、正常に画像を展開できるようになります。
 * 実際、この方法でエンコードされたファイルは、以下のソフトで正常に展開
 * できる事を確認しています。
 *
 * ・GV for Win32 ver 0.86
 * ・ViX ver 2.21
 * ・OPTPiX webDesigner ver 2.71
 * ・Multi Graphic Loader MJL/Win ver 0.20
 */

#ifdef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING
#define pi_write_initial_pos(p) (PI_POS_ALL)
#else
#define pi_write_initial_pos(p) \
            (((p)->width > 2) ? PI_POS_ALL : \
             (((p)->width == 2) ? PI_POS_1 | PI_POS_2 | PI_POS_4 : PI_POS_2))
#endif

#define pi_write_init_bitbuf(p) ((p)->bitcnt = bitsof(pi_bitbuf))
#define pi_write_init_poslen(p) ((p)->prevpos = PI_POS_ALL, \
            (p)->savedpos = pi_write_initial_pos(p), (p)->savedlen = 0)

#define pi_write_free_rowbuf(p)      pi_free_memory(p,(p)->rowbuf)
#define pi_write_free_color_table(p) pi_free_memory(p,(p)->clrtable)
#define pi_write_free_colorcode_table(p) do { pi_free_memory(p,(p)->clrcode); \
            pi_free_memory(p,(p)->clrcodelen); } while (0)
#define pi_write_free_lencode_table(p)   do { pi_free_memory(p,(p)->lencode); \
            pi_free_memory(p,(p)->lencodelen); } while (0)



/*
**		イメージの書き込み開始(準備)
*/
void pi_write_row_init(pi_structp pi_ptr)
{
	pi_write_init_color_table(pi_ptr);
	pi_write_init_colorcode_table(pi_ptr);
	pi_write_init_lencode_table(pi_ptr);
	pi_write_init_rowbuf(pi_ptr);
	pi_write_init_poslen(pi_ptr);
	pi_write_init_bitbuf(pi_ptr);
}


/*
**		イメージの書き込み終了(後始末)
*/
static void FASTCALL
 pi_write_row_end(pi_structp pi_ptr)
{
	pi_write_free_color_table(pi_ptr);
	pi_write_free_colorcode_table(pi_ptr);
	pi_write_free_lencode_table(pi_ptr);
	pi_write_free_rowbuf(pi_ptr);
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
 pi_write_init_color_table(pi_structp pi_ptr)
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
 pi_write_init_colorcode_table(pi_structp pi_ptr)
{
	enum { BITS = PI_WRITE_CLRCODE_TABLE_BITS };
	pi_bytep code, codelen;
	pi_uint i, b, c, l;

	PI_CHKERR_RETURN(pi_ptr);

	pi_ptr->clrcode    = code    = pi_alloc_memory(pi_ptr, pi_ptr->colors);
	pi_ptr->clrcodelen = codelen = pi_alloc_memory(pi_ptr, pi_ptr->colors);
	PI_CHKERR_RETURN(pi_ptr);

	for (b = 0, i = 0; i < pi_ptr->colors; i++) {
		if ((i >> b) & 1) b++;
		if (b <= 1) {
			c = i | 0x02;
			l = 2;
		} else {
			c = (1 << (b - 1)) - 1;
			l = 2 * b - 1;
			if ((pi_byte)b == pi_ptr->bitdepth) {
				c = c >> 1 & ~1;
				l--;
			}
			c = c << (b - 1) ^ i;
			if (l > BITS) c >>= (l - BITS);
		}
		code[i]    = c;
		codelen[i] = l;
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
 pi_write_init_lencode_table(pi_structp pi_ptr)
{
	enum { BITS = PI_WRITE_LENCODE_TABLE_BITS, SIZE = 1 << BITS };
	pi_bytep code, codelen;
	pi_uint i, b, c, l;

	PI_CHKERR_RETURN(pi_ptr);

	pi_ptr->lencode    = code    = pi_alloc_memory(pi_ptr, SIZE);
	pi_ptr->lencodelen = codelen = pi_alloc_memory(pi_ptr, SIZE);
	PI_CHKERR_RETURN(pi_ptr);

	for (b = 0, i = 1; i < SIZE; i++) {
		if ((i >> b) & 1) b++;
		c = ((1 << b) - 1) << (b - 1) ^ i;
		l = 2 * b - 1;
		if (l > BITS) c >>= (l - BITS);
		code[i]    = c;
		codelen[i] = l;
	}
}


/*
**		行バッファの初期化
*/
static void FASTCALL
 pi_write_init_rowbuf(pi_structp pi_ptr)
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
**		イメージ本体の書き込み
*/

#define pi_write_input_row(p,r)  do { \
            if ((p)->bitdepth==8) pi_memcpy((p)->rowptr[3],(r),(p)->width); \
            else pi_write_rowcpy4((p)->rowptr[3],(r),(p)->width); } while (0)

#ifdef PI_WRITE_SUPPORT_WRITEIMAGE

/*
**		イメージ全体を一度に書き込む
*/
void pi_write_image(pi_structp pi_ptr, pi_bytepp rowptr)
{
	pi_write_row_init(pi_ptr);
	while (pi_write_row(pi_ptr, *rowptr++)) ;
}

#endif /* PI_WRITE_SUPPORT_WRITEIMAGE */

/*
**		イメージの一行を書き込む
**
**		行の書き込みに成功した場合は 1 を返す。エラーが発生した
**		場合や、これ以上書くべき行がない場合は 0 を返す。
*/
int pi_write_row(pi_structp pi_ptr, pi_bytep row)
{
	PI_CHKERR_RETURN_VAL(pi_ptr, 0);

	if (pi_ptr->rownum >= pi_ptr->height) return 0;		/* end */

	pi_write_input_row(pi_ptr, row);
	pi_write_rotate_rowbuf(pi_ptr);

#ifdef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING
	if (pi_ptr->width <= 2) {
		pi_write_pixels_width2(pi_ptr);
	} else {
		if (pi_ptr->rownum == 1)
			pi_write_initial_colors(pi_ptr);

		pi_write_pixels(pi_ptr);
	}
#else
	if ((pi_ptr->rownum == 2 && (pi_ptr->width == 1 && pi_ptr->height != 1)) ||
	    (pi_ptr->rownum == 1 && (pi_ptr->width != 1 || pi_ptr->height == 1)))
		pi_write_initial_colors(pi_ptr);

	pi_write_pixels(pi_ptr);
#endif
	if (pi_ptr->rownum >= pi_ptr->height)
		pi_write_pending_data(pi_ptr);

#ifdef PI_SUPPORT_PROGRESS_CALLBACK
	if (pi_ptr->progfunc != NULL)
		if ((*pi_ptr->progfunc)(pi_ptr, pi_ptr->rownum, pi_ptr->height) != 0)
			PI_SETERR_RETURN_VAL(pi_ptr, PI_ERR_CALLBACK_CANCELED, 0);
#endif
	return !PI_ISERR(pi_ptr);
}


/*
**		イメージの一行をコピーする(16色画像用)
*/
static void FASTCALL
 pi_write_rowcpy4(pi_bytep dst, pi_bytep src, pi_size_t cnt)
{
#ifdef PI_WRITE_INPUT_8BPP_FMT
	for ( ; cnt > 0; cnt--) *dst++ = (*src++ & 0x0F);
#else
	pi_byte c;

	for ( ; cnt > 1; cnt -= 2) {
		c = src[0]; dst[0] = (c >> 4); dst[1] = (c & 0x0F);
		src += 1; dst += 2;
	}
	if (cnt > 0) {
		c = src[0]; dst[0] = (c >> 4);
	}
#endif
}


/*
**		初期カラー(2色)をファイルへ書き、上２行分の行バッファを初期化する
*/
static void FASTCALL
 pi_write_initial_colors(pi_structp pi_ptr)
{
	pi_bytep p = pi_ptr->currentp;
	pi_uint c, d, i;

	pi_write_color(pi_ptr, (c = p[0]), 0);
	pi_write_color(pi_ptr, (d = p[1]), c);

	for (i = pi_ptr->width; i > 0; i--) {
		*(--p) = d;
		*(--p) = c;
		/*
		 *	上２行分の行バッファが連続している必要あり。
		 *	(see pi_write_init_rowbuf)
		 */
	}
}


/*
**		行バッファのローテート
*/
static void FASTCALL
 pi_write_rotate_rowbuf(pi_structp pi_ptr)
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
	pi_ptr->rowptr[0] = row0;		/* ↓2pixelが複数行にまたがる場合、 */
	pi_ptr->rowend = row0 + pi_ptr->width - 1;	/* 一つ手前で停止させる */
	pi_ptr->rownum++;

	for (i = 0; i < 6; i++)
		pi_ptr->posdiff[i] = pi_ptr->rowptr[posy[i]] - row0 + posx[i];

		/* 最後の行は最後までエンコードする */
	if (pi_ptr->rownum >= pi_ptr->height) {
		pi_ptr->rowend++;
		pi_ptr->rowend[0] = pi_ptr->rowend[-1];
		/*
		 *	↑総画素数が奇数の場合、最後にゴミデータが 1pixel だけ
		 *	エンコードされるが、その 1pixel を１画素前から持ってくる。
		 */
	}
		/* 行バッファの切れ目の前後(6pixels)を重ね合わせる。 */
	if (pi_ptr->rowptr[1] > pi_ptr->rowptr[0]) {
		pi_ptr->currentp -= (w = pi_ptr->width * 4);
		pi_memcpy(pi_ptr->rowbuf, (pi_ptr->rowbuf + w), 4);
		pi_memcpy((pi_ptr->rowbuf + w + 4), (pi_ptr->rowbuf + 4), 2);
	}
}


/* -----------------------------------------------------------------------
**		ピクセルデータの書き込み
*/

	/* ピクセルの比較を行なうマクロ */
#ifdef PI_WORD_OPERATION_ON_PIXEL
/* プロセッサによっては、奇数アドレスに対して16bitアクセスを行なうと遅く
 * なる場合があるため、この方法が常に速いとは限らない。
 * さらに、x86以外のプロセッサでは、アドレスのアラインメントが合わない
 * メモリアクセスそのものができないものもある。 */
#define pi_ispixeq(p,cp,ps) \
            (*((pi_uint16p)(cp)) == *((pi_uint16p)((cp)+(p)->posdiff[ps])))
#else
#define pi_ispixeq(p,cp,ps) (*((cp)+0) == *((cp)+(p)->posdiff[ps]+0) && \
                             *((cp)+1) == *((cp)+(p)->posdiff[ps]+1))
#endif

	/* ビットバッファ関係マクロ */
#ifdef PI_WRITE_PUTBITS_FUNC
#define pi_putbits(p,b,n) pi_write_bits(p,b,n)
#else
#define pi_putbits(p,b,n) \
            (((p)->bitcnt < (pi_uint)(n)) ? pi_write_flush_bitbuf(p):(void)0, \
             (p)->bitbuf = ((p)->bitbuf << (n)) | ((b) & pi_bit1(n)), \
             (p)->bitcnt -= (n))
#endif
#define pi_bit1(n)        (((pi_uint)1 << (n)) - 1)


/*
**		行バッファからピクセルデータを読み、ファイルへ書き出す
*/
static void FASTCALL
 pi_write_pixels(pi_structp pi_ptr)
{
	pi_bytep curp = pi_ptr->currentp;
	pi_uint  npos = pi_ptr->savedpos;
	pi_uint32 len = pi_ptr->savedlen;
	pi_uint pos;

	while (curp < pi_ptr->rowend) {
		npos = pi_write_find_pixels(pi_ptr, (pos = npos), curp);
		if (npos != PI_POS_NONE) { len++; curp += 2; continue; }
			/* -- */
		if (len != 0) {
			npos = pi_write_pos(pi_ptr, pos);
			pi_write_len(pi_ptr, len);
			len = 0;
		} else {
			npos = pi_write_pos(pi_ptr, PI_POS_NONE);
			pi_write_color(pi_ptr, curp[0], curp[-1]);
			pi_write_color(pi_ptr, curp[1], curp[0]);
			curp += 2;
		}
	}
	pi_ptr->savedpos = npos;
	pi_ptr->savedlen = len;
	pi_ptr->currentp = curp;
}

#ifdef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING

/*
**		行バッファからピクセルデータを読み、ファイルへ書き出す(width <= 2)
**
**		幅が2pixel以下の画像を、Pi仕様書(PITECH.TXT)に書かれているとおりに
**		エンコードする場合、以下のような感じになります。「以下のような感じ」
**		というのは、これで本当に正しいのかよくわからないからです(^_^;。
**		一応、gimp-jp-plugins-20010820 に含まれる Pi プラグイン
**		(pi.c, PI plug-in for GIMP)と互換になるようにしてあります。
**
**		この「２ドット以下(含)の場合の特殊ケース」を実装していないソフトが
**		ほとんどなのは、この場合のPi仕様書(PITECH.TXT)の解説がきわめて曖昧で
**		ある事と、これをきちんと実装したソースコード(アセンブリ言語ではなく
**		高級言語の)がほとんど存在しないからだと思うなぁ。「フォーマットを
**		勝手に変更するのは、やめて下さい。」(PITECH.TXT)というのなら、
**		最初から曖昧さのないような解説を書いていただくか、もしくは参考となる
**		ソースコードを示していただくかをしていただきたかった。
*/
static void FASTCALL
 pi_write_pixels_width2(pi_structp pi_ptr)
{
	pi_bytep curp = pi_ptr->currentp;

	if (pi_ptr->rownum == 1) curp[-1] = 0;

	for ( ; curp < pi_ptr->rowend; curp++)
		pi_write_color(pi_ptr, curp[0], curp[-1]);

	pi_ptr->currentp = curp;
}

#endif /* PI_WRITE_W2_SPEC_COMPLIANT_ENCODING */

/*
**		保留状態になっているデータをファイルへ書き込む
*/
static void FASTCALL
 pi_write_pending_data(pi_structp pi_ptr)
{
	pi_uint m, n;

	if (pi_ptr->savedlen != 0) {
		pi_write_pos(pi_ptr, pi_ptr->savedpos);
		pi_write_len(pi_ptr, pi_ptr->savedlen);
	}
	/* m = (一度に出力できる最大ビット数) */
	m = (bitsof(pi_bitbuf) - 7 < 31) ?
	        bitsof(pi_bitbuf) - 7 : 31;
	for (n = 32 + 7; n > 0; n -= m) {
		if (n < m) m = n;
		pi_putbits(pi_ptr, 0, m);
	}
	pi_write_flush_bitbuf(pi_ptr);
	pi_write_flush_buffer(pi_ptr);
}


/*
**		周囲に同じピクセルがあるかどうかを調べる
*/
static pi_uint FASTCALL
 pi_write_find_pixels(pi_structp pi_ptr, register pi_uint pos, pi_bytep curp)
{
	if ((pos & PI_POS_0) == PI_POS_0) {
		if (curp[-1] != curp[-2]) pos &= ~PI_POS_0A;	/* １単位前が異色 */
		else                      pos &= ~PI_POS_0B;	/* １単位前が同色 */
	}
		/* ループにもできるが、こういうふうにアンロールした方が速い */
	if ((pos & PI_POS_0A) && !pi_ispixeq(pi_ptr, curp, 0)) pos &= ~PI_POS_0A;
	if ((pos & PI_POS_0B) && !pi_ispixeq(pi_ptr, curp, 1)) pos &= ~PI_POS_0B;
	if ((pos & PI_POS_1 ) && !pi_ispixeq(pi_ptr, curp, 2)) pos &= ~PI_POS_1;
	if ((pos & PI_POS_2 ) && !pi_ispixeq(pi_ptr, curp, 3)) pos &= ~PI_POS_2;
	if ((pos & PI_POS_3 ) && !pi_ispixeq(pi_ptr, curp, 4)) pos &= ~PI_POS_3;
	if ((pos & PI_POS_4 ) && !pi_ispixeq(pi_ptr, curp, 5)) pos &= ~PI_POS_4;

	return pos;
}


/*
**		位置コード/色継続フラグを書く
*/
static pi_uint FASTCALL
 pi_write_pos(pi_structp pi_ptr, pi_uint pos)
{
	static const pi_byte poscode[] = {
		0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1,
		6, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1,
		7, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1,
		6, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1,
	};
	static const pi_byte poscodelen[] = { 2, 2, 2, 0, 0, 0, 3, 3 };
	pi_uint next = pi_write_initial_pos(pi_ptr);
	pi_uint c, l;

	if (pos == PI_POS_NONE) {
		if (pi_ptr->prevpos == PI_POS_NONE) {
			c = l = 1;								/* 色継続フラグ(1) */
		} else {
			c = poscode[pi_ptr->prevpos];
			l = poscodelen[c];
		}
	} else {
		c = poscode[pos];
		l = poscodelen[c];
		if (pi_ptr->prevpos == PI_POS_NONE) l++;	/* 色継続フラグ(0) */
		if (c == 0) next &= ~PI_POS_0;
	}
	pi_putbits(pi_ptr, c, l);
	pi_ptr->prevpos = pos;

	return next;
}


/*
**		色を書く
*/
static void FASTCALL
 pi_write_color(pi_structp pi_ptr, pi_uint color, pi_uint prev)
{
	enum { TABLE_BITS = PI_WRITE_CLRCODE_TABLE_BITS };
	pi_bytep tbl, p;
	pi_byte x, t;
	pi_uint idx, c, l;

	tbl = pi_ptr->clrtable + pi_ptr->colors * prev;
	for (p = tbl, x = *p; x != (pi_byte)color; ++p, t = *p, *p = x, x = t) ;
	*tbl = x;
	idx = p - tbl;

	c = pi_ptr->clrcode[idx];
	l = pi_ptr->clrcodelen[idx];

	if (l > TABLE_BITS) {
		pi_putbits(pi_ptr, c, TABLE_BITS);
		l -= TABLE_BITS;
		c = idx;
	}
	pi_putbits(pi_ptr, c, l);
}


/*
**		連鎖数を書く
*/
static void FASTCALL
 pi_write_len(pi_structp pi_ptr, pi_uint32 len)
{
	enum { TABLE_BITS = PI_WRITE_LENCODE_TABLE_BITS,
	       TABLE_SIZE = 1 << TABLE_BITS };
	pi_uint c, l;

	if (len >= TABLE_SIZE) {
		pi_putbits(pi_ptr, pi_bit1(TABLE_BITS), TABLE_BITS);
		pi_write_len(pi_ptr, len >> TABLE_BITS);
		pi_putbits(pi_ptr, (pi_uint)len, TABLE_BITS);
		return;
	}

	c = pi_ptr->lencode[len];
	l = pi_ptr->lencodelen[len];

	if (l > TABLE_BITS) {
		pi_putbits(pi_ptr, c, TABLE_BITS);
		l -= TABLE_BITS;
		c = (pi_uint)len;
	}
	pi_putbits(pi_ptr, c, l);
}



/* ***********************************************************************
**		データ出力関数 (ビットバッファ経由)
*/

#ifdef PI_WRITE_PUTBITS_FUNC

/*
**		ビットバッファへ n ビット書く
*/
static void FASTCALL
 pi_write_bits(pi_structp pi_ptr, pi_uint bits, register pi_uint cnt)
{
	if (pi_ptr->bitcnt < cnt) pi_write_flush_bitbuf(pi_ptr);
	pi_ptr->bitbuf = (pi_ptr->bitbuf << cnt) | (bits & pi_bit1(cnt));
	pi_ptr->bitcnt -= cnt;
}

#endif

/*
**		ビットバッファのデータを出力する
*/
static void FASTCALL
 pi_write_flush_bitbuf(pi_structp pi_ptr)
{
	register pi_bytep  iobufptr = pi_ptr->iobufptr;
	register pi_size_t iobufcnt = pi_ptr->iobufcnt;
	register pi_bitbuf bitbuf   = pi_ptr->bitbuf;
	register pi_uint   bitcnt   = pi_ptr->bitcnt;

	while (bitcnt < bitsof(pi_bitbuf) - 7) {
		if (iobufcnt == 0) {
			pi_ptr->iobufptr = iobufptr;
			pi_ptr->iobufcnt = iobufcnt;
			pi_write_flush_buffer(pi_ptr);
			iobufptr = pi_ptr->iobufptr;
			iobufcnt = pi_ptr->iobufcnt;
		}
		bitcnt += 8;
		iobufcnt--;
		*iobufptr++ = (pi_byte)(bitbuf >> (bitsof(pi_bitbuf) - bitcnt)) & 0xFF;
	}
	pi_ptr->iobufptr = iobufptr;
	pi_ptr->iobufcnt = iobufcnt;
	pi_ptr->bitbuf   = bitbuf;
	pi_ptr->bitcnt   = bitcnt;
}



/* ***********************************************************************
**		データ出力関数 (バイト単位)
*/

/*
**		書き込みバッファのデータを出力する
*/
static void FASTCALL
 pi_write_flush_buffer(pi_structp pi_ptr)
{
#ifdef PI_SUPPORT_LONGJMP
	if ((*pi_ptr->iofunc)(pi_ptr) != 0)
		PI_SETERR_RETURN(pi_ptr, PI_ERR_FILE_WRITE);	/* longjmp */
#else
	static pi_byte nullbuff[16];

	if (pi_ptr->iofunc == NULL || (*pi_ptr->iofunc)(pi_ptr) != 0) {
		pi_ptr->iofunc = NULL;
		pi_ptr->iobufptr = nullbuff;
		pi_ptr->iobufcnt = sizeof(nullbuff);
		PI_SETERR_RETURN(pi_ptr, PI_ERR_FILE_WRITE);
	}
#endif
}


/*
**		書き込みバッファへ n バイト書く
*/
static void FASTCALL
 pi_write_bytes(pi_structp pi_ptr, pi_bytep buf, pi_size_t cnt)
{
	pi_size_t n;

	for (;;) {
		if (pi_ptr->iobufcnt > 0) {
			n = (pi_ptr->iobufcnt < cnt) ? pi_ptr->iobufcnt : cnt;
			pi_memcpy(pi_ptr->iobufptr, buf, n);
			pi_ptr->iobufptr += n;
			pi_ptr->iobufcnt -= n;
			buf += n;
			cnt -= n;
			if (cnt == 0) break;
		}
		pi_write_flush_buffer(pi_ptr);
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
**		Ｃ言語標準 I/O による出力サポート関数群
*/

#ifdef PI_WRITE_SUPPORT_STDIO

/*
**		データ書き込みの準備
*/
void pi_write_init_io(pi_structp pi_ptr, FILE *fp)
{
	pi_stdiop ioptr;

	pi_ptr->ioptr = ioptr = pi_alloc_memory(pi_ptr, sizeof(pi_stdio));
	PI_CHKERR_RETURN(pi_ptr);

	pi_ptr->iofunc = pi_write_iofunc_stdio;
	pi_ptr->iobufptr = ioptr->iobuf;
	pi_ptr->iobufcnt = sizeof(ioptr->iobuf);
	ioptr->fp = fp;
}


/*
**		データ書き込み終了(後始末)
*/
void pi_write_end_io(pi_structp pi_ptr)
{
	pi_free_memory(pi_ptr, pi_ptr->ioptr);
}


/*
**		データ書き込み関数(stdio)
*/
static pi_size_t
 pi_write_iofunc_stdio(pi_structp pi_ptr)
{
	pi_stdiop ioptr = pi_ptr->ioptr;
	pi_size_t datsize = sizeof(ioptr->iobuf) - pi_ptr->iobufcnt;

	datsize -= fwrite(ioptr->iobuf, 1, datsize, ioptr->fp);

	pi_ptr->iobufptr = ioptr->iobuf;
	pi_ptr->iobufcnt = sizeof(ioptr->iobuf);

	return datsize;
}

#endif /* PI_WRITE_SUPPORT_STDIO */



/* ***********************************************************************
**		以下のプログラムは、pi_write_pos() の中にある
**		poscode[] テーブル(のソースコード)を生成するものです。
*/

#if 0  /* ---- Cut Here ---- */

#include <stdio.h>

	/* 位置コードのビットフラグ */
#define PI_POS_0A		0x01	/* ２ドット左の２ドット(１単位前が同色) */
#define PI_POS_0B		0x02	/* ４ドット左の２ドット(１単位前が異色) */
#define PI_POS_1		0x04	/* １ライン上の２ドット */
#define PI_POS_2		0x08	/* ２ライン上の２ドット */
#define PI_POS_3		0x10	/* １ライン上の１ドット右の２ドット */
#define PI_POS_4		0x20	/* １ライン上の１ドット左の２ドット */
#define PI_POS_ALL		0x3F

int main(void)
{
	unsigned i;

	fputs("\tstatic const pi_byte poscode[] = {", stdout);

	for (i = 0; i <= PI_POS_ALL; i++) {
		if ((i % 16) == 0) fputs("\n\t\t", stdout);
			/* ----- */
		/*
		 *	位置に複数の候補がある場合、コード長の短い位置を優先的に
		 *	採用する。位置０の優先度が位置１/２より低いのは、一度
		 *	位置０を選ぶと次回は位置０を選べなくなってしまうことによる。
		 */
		if (i & PI_POS_1 ) { fputs("1, ", stdout); continue; }
		if (i & PI_POS_2 ) { fputs("2, ", stdout); continue; }
		if (i & PI_POS_0A) { fputs("0, ", stdout); continue; }
		if (i & PI_POS_0B) { fputs("0, ", stdout); continue; }
		if (i & PI_POS_3 ) { fputs("6, ", stdout); continue; }
		if (i & PI_POS_4 ) { fputs("7, ", stdout); continue; }
			/* ----- */
		fputs("0, ", stdout);	/* dummy */
	}
	fputs("\n\t};\n", stdout);

	return 0;
}

#endif /* ---- Cut Here ---- */

