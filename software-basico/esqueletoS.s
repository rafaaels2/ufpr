.section .data
	A : .quad 0
.section .text
.global _start
_start:
	movq $0x123456789abcdef, % rcx
	movq % rcx, A 
	movq $60, %rax
	movq $13, %rdi
	syscall
