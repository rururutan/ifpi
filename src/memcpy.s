#
#  Public domain by MIYASAKA Masaru (Feb 4, 2003)
#

	.text

# void *memcpy(void *dst, const void *src, size_t cnt);

	.balign	4
	.global	_memcpy
_memcpy:
	pushl	%esi
	pushl	%edi

	movl	12(%esp),%edi	# edi = dst
	movl	16(%esp),%esi	# esi = src
	movl	20(%esp),%ecx	# ecx = cnt
	movl	%edi,%eax	# eax = dst(return)

	cld
	cmpl	$20,%ecx	# optimized for pentium
	jb	small_copy

	movl	%ecx,%edx
	movl	%edi,%ecx
	negl	%ecx
	andl	$3,%ecx
	subl	%ecx,%edx
	rep
	movsb

	movl	%edx,%ecx
	shrl	$2,%ecx
	andl	$3,%edx
	rep
	movsl

	movl	%edx,%ecx
small_copy:
	rep
	movsb

	popl	%edi
	popl	%esi
	ret

