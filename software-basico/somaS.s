.section .data
	A: .quad 0
	B: .quad 0
.section .text
.global _start
_start:
	movq $7, A
	movq $6, B
	movq A, %rax
	movq B, %rbx
	addq %rax, %rbx
	movq $60, %rax
	movq %rbx, %rdi
	syscall
