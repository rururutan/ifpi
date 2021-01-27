/*
**  pilib.h - .pi �ǂݍ���/�����o�����C�u���� ver.3.10 (Sep 6, 2004
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
**  �{�\�t�g�E�F�A�́u����̂܂܁v�ŁA�����ł��邩�Öقł��邩���킸�A�����
**  �ۏ؂��Ȃ��񋟂���܂��B �{�\�t�g�E�F�A�̎g�p�ɂ���Đ����邢���Ȃ鑹�Q��
**  ���Ă��A��҂͈�؂̐ӔC�𕉂�Ȃ����̂Ƃ��܂��B
**
**  �ȉ��̐����ɏ]������A���p�A�v���P�[�V�������܂߂āA�{�\�t�g�E�F�A��C�ӂ�
**  �ړI�Ɏg�p���A���R�ɉ��ς��čĔЕz���邱�Ƃ����ׂĂ̐l�ɋ����܂��B
**
**  1. �{�\�t�g�E�F�A�̏o���ɂ��ċ��U�̕\�������Ă͂Ȃ�܂���B
**     ���Ȃ����I���W�i���̃\�t�g�E�F�A���쐬�����Ǝ咣���Ă͂Ȃ�܂���B
**     ���Ȃ����{�\�t�g�E�F�A�𐻕i���Ŏg�p����ꍇ�A���i�̕����Ɏӎ�������
**     ����������΍K���ł����A�K�{�ł͂���܂���B
**
**  2. �\�[�X��ύX�����ꍇ�́A���̂��Ƃ𖾎����Ȃ���΂Ȃ�܂���B�I���W�i��
**     �̃\�t�g�E�F�A�ł���Ƃ������U�̕\�������Ă͂Ȃ�܂���B
**
**  3. �\�[�X�̔Еz������A���̕\�����폜������A�\���̓��e��ύX�����肵�Ă�
**     �Ȃ�܂���
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

	/* Susie�G���[�R�[�h (from spibase.h)*/
#ifndef SPI_ERROR_SUCCESS
#define SPI_ERROR_NOT_IMPLEMENTED	(-1)	/* ���̋@�\�͖����� */
#define SPI_ERROR_SUCCESS			0	/* ����I�� */
#define SPI_ERROR_CANCEL_EXPAND		1	/* �R�[���o�b�N�֐�����0��Ԃ��� */
#define SPI_ERROR_UNKNOWN_FORMAT	2	/* ���m�̃t�H�[�}�b�g */
#define SPI_ERROR_BROKEN_DATA		3	/* �f�[�^�����Ă��� */
#define SPI_ERROR_ALLOCATE_MEMORY	4	/* �������[���m�ۏo���Ȃ� */
#define SPI_ERROR_MEMORY			5	/* �������[�G���[�iLock�o���Ȃ����j*/
#define SPI_ERROR_FILE_READ			6	/* �t�@�C�����[�h�G���[ */
#define SPI_ERROR_WINDOW			7	/* �����J���Ȃ� (����J) */
#define SPI_ERROR_INTERNAL			8	/* �����G���[ */
#define SPI_ERROR_FILE_WRITE		9	/* �������݃G���[ (����J) */
#define SPI_ERROR_END_OF_FILE		10	/* �t�@�C���I�[ (����J) */
#endif

	/* �G���[�R�[�h�̃I�[�o�[���C�h��` */
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

	/* �p���b�g�\���̂̃I�[�o�[���C�h��`
		(= BMP �t�@�C���� RGBQUAD �\����) */
typedef RGBQUAD pi_color;
#define PI_COLOR_DEFINED
#define pi_set_color(p,r,g,b) \
            ((p)->rgbRed = (r), (p)->rgbGreen = (g), (p)->rgbBlue = (b))
#define pi_get_color(p,r,g,b) \
            (*(r) = (p)->rgbRed, *(g) = (p)->rgbGreen, *(b) = (p)->rgbBlue)

	/* �e�퓮��ݒ� */
#undef PI_SUPPORT_LONGJMP				/* longjmp() ���g���ăG���[���������� */
#undef PI_SUPPORT_PROGRESS_CALLBACK 	/* �R�[���o�b�N�֐����g�p�\�ɂ��� */

#undef PI_READ_SKIP_MACBINARY			/* �擪�� MacBinary ��ǂݔ�΂� */
#undef PI_READ_SUPPORT_STDIO			/* �b�W���X�g���[�����͂̃T�|�[�g */
#define PI_READ_SUPPORT_EXTINFO			/* �w�b�_�̊g�������擾���� */
#define PI_READ_SUPPORT_COMMENT			/* �����e�L�X�g���擾���� */
#undef PI_READ_SUPPORT_READIMAGE		/* pi_read_image() ���g�p�\�ɂ��� */
#undef PI_READ_OUTPUT_8BPP_FMT			/* ��� 8bit/pixel �`���ŏo�͂��� */

#undef PI_WRITE_SUPPORT_STDIO			/* �b�W���X�g���[���o�͂̃T�|�[�g */
#define PI_WRITE_SUPPORT_EXTINFO 		/* �w�b�_�̊g������ݒ肷�� */
#define PI_WRITE_SUPPORT_COMMENT 		/* �����e�L�X�g��ݒ肷�� */
#undef PI_WRITE_SUPPORT_WRITEIMAGE		/* pi_write_image() ���g�p�\�ɂ��� */
#undef PI_WRITE_INPUT_8BPP_FMT			/* ��� 8bit/pixel �`���œ��͂��� */

#define PI_CALL_USING_FASTCALL			/* �����֐��� fastcall �Ăяo�����g�� */
#define PI_INIT_STRUCT_BY_MEMSET 		/* pi_memset() �ō\���̂����������� */
#define PI_WORD_OPERATION_ON_PIXEL		/* 16�r�b�g�P�ʂŃs�N�Z�������� */
#define PI_WRITE_PUTBITS_FUNC			/* �֐��ł̃r�b�g�o�̓��[�`�����g�� */

	/* ����2pixel�ȉ��̉摜���APi�d�l��(PITECH.TXT)�ɏ�����Ă���Ƃ����
	 * �G���R�[�h�^�f�R�[�h����ɂ́A�ȉ��̃}�N���� #define ���܂��B
	 * �ڂ����� piwrite.c �̒��́u����2pixel�ȉ��̉摜�̏����@�v������
	 * ���������B���ɗ��R�̂Ȃ����� #undef �ɂ��Ă������Ƃ����E�߂��܂��B*/
#undef PI_READ_W2_SPEC_COMPLIANT_DECODING
#undef PI_WRITE_W2_SPEC_COMPLIANT_ENCODING



/* ***********************************************************************
**		Pi format encoding/decoding library (piwrite.c/piread.c)
*/

	/* �f�t�H���g�̃G���[�R�[�h(�Ē�`�\) */
#ifndef PI_OK
#define PI_OK						0		/* ���� */
#define PI_ERR_NOT_A_PI				1		/* .PI�t�@�C���ł͂Ȃ� */
#define PI_ERR_INVALID_BITDEPTH		2		/* �����ȐF�[�x(4,8 �ȊO) */
#define PI_ERR_INVALID_WIDTH		3		/* �����ȕ�(0 or 65535��) */
#define PI_ERR_INVALID_HEIGHT		4		/* �����ȍ���(0 or 65535��) */
#define PI_ERR_FILE_WRITE			5		/* �t�@�C���������݃G���[ */
#define PI_ERR_FILE_READ			6		/* �t�@�C���ǂݍ��݃G���[ */
#define PI_ERR_END_OF_FILE			7		/* �t�@�C���I�[�����o */
#define PI_ERR_OUT_OF_MEMORY		8		/* �������s�� */
#define PI_ERR_CALLBACK_CANCELED	9		/* �s�R�[���o�b�N�֐��ɂ�钆�~ */
#endif

	/* �e��t���O�Ȃ� */
#define PI_FLAG_NO_COMMENT		0x0001		/* �R�����g���擾���Ȃ� */
#define PI_FLAG_MISSING_SIG		0x0002		/* �t�@�C���擪�� "Pi" ���Ȃ� */
#define PI_MODE_NO_PALETTE		0x80		/* �f�t�H���g�p���b�g�g�p�摜 */
#define PI_TRANSCOLOR_NONE		(-1)		/* �����F���Ȃ��ꍇ�̎w���l */

	/* �G���[�����p�}�N�� */
#ifdef PI_SUPPORT_LONGJMP
	/* setjmp() �� longjmp() �ɂ����@ */
#include <setjmp.h>		/* setjmp() & longjmp() */
#define PI_ISERR(p)     (0)
#define PI_RESETERR(p)
#define PI_CHKERR_RETURN(p)
#define PI_CHKERR_RETURN_VAL(p,r)
#define PI_SETERR_RETURN(p,e)        longjmp((p)->jmpbuf,(e))
#define PI_SETERR_RETURN_VAL(p,e,r)  longjmp((p)->jmpbuf,(e))
#else
	/* �ϐ� error �̒l�����Ȃ��珇�� return ���Ă������@ */
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

	/* ��s������̃o�C�g�����v�Z����}�N�� */
#ifdef PI_READ_OUTPUT_8BPP_FMT
#define pi_read_get_rowbytes(p) ((p)->width)
#else
#define pi_read_get_rowbytes(p) \
            (((p)->bitdepth == 8) ? (p)->width : ((pi_uint32)(p)->width+1)/2)
#endif

	/* piread.c / piwrite.c �ł̂ݎQ�Ƃ����}�N�� */
#ifdef PI_INTERNAL

	/* FASTCALL �Ăяo���K�� */
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

	/* �ϐ�/�^�̃r�b�g�� */
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

typedef unsigned int pi_bitbuf;		/* unsigned ���� 16bit �ȏ� */
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

	/* �p���b�g�p�J���[�\����(�f�t�H���g��`) */
#ifndef PI_COLOR_DEFINED
#define PI_COLOR_DEFINED
typedef struct pi_color {
	pi_byte		red;
	pi_byte		green;
	pi_byte		blue;
} pi_color;
#endif
typedef pi_color * pi_colorp;

	/* �b����W�� I/O �ɂ����o�̓T�|�[�g�p�\���� */
#ifdef PI_READ_SUPPORT_STDIO
#include <stdio.h>
struct pi_stdio {
	FILE		*fp;			/* ���o�̓X�g���[�� */
	pi_byte		iobuf[4096];	/* ���o�̓o�b�t�@ */
};
#endif

	/* �����R�����g�ێ��p�\���� */
#ifdef PI_READ_SUPPORT_COMMENT
struct pi_text {
	pi_textp	next;		/* ���ւ̃|�C���^ */
	pi_uint32	size;		/* �����e�L�X�g�̃T�C�Y */
	pi_charp	text;		/* �����e�L�X�g */
};
#endif

	/* pi_struct �\���� */
struct pi_struct {
		/* �G���[�����p�ϐ� */
#ifdef PI_SUPPORT_LONGJMP
	jmp_buf		jmpbuf;				/* setjmp() / longjmp() �p�R���e�L�X�g */
#else
	pi_int		error;				/* �G���[�R�[�h */
#endif
		/* .PI�t�@�C���w�b�_��񁕊e��摜��� */
	pi_uint		width, height;		/* �摜�̃T�C�Y */
	pi_uint		colors;				/* �F��(16 or 256) */
	pi_byte		bitdepth;			/* �F�[�x(4 or 8) */
	pi_byte		aspect_x, aspect_y;	/* �A�X�y�N�g�� */
	pi_byte		mode;				/* �摜���[�h */
	pi_byte		machine[4];			/* �@�펯�ʎq(�G���R�[�_��) */

		/* I/O �֌W */
	pi_iofncp	iofunc;				/* I/O �֐� */
	pi_voidp	ioptr;				/* I/O �֐��p�ėp�|�C���^ */
	pi_bytep	iobufptr;			/* I/O �o�b�t�@�̃|�C���^ */
	pi_size_t	iobufcnt;			/* I/O �o�b�t�@�̎c��o�C�g�� */
	pi_bitbuf	bitbuf;				/* �r�b�g�o�b�t�@ */
	pi_uint		bitcnt;				/* �r�b�g�o�b�t�@�̎c��r�b�g�� */

		/* ��Ɨp�̈�E��Ɨp�ϐ� */
	pi_bytep	clrtable;			/* �J���[�e�[�u�� */
	pi_bytep	clrcode;			/* �J���[�R�[�h�e�[�u�� */
	pi_bytep	clrcodelen;			/* �J���[�R�[�h�̒��� */
	pi_bytep	lencode;			/* �A�����R�[�h�e�[�u�� */
	pi_bytep	lencodelen;			/* �A�����R�[�h�̒��� */

	pi_uint		prevpos;			/* ��O�̈ʒu�R�[�h */
	pi_uint		savedpos;			/* ���݂̈ʒu�R�[�h */
	pi_uint32	savedlen;			/* ���݂̘A���� */

	pi_bytep	rowbuf;				/* �s�o�b�t�@ */
	pi_bytep	rowptr[4];			/* �s�̐擪�A�h���X */
	pi_bytep	rowend;				/* �s�̏I�[�A�h���X */
	pi_bytep	currentp;			/* ���݂̍s�̒��ړ_ */
	pi_uint		rownum;				/* ���݂̍s�̍s�ԍ� (1�`height) */
	pi_ptrdif_t	posdiff[6];			/* �ʒu�R�[�h�ɑΉ����鑊�΃A�h���X */

		/* .PI�t�@�C���w�b�_�̊g����� */
#if defined(PI_READ_SUPPORT_EXTINFO) || defined(PI_WRITE_SUPPORT_EXTINFO)
	pi_uint		offset_x, offset_y;	/* �摜�̕\���J�n�_(�I�t�Z�b�g) */
	pi_int		transcolor;			/* �����F�ɂȂ�p���b�g�ԍ� (0�`15/255) */
	pi_int		sigbits;			/* �p���b�g�̗L���r�b�g�� (1�`8) */
	pi_int		colorused;			/* �p���b�g�̎g�p�� (1�`16/256) */
#endif
		/* �����e�L�X�g��� */
#if defined(PI_READ_SUPPORT_COMMENT) || defined(PI_WRITE_SUPPORT_COMMENT)
	pi_text		comment;			/* �����e�L�X�g�o�b�t�@ */
#endif
		/* �R�[���o�b�N�֐� */
#ifdef PI_SUPPORT_PROGRESS_CALLBACK
	pi_progfncp	progfunc;			/* �R�[���o�b�N�֐� */
	pi_voidp	progptr;			/* �R�[���o�b�N�֐��p�ėp�|�C���^ */
#endif
};

	/* �v���g�^�C�v�錾 (piread.c) */
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

	/* �v���g�^�C�v�錾 (piwrite.c) */
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
