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
/* #undef SPI_IMPLEMENT_DLLMAIN */		/* DllMain() ���`���� */
#undef SPI_IMPLEMENT_DLLHANDLE		/* ghThisInst ���`���� */
#undef SPI_IMPLEMENT_INITOPTIONS	/* DllMain() �ŃI�v�V���������������� */
#undef SPI_IMPLEMENT_CONFIGDLG		/* ConfigurationDlg() ���`���� */
#undef SPI_IMPLEMENT_ABOUTDLG		/* �Ǝ��� About �_�C�A���O���������� */
#undef SPI_IMPLEMENT_GETPREVIEW 	/* GetPreview() �p�̃R�[�h���������� */

	/* spiio.c */
#define SPI_SUPPORT_BUFFERING		/* �ǂݍ��݃f�[�^�̃o�b�t�@�����O���s�� */
#undef SPI_SUPPORT_SPIGETBYTE		/* SpiGetByte() �𗘗p�\�ɂ��� */
#undef SPI_SUPPORT_SPIREAD			/* SpiRead() �𗘗p�\�ɂ��� */
#undef SPI_SUPPORT_NULLREAD 		/* SpiRead() �ł̋�ǂݓ����L���ɂ��� */
#undef SPI_SUPPORT_SPISEEK			/* SpiSeek() �𗘗p�\�ɂ��� */
#undef SPI_SUPPORT_EOFFLAG			/* EOF ��\���t���O�𗘗p�\�ɂ��� */
#undef SPI_OPTIMIZE_SPIREAD 		/* �œK�����ꂽ SpiRead() ���g�p���� */
#undef SPI_OPTIMIZE_SPISEEK 		/* �œK�����ꂽ SpiSeek() ���g�p���� */

	/* spialloc.c */
#undef SPI_ALLOCATE_ROWPOINTERS 	/* �s�|�C���^�z������蓖�Ă� */
#undef SPI_SUPPORT_SPIREALLOC		/* SpiReAllocBuffer() �𗘗p�\�ɂ��� */

#define SPI_BUFSIZ			(32*1024)	/* �ǂݍ��݃o�b�t�@�T�C�Y */

#if defined(_MSC_VER) && (_MSC_VER < 1400) && defined(NO_CRTENV)
/* VC2005 �ȍ~�̏ꍇ�Amemmove() ��SIMD���߂��g���Ă��āASIMD���߂�
 * �T�|�[�g�`�F�b�N�Ȃǂ��s�Ȃ��K�v�����邽�߁A�X�^�[�g�A�b�v���[�`��
 * �̃����N���K�v�ƂȂ�B�R���p�C���ɕt���� memmove() �ɑ����āA
 * SIMD���߂��g��Ȃ��Ǝ��� memmove() �������N����悤�ɂ���΁A
 * VC2005 �ȍ~�̏ꍇ�ł��X�^�[�g�A�b�v���[�`�����O�����Ƃ͂ł���͂��B
 */
#define SPI_IMPLEMENT_DLLMAIN		/* DllMain() ���`���� */
#define DllMain	_DllMainCRTStartup	/* �X�^�[�g�A�b�v���[�`���������N���Ȃ� */
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

	/* �G���[�R�[�h */
#define SPI_ERROR_NOT_IMPLEMENTED	(-1)	/* ���̋@�\�͖����� */
#define SPI_ERROR_SUCCESS			0	/* ����I�� */
#define SPI_ERROR_CANCEL_EXPAND		1	/* �R�[���o�b�N�֐�����0��Ԃ��� */
#define SPI_ERROR_UNKNOWN_FORMAT	2	/* ���m�̃t�H�[�}�b�g */
#define SPI_ERROR_BROKEN_DATA		3	/* �f�[�^�����Ă��� */
#define SPI_ERROR_ALLOCATE_MEMORY	4	/* �������[���m�ۏo���Ȃ� */
#define SPI_ERROR_MEMORY			5	/* �������[�G���[�iLock�o���Ȃ����j */
#define SPI_ERROR_FILE_READ			6	/* �t�@�C�����[�h�G���[ */
#define SPI_ERROR_WINDOW			7	/* �����J���Ȃ� (����J) */
#define SPI_ERROR_INTERNAL			8	/* �����G���[ */
#define SPI_ERROR_FILE_WRITE		9	/* �������݃G���[ (����J) */
#define SPI_ERROR_END_OF_FILE		10	/* �t�@�C���I�[ (����J) */

	/* �摜���\���� */
#pragma pack(1)
typedef struct PictureInfo {
	long   left, top;			/* �摜��W�J����ʒu */
	long   width;				/* �摜�̕�(pixel) */
	long   height;				/* �摜�̍���(pixel) */
	WORD   x_density;			/* ��f�̐����������x */
	WORD   y_density;			/* ��f�̐����������x */
	short  colorDepth;			/* ��f�������bit�� */
#ifdef _WIN64
	char   dummy[2];			/* �A���C�������g */
#endif
	HLOCAL hInfo;				/* �摜���̃e�L�X�g��� */
} PictureInfo;
#pragma pack()

	/* �R�[���o�b�N�֐�(typedef) */
#ifdef __GNUC__
typedef int CALLBACK (*SPIPROC) (int, int, LONG_PTR);
#else
typedef int (CALLBACK *SPIPROC) (int, int, LONG_PTR);
#endif

	/* �O���[�o���ϐ� */
#if defined(SPI_IMPLEMENT_DLLMAIN) && defined(SPI_IMPLEMENT_DLLHANDLE)
extern HINSTANCE ghThisInst;	/* Handle to the DLL's instance. */
#endif

// �R�[���o�b�N
typedef int (__stdcall *SUSIE_PROGRESS)(int nNum, int nDenom, LONG_PTR lData);

	/* �v���g�^�C�v�錾 */
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
# define SPI_BUFSIZ			8192	/* �f�t�H���g�o�b�t�@�T�C�Y */
#endif
#ifndef SPI_EOF
# define SPI_EOF			(-256)	/* End Of File (any negative value) */
#endif

	/* �X�g���[����ԃt���O */
#define SPI_IOTYPE_NONE		0x00	/* I/O �^�C�v�F���͖������� */
#define SPI_IOTYPE_FILE		0x01	/* I/O �^�C�v�F�t�@�C������ */
#define SPI_IOTYPE_MEMORY	0x02	/* I/O �^�C�v�F���������� */
#define SPI_IOTYPE_MASK		0x0F
#define SPI_IOFLAG_ERROR	0x10	/* �G���[�����o */
#define SPI_IOFLAG_EOF		0x20	/* EOF�����o */

	/* ��ԃt���O�֘A�}�N�� */
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

	/* Susie plug-in �p���̓X�g���[���\���� */
typedef struct {
	UINT   flags;				/* �X�g���[����ԃt���O */
	HANDLE fhandle;				/* �t�@�C�����͂̃t�@�C���n���h�� */
	LPCSTR  fname;				/* �t�@�C�����͂̓��̓t�@�C���� */
	LONG_PTR   foffset;			/* �t�@�C�����͂̓ǂݍ��݊J�n�I�t�Z�b�g */
#ifdef SPI_SUPPORT_BUFFERING
	LONG   ffilptr;				/* �t�@�C�����͂̃t�@�C���|�C���^ */
#endif
	LPBYTE mbuffer;				/* ���������͂̃o�b�t�@ */
	LPBYTE mptr;				/* ���������͂̃|�C���^ */
	LONG_PTR   mcount;			/* ���������͂̎c��o�C�g�� */
	LONG_PTR   msize;			/* ���������͂̃f�[�^�T�C�Y */
} SPI_FILE;

	/* �v���g�^�C�v�錾 */
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
            /* ��LocalFree �́A���������ꍇ�� NULL ��Ԃ��B���s�����ꍇ��
             *   �����̒l�����̂܂ܕԂ��B�����̒l�� NULL �Ȃ�Ή���������
             *   NULL ��Ԃ��B
             */

	/* PictureInfo �\���̂ɒl��ݒ肷�� */
#define SpiSetPictureInfo(pi,w,h,c,xd,yd,l,t,tx) \
          do { UINT _xd=(xd), _yd=(yd);  if (_xd==0 || _yd==0) _xd=_yd=0; \
               (pi)->left      = (long)(l);  (pi)->top       = (long)(t); \
               (pi)->width     = (long)(w);  (pi)->height    = (long)(h); \
               (pi)->x_density = (WORD)_xd;  (pi)->y_density = (WORD)_yd; \
               (pi)->colorDepth = (short)(c); (pi)->hInfo = (HLOCAL)(tx); \
          } while (0)

	/* �v���g�^�C�v�錾 */
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

	/* �O���[�o���ϐ� */
extern const LPCSTR PluginInfo[];	/* �v���O�C����� */
extern const int    NumInfo;		/* �v���O�C�����̏�� */

	/* �v���g�^�C�v�錾 */
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
