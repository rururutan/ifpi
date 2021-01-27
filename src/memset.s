#
#  Public domain by MIYASAKA Masaru (Feb 4, 2003)
#

	.text

# void *memset(void *dst, int ch, size_t cnt);

	.balign	4
	.global	_memset
_memset:
	pushl	%edi

	movl	8(%esp),%edi	# edi = dst
	movl	12(%esp),%eax	# eax = ch
	movl	16(%esp),%ecx	# ecx = cnt

	cld
	cmpl	$20,%ecx	# optimized for pentium
	jb	small_set

	andl	$0xFF,%eax
	movl	%eax,%edx
	shll	$8,%eax
	orl	%edx,%eax
	movl	%eax,%edx
	shll	$16,%eax
	orl	%edx,%eax

	movl	%ecx,%edx
	movl	%edi,%ecx
	negl	%ecx
	andl	$3,%ecx
	subl	%ecx,%edx
	rep
	stosb

	movl	%edx,%ecx
	shrl	$2,%ecx
	andl	$3,%edx
	rep
	stosl

	movl	%edx,%ecx
small_set:
	rep
	stosb

	movl	8(%esp),%eax	# eax = dst(return)

	popl	%edi
	ret

