/*
**  piwrite.c - .pi �����o�����C�u���� ver.3.10 (Sep 6, 2004)
**
**  Copyright(C) 1999-2004 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp>
**
**  For conditions of distribution and use, see copyright notice in pilib.h
**
**  �g�p�E�z�z�����ɂ��ẮApilib.h �̒��̒��쌠�\�������Ă��������B
*/

#define PI_INTERNAL
#include "pilib.h"

	/* �J���[�R�[�h�e�[�u���̃r�b�g�� (7�`CHAR_BIT) */
#define PI_WRITE_CLRCODE_TABLE_BITS		CHAR_BIT

	/* �A�����R�[�h�e�[�u���̃r�b�g�� (1�`CHAR_BIT) */
#define PI_WRITE_LENCODE_TABLE_BITS		CHAR_BIT


	/* �v���g�^�C�v�錾 */
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
**		pi_struct �\���̂̏������E�I������
*/

/*
**		�\���̂̏�����
*/
void pi_write_init(pi_structp pi_ptr)
{
#ifdef PI_INIT_STRUCT_BY_MEMSET
	pi_memset(pi_ptr, 0, sizeof(pi_struct));
#else
	static const pi_struct new_pi;	/* �J�n���� 0 (NULL) �ɏ���������� */
	*pi_ptr = new_pi;
#endif
#if (PI_OK != 0)
	PI_RESETERR(pi_ptr);	/* �G���[�̃��Z�b�g */
#endif
#ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_ptr->transcolor = PI_TRANSCOLOR_NONE;
#endif
}


/*
**		�\���̂Ɋ֘A�Â����Ă��郁�������������(�I������)
*/
void pi_write_end(pi_structp pi_ptr)
{
	pi_write_row_end(pi_ptr);
}



/* ***********************************************************************
**		�t�@�C���w�b�_�̏�������
*/

/*
**		�w�b�_����������
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

	pi_ptr->colors = 1 << pi_ptr->bitdepth;		/* �F��(16 or 256) */

	pi_memcpy(hed, templete, sizeof(templete));

	if (pi_ptr->machine[0] != '\0')
		pi_memcpy(hed+8, pi_ptr->machine, 4);	/* �@�펯�ʎq(�G���R�[�_��) */
	if (pi_ptr->aspect_x != 0 && pi_ptr->aspect_y != 0) {
		hed[5] = pi_ptr->aspect_x;
		hed[6] = pi_ptr->aspect_y;					/* �A�X�y�N�g�� */
	}
	hed[4] = (pi_ptr->mode & PI_MODE_NO_PALETTE);	/* �摜���[�h */
	hed[7] = pi_ptr->bitdepth;						/* �F�[�x(4 or 8) */
	pi_set_uint16(hed+14, pi_ptr->width);			/* �摜�̕� */
	pi_set_uint16(hed+16, pi_ptr->height);			/* �摜�̍��� */

#ifdef PI_WRITE_SUPPORT_COMMENT
# ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_write_bytes(pi_ptr, hed+0, 2);
	pi_write_comment(pi_ptr);			/* �����R�����g */
	pi_write_bytes(pi_ptr, hed+2, 10);
	pi_write_extinfo(pi_ptr);			/* �g����� */
	pi_write_bytes(pi_ptr, hed+14, 4);
# else
	pi_write_bytes(pi_ptr, hed+0, 2);
	pi_write_comment(pi_ptr);			/* �����R�����g */
	pi_write_bytes(pi_ptr, hed+2, 16);
# endif
#else	/* !PI_WRITE_SUPPORT_COMMENT */
# ifdef PI_WRITE_SUPPORT_EXTINFO
	pi_write_bytes(pi_ptr, hed+0, 12);
	pi_write_extinfo(pi_ptr);			/* �g����� */
	pi_write_bytes(pi_ptr, hed+14, 4);
# else
	pi_write_bytes(pi_ptr, hed+0, 18);
# endif
#endif /* PI_WRITE_SUPPORT_COMMENT */
}


/*
**	�������� big-endien �`�� 2�o�C�g����������������
*/
static void FASTCALL
 pi_set_uint16(pi_bytep ptr, pi_uint val)
{
	ptr[0] = (pi_byte)(val >> 8 & 0xFF);
	ptr[1] = (pi_byte)(val      & 0xFF);
}


/* -----------------------------------------------------------------------
**		.PI �t�@�C���g�����̏�������
*/

#ifdef PI_WRITE_SUPPORT_EXTINFO

#define pi_make_uint32(h,l) (((pi_uint32)(h)<<16) | ((pi_uint32)(l)&0xFFFF))

/*
**		�g��������������
**
**		�g�����̃t�H�[�}�b�g�ɂ��Ă� piread.c ��
**		pi_read_extinfo() �̃R�����g���Q�Ƃ̂��ƁB
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
		base[0] = 0x01;		/* �摜�̕\���J�n�_(X,Y) */
		pi_set_uint32(base+1, offset);
		base += TAG_SIZE;
	}
	if (pi_ptr->transcolor >= 0 &&
	    pi_ptr->transcolor < (pi_int)pi_ptr->colors) {
		base[0] = 0x02;		/* �����F�ɂȂ�p���b�g�ԍ� (0�`15/255) */
		pi_set_uint32(base+1, pi_ptr->transcolor);
		base += TAG_SIZE;
	}
	if (pi_ptr->sigbits > 0 && pi_ptr->sigbits <= 8) {
		base[0] = 0x03;		/* �p���b�g�̗L���r�b�g�� (1�`8) */
		pi_set_uint32(base+1, pi_ptr->sigbits);
		base += TAG_SIZE;
	}
	if (pi_ptr->colorused > 0 &&
	    pi_ptr->colorused <= (pi_int)pi_ptr->colors) {
		base[0] = 0x04;		/* �p���b�g�̎g�p�� (1�`16/256) */
		pi_set_uint32(base+1, pi_ptr->colorused);
		base += TAG_SIZE;
	}
	hedsize = base - hed;
	pi_set_uint16(hed, hedsize - 2);	/* �g�����̃T�C�Y */
	pi_write_bytes(pi_ptr, hed, hedsize);
}


/*
**	�������� big-endien �`�� 4�o�C�g����������������
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
**		�����R�����g�̏�������
*/

#ifdef PI_WRITE_SUPPORT_COMMENT

/*
**		�����R�����g����������
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
**		�p���b�g�̏�������
*/

	/* �J���[�\���̃A�N�Z�X�p�}�N��(�f�t�H���g��`) */
#ifndef pi_get_color
#define pi_get_color(p,r,g,b) \
            (*(r) = (p)->red, *(g) = (p)->green, *(b) = (p)->blue)
#endif

/*
**		�p���b�g����������
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
**		�C���[�W�̏������ݏ�������n��
*/

	/* �ʒu�R�[�h�̃r�b�g�t���O */
#define PI_POS_NONE		0x00
#define PI_POS_0A		0x01	/* �Q�h�b�g���̂Q�h�b�g(�P�P�ʑO�����F) */
#define PI_POS_0B		0x02	/* �S�h�b�g���̂Q�h�b�g(�P�P�ʑO���ِF) */
#define PI_POS_0		0x03
#define PI_POS_1		0x04	/* �P���C����̂Q�h�b�g */
#define PI_POS_2		0x08	/* �Q���C����̂Q�h�b�g */
#define PI_POS_3		0x10	/* �P���C����̂P�h�b�g�E�̂Q�h�b�g */
#define PI_POS_4		0x20	/* �P���C����̂P�h�b�g���̂Q�h�b�g */
#define PI_POS_ALL		0x3F

/*
 * ����2pixel�ȉ��̉摜�̏����@�F
 * 
 * Pi�d�l��(PITECH.TXT)�ɂ��ƁA����2pixel�ȉ��̉摜��Pi�Ɉ��k����ꍇ�A
 * �u�����Q�h�b�g�ȉ��i�܁j�̏ꍇ�͓���P�[�X�Ƃ��āA�F�f�[�^�݂̂��ォ��
 * �������֑S�h�b�g�����������܂��B�v�Ƃ���܂��B�ł����A����2pixel�ȉ���
 * �摜������Pi�d�l���ɏ�����Ă���Ƃ���ɃG���R�[�h�^�f�R�[�h���Ă���
 * �\�t�g�͎��ۂɂ͂قƂ�ǂ���܂���(pi_write_pixels_width2() �̃R�����g
 * ���Q�Ƃ��Ă�������)�B���������A����2pixel�ȉ��̏ꍇ�ł�������ƍH�v��
 * ����΁A���ꈵ�������ɒʏ�̕��@�ŃG���R�[�h�^�f�R�[�h�ł���̂ŁA����
 * piwrite.c �ł͈ȉ��̂悤�ȕ��@���̂��Ă��܂��B
 *
 * ��{�I�ɂ́A�ʏ�̃G���R�[�h�̎菇�ɏ]���܂��B�����A���͂��瓯���s�N�Z��
 * ��T���ہA���̈ʒu�⌻�݂̒��ړ_�Əd�Ȃ�����o�b�t�@�̊O���w���\����
 * ����ʒu���ŏ����珜�O���܂��B
 *
 * ������2pixel�̏ꍇ
 *
 *     �ʒu0A �� �ʒu1(1���C�����2�h�b�g)�ƈ�v����i���O�j
 *     �ʒu0B �� �ʒu2(2���C�����2�h�b�g)�ƈ�v����i���O�j
 *     �ʒu1  �� ���Ȃ�
 *     �ʒu2  �� ���Ȃ�
 *     �ʒu3  �� 2�h�b�g�ڂ����݂̒��ړ_��1�h�b�g�ڂƏd�Ȃ�i���O�j
 *     �ʒu4  �� ���Ȃ�
 * 
 *   �ʒu0�ɂ��ẮA�K���������O����K�v�͂Ȃ��̂ł����A�킴�킴
 *   ��������Ӗ����Ȃ����߁A���̃R�[�h�ł͏��O���Ă��܂��B
 * 
 * ������1pixel�̏ꍇ
 *
 *   ���̏ꍇ�APi�d�l���́u�n�߂́i����́j�Q�h�b�g�v�Ƃ́A1���C���ڂ�
 *   2���C���ڂ�2�h�b�g�Ƃ������ƂɂȂ�܂��B
 *
 *     �ʒu0A �� �ʒu2(2���C�����2�h�b�g)�ƈ�v����i���O�j
 *     �ʒu0B �� 4���C����/3���C����(�o�b�t�@�̊O)���w���i���O�j
 *     �ʒu1  �� 2�h�b�g�ڂ����݂̒��ړ_��1�h�b�g�ڂƏd�Ȃ�i���O�j
 *     �ʒu2  �� ���Ȃ�
 *     �ʒu3  �� ���݂̒��ړ_�Ɗ��S�ɏd�Ȃ�i���O�j
 *     �ʒu4  �� �ʒu2(2���C�����2�h�b�g)�ƈ�v����i���O�j
 *
 *   �ʒu0�ɂ��ẮA�ʒu0B���o�b�t�@�̊O���w�����߁A���O����K�v��
 *   ����܂��i�ʒu0B�݂̂����O���邱�Ƃ͂ł��Ȃ����߁j�B����������
 *   ����1pixel�̏ꍇ�́A�ʒu2�݂̂������Ώۂɂ��܂��B
 *
 * �ȏ�̂悤�ȕ��@���̂鎖�ŁA����2pixel�ȉ��̏ꍇ����ꈵ�����Ă��Ȃ�
 * �W�J�R�[�h�ł��A�����̏ꍇ�i��1pixel/2pixel�Ƃ������E�������ł�������
 * �����R�[�h�ɂȂ��Ă���΁j�A����ɉ摜��W�J�ł���悤�ɂȂ�܂��B
 * ���ہA���̕��@�ŃG���R�[�h���ꂽ�t�@�C���́A�ȉ��̃\�t�g�Ő���ɓW�J
 * �ł��鎖���m�F���Ă��܂��B
 *
 * �EGV for Win32 ver 0.86
 * �EViX ver 2.21
 * �EOPTPiX webDesigner ver 2.71
 * �EMulti Graphic Loader MJL/Win ver 0.20
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
**		�C���[�W�̏������݊J�n(����)
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
**		�C���[�W�̏������ݏI��(��n��)
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
**		�e��R�[�h�e�[�u������Ɨp�o�b�t�@�̏�����
*/

/*
**		�J���[�e�[�u���̏�����
**
**	==============================================
**	| ���ׂ̐F | �J���[�e�[�u�� (16�F�p)
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
**		�J���[�R�[�h�e�[�u���̏�����
**
**	======================================
**	|   �F�R�[�h |������| ���� (16�F�p)
**	+------------+------+-----------------
**	|    0�`1    |   2  | 1x
**	|    2�`3    |   3  | 00x
**	|    4�`7    |   5  | 010xx
**	|    8�`15   |   6  | 011xxx
**	======================================
**	|   �F�R�[�h |������| ���� (256�F�p)
**	+------------+------+-----------------
**	|    0�`  1  |   2  | 1x
**	|    2�`  3  |   3  | 00x
**	|    4�`  7  |   5  | 010xx
**	|    8�` 15  |   7  | 0110xxx
**	|   16�` 31  |   9  | 01110xxxx
**	|   32�` 63  |  11  | 011110xxxxx
**	|   64�`127  |  13  | 0111110xxxxxx
**	|  128�`255  |  14  | 0111111xxxxxxx
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
**		�A�����R�[�h�e�[�u���̏�����
**
**	===================================================
**	|   �A����   |������| ����
**	+------------+------+---------+--------------------
**	|    1       |   1  | 0
**	|    2�`   3 |   3  | 10x
**	|    4�`   7 |   5  | 110xx
**	|    8�`  15 |   7  | 1110xxx
**	|   16�`  31 |   9  | 11110xxx x
**	|   32�`  63 |  11  | 111110xx xxx
**	|   64�` 127 |  13  | 1111110x xxxxx
**	|  128�` 255 |  15  | 11111110 xxxxxxx
**	+------------+------+---------+--------------------
**	|  256�` 511 |  17  | 11111111 0 xxxxxxxx
**	|  512�`1023 |  19  | 11111111 10x xxxxxxxx
**	| 1024�`2047 |  21  | 11111111 110xx xxxxxxxx
**	| 2048�`4095 |  23  | 11111111 1110xxx xxxxxxxx
**	| 4096�`8191 |  25  | 11111111 11110xxxx xxxxxxxx
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
**		�s�o�b�t�@�̏�����
*/
static void FASTCALL
 pi_write_init_rowbuf(pi_structp pi_ptr)
{
	pi_bytep row;
	pi_uint i;

	PI_CHKERR_RETURN(pi_ptr);

	/*
	 *	�S�s���̍s�o�b�t�@���g���̂́A.pi �̏����P��(2pixels)��
	 *	�s�o�b�t�@�̐؂�ڂɂ܂�����Ȃ��悤�ɂ��邽�߁B��������ƁA
	 *	�摜�̕�����ł���悤�ȉ摜�����܂��������Ƃ��ł���B
	 *	�c��� 6pixels �́A�s�o�b�t�@�̐؂�ڂ̉e�������Ȃ�����
	 *	���߂̂��́B
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
**		�C���[�W�{�̂̏�������
*/

#define pi_write_input_row(p,r)  do { \
            if ((p)->bitdepth==8) pi_memcpy((p)->rowptr[3],(r),(p)->width); \
            else pi_write_rowcpy4((p)->rowptr[3],(r),(p)->width); } while (0)

#ifdef PI_WRITE_SUPPORT_WRITEIMAGE

/*
**		�C���[�W�S�̂���x�ɏ�������
*/
void pi_write_image(pi_structp pi_ptr, pi_bytepp rowptr)
{
	pi_write_row_init(pi_ptr);
	while (pi_write_row(pi_ptr, *rowptr++)) ;
}

#endif /* PI_WRITE_SUPPORT_WRITEIMAGE */

/*
**		�C���[�W�̈�s����������
**
**		�s�̏������݂ɐ��������ꍇ�� 1 ��Ԃ��B�G���[����������
**		�ꍇ��A����ȏ㏑���ׂ��s���Ȃ��ꍇ�� 0 ��Ԃ��B
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
**		�C���[�W�̈�s���R�s�[����(16�F�摜�p)
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
**		�����J���[(2�F)���t�@�C���֏����A��Q�s���̍s�o�b�t�@������������
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
		 *	��Q�s���̍s�o�b�t�@���A�����Ă���K�v����B
		 *	(see pi_write_init_rowbuf)
		 */
	}
}


/*
**		�s�o�b�t�@�̃��[�e�[�g
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
	pi_ptr->rowptr[0] = row0;		/* ��2pixel�������s�ɂ܂�����ꍇ�A */
	pi_ptr->rowend = row0 + pi_ptr->width - 1;	/* ���O�Œ�~������ */
	pi_ptr->rownum++;

	for (i = 0; i < 6; i++)
		pi_ptr->posdiff[i] = pi_ptr->rowptr[posy[i]] - row0 + posx[i];

		/* �Ō�̍s�͍Ō�܂ŃG���R�[�h���� */
	if (pi_ptr->rownum >= pi_ptr->height) {
		pi_ptr->rowend++;
		pi_ptr->rowend[0] = pi_ptr->rowend[-1];
		/*
		 *	������f������̏ꍇ�A�Ō�ɃS�~�f�[�^�� 1pixel ����
		 *	�G���R�[�h����邪�A���� 1pixel ���P��f�O���玝���Ă���B
		 */
	}
		/* �s�o�b�t�@�̐؂�ڂ̑O��(6pixels)���d�ˍ��킹��B */
	if (pi_ptr->rowptr[1] > pi_ptr->rowptr[0]) {
		pi_ptr->currentp -= (w = pi_ptr->width * 4);
		pi_memcpy(pi_ptr->rowbuf, (pi_ptr->rowbuf + w), 4);
		pi_memcpy((pi_ptr->rowbuf + w + 4), (pi_ptr->rowbuf + 4), 2);
	}
}


/* -----------------------------------------------------------------------
**		�s�N�Z���f�[�^�̏�������
*/

	/* �s�N�Z���̔�r���s�Ȃ��}�N�� */
#ifdef PI_WORD_OPERATION_ON_PIXEL
/* �v���Z�b�T�ɂ���ẮA��A�h���X�ɑ΂���16bit�A�N�Z�X���s�Ȃ��ƒx��
 * �Ȃ�ꍇ�����邽�߁A���̕��@����ɑ����Ƃ͌���Ȃ��B
 * ����ɁAx86�ȊO�̃v���Z�b�T�ł́A�A�h���X�̃A���C�������g������Ȃ�
 * �������A�N�Z�X���̂��̂��ł��Ȃ����̂�����B */
#define pi_ispixeq(p,cp,ps) \
            (*((pi_uint16p)(cp)) == *((pi_uint16p)((cp)+(p)->posdiff[ps])))
#else
#define pi_ispixeq(p,cp,ps) (*((cp)+0) == *((cp)+(p)->posdiff[ps]+0) && \
                             *((cp)+1) == *((cp)+(p)->posdiff[ps]+1))
#endif

	/* �r�b�g�o�b�t�@�֌W�}�N�� */
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
**		�s�o�b�t�@����s�N�Z���f�[�^��ǂ݁A�t�@�C���֏����o��
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
**		�s�o�b�t�@����s�N�Z���f�[�^��ǂ݁A�t�@�C���֏����o��(width <= 2)
**
**		����2pixel�ȉ��̉摜���APi�d�l��(PITECH.TXT)�ɏ�����Ă���Ƃ����
**		�G���R�[�h����ꍇ�A�ȉ��̂悤�Ȋ����ɂȂ�܂��B�u�ȉ��̂悤�Ȋ����v
**		�Ƃ����̂́A����Ŗ{���ɐ������̂��悭�킩��Ȃ�����ł�(^_^;�B
**		�ꉞ�Agimp-jp-plugins-20010820 �Ɋ܂܂�� Pi �v���O�C��
**		(pi.c, PI plug-in for GIMP)�ƌ݊��ɂȂ�悤�ɂ��Ă���܂��B
**
**		���́u�Q�h�b�g�ȉ�(��)�̏ꍇ�̓���P�[�X�v���������Ă��Ȃ��\�t�g��
**		�قƂ�ǂȂ̂́A���̏ꍇ��Pi�d�l��(PITECH.TXT)�̉��������߂ĞB����
**		���鎖�ƁA�����������Ǝ��������\�[�X�R�[�h(�A�Z���u������ł͂Ȃ�
**		���������)���قƂ�Ǒ��݂��Ȃ����炾�Ǝv���Ȃ��B�u�t�H�[�}�b�g��
**		����ɕύX����̂́A��߂ĉ������B�v(PITECH.TXT)�Ƃ����̂Ȃ�A
**		�ŏ�����B�����̂Ȃ��悤�ȉ���������Ă����������A�������͎Q�l�ƂȂ�
**		�\�[�X�R�[�h�������Ă��������������Ă����������������B
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
**		�ۗ���ԂɂȂ��Ă���f�[�^���t�@�C���֏�������
*/
static void FASTCALL
 pi_write_pending_data(pi_structp pi_ptr)
{
	pi_uint m, n;

	if (pi_ptr->savedlen != 0) {
		pi_write_pos(pi_ptr, pi_ptr->savedpos);
		pi_write_len(pi_ptr, pi_ptr->savedlen);
	}
	/* m = (��x�ɏo�͂ł���ő�r�b�g��) */
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
**		���͂ɓ����s�N�Z�������邩�ǂ����𒲂ׂ�
*/
static pi_uint FASTCALL
 pi_write_find_pixels(pi_structp pi_ptr, register pi_uint pos, pi_bytep curp)
{
	if ((pos & PI_POS_0) == PI_POS_0) {
		if (curp[-1] != curp[-2]) pos &= ~PI_POS_0A;	/* �P�P�ʑO���ِF */
		else                      pos &= ~PI_POS_0B;	/* �P�P�ʑO�����F */
	}
		/* ���[�v�ɂ��ł��邪�A���������ӂ��ɃA�����[�������������� */
	if ((pos & PI_POS_0A) && !pi_ispixeq(pi_ptr, curp, 0)) pos &= ~PI_POS_0A;
	if ((pos & PI_POS_0B) && !pi_ispixeq(pi_ptr, curp, 1)) pos &= ~PI_POS_0B;
	if ((pos & PI_POS_1 ) && !pi_ispixeq(pi_ptr, curp, 2)) pos &= ~PI_POS_1;
	if ((pos & PI_POS_2 ) && !pi_ispixeq(pi_ptr, curp, 3)) pos &= ~PI_POS_2;
	if ((pos & PI_POS_3 ) && !pi_ispixeq(pi_ptr, curp, 4)) pos &= ~PI_POS_3;
	if ((pos & PI_POS_4 ) && !pi_ispixeq(pi_ptr, curp, 5)) pos &= ~PI_POS_4;

	return pos;
}


/*
**		�ʒu�R�[�h/�F�p���t���O������
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
			c = l = 1;								/* �F�p���t���O(1) */
		} else {
			c = poscode[pi_ptr->prevpos];
			l = poscodelen[c];
		}
	} else {
		c = poscode[pos];
		l = poscodelen[c];
		if (pi_ptr->prevpos == PI_POS_NONE) l++;	/* �F�p���t���O(0) */
		if (c == 0) next &= ~PI_POS_0;
	}
	pi_putbits(pi_ptr, c, l);
	pi_ptr->prevpos = pos;

	return next;
}


/*
**		�F������
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
**		�A����������
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
**		�f�[�^�o�͊֐� (�r�b�g�o�b�t�@�o�R)
*/

#ifdef PI_WRITE_PUTBITS_FUNC

/*
**		�r�b�g�o�b�t�@�� n �r�b�g����
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
**		�r�b�g�o�b�t�@�̃f�[�^���o�͂���
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
**		�f�[�^�o�͊֐� (�o�C�g�P��)
*/

/*
**		�������݃o�b�t�@�̃f�[�^���o�͂���
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
**		�������݃o�b�t�@�� n �o�C�g����
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
**		�������̊m�ہ����
*/

/*
**		�������̊m��(�`�F�b�N�t��)
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
**		�������̉��
*/
static void FASTCALL
 pi_free_memory(pi_structp pi_ptr, pi_voidp ptr)
{
	if (ptr != NULL) pi_free(ptr);
}



/* ***********************************************************************
**		�b����W�� I/O �ɂ��o�̓T�|�[�g�֐��Q
*/

#ifdef PI_WRITE_SUPPORT_STDIO

/*
**		�f�[�^�������݂̏���
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
**		�f�[�^�������ݏI��(��n��)
*/
void pi_write_end_io(pi_structp pi_ptr)
{
	pi_free_memory(pi_ptr, pi_ptr->ioptr);
}


/*
**		�f�[�^�������݊֐�(stdio)
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
**		�ȉ��̃v���O�����́Api_write_pos() �̒��ɂ���
**		poscode[] �e�[�u��(�̃\�[�X�R�[�h)�𐶐�������̂ł��B
*/

#if 0  /* ---- Cut Here ---- */

#include <stdio.h>

	/* �ʒu�R�[�h�̃r�b�g�t���O */
#define PI_POS_0A		0x01	/* �Q�h�b�g���̂Q�h�b�g(�P�P�ʑO�����F) */
#define PI_POS_0B		0x02	/* �S�h�b�g���̂Q�h�b�g(�P�P�ʑO���ِF) */
#define PI_POS_1		0x04	/* �P���C����̂Q�h�b�g */
#define PI_POS_2		0x08	/* �Q���C����̂Q�h�b�g */
#define PI_POS_3		0x10	/* �P���C����̂P�h�b�g�E�̂Q�h�b�g */
#define PI_POS_4		0x20	/* �P���C����̂P�h�b�g���̂Q�h�b�g */
#define PI_POS_ALL		0x3F

int main(void)
{
	unsigned i;

	fputs("\tstatic const pi_byte poscode[] = {", stdout);

	for (i = 0; i <= PI_POS_ALL; i++) {
		if ((i % 16) == 0) fputs("\n\t\t", stdout);
			/* ----- */
		/*
		 *	�ʒu�ɕ����̌�₪����ꍇ�A�R�[�h���̒Z���ʒu��D��I��
		 *	�̗p����B�ʒu�O�̗D��x���ʒu�P/�Q���Ⴂ�̂́A��x
		 *	�ʒu�O��I�ԂƎ���͈ʒu�O��I�ׂȂ��Ȃ��Ă��܂����Ƃɂ��B
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

