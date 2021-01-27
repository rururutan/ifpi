#
#  Public domain by MIYASAKA Masaru (Feb 4, 2003)
#

	.text

	.extern	_memcpy

# void *memmove(void *dst, const void *src, size_t cnt);

	.balign	4
	.global	_memmove
_memmove:
	movl	4(%esp),%eax	# eax = dst
	movl	8(%esp),%edx	# edx = src
	movl	12(%esp),%ecx	# ecx = cnt

	cmpl	%edx,%eax
	jbe	_memcpy

	addl	%ecx,%edx	# edx = src + cnt
	cmpl	%edx,%eax
	jae	_memcpy

	pushl	%esi
	pushl	%edi

	leal	-1(%eax,%ecx),%edi	# edi = dst + cnt - 1
	leal	-1(%edx),%esi		# esi = src + cnt - 1

	std
	cmpl	$16,%ecx	# optimized for pentium
	jb	small_move

	movl	%ecx,%edx
	leal	1(%edi),%ecx
	andl	$3,%ecx
	subl	%ecx,%edx
	rep
	movsb

	movl	%edx,%ecx
	shrl	$2,%ecx
	andl	$3,%edx
	subl	$3,%edi
	subl	$3,%esi
	rep
	movsl

	addl	$3,%edi
	addl	$3,%esi
	movl	%edx,%ecx
small_move:
	rep
	movsb
	cld

	popl	%edi
	popl	%esi
	ret

