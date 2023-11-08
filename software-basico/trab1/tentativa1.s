.section .data
.section .text
.global _start
_start:
	movq $12, %rax
	syscall

    cmpq $-1, %rax
    je erro

    movq $1, %rdi
    movq $60, %rax
    syscall
    
erro:
    movq $-1, %rdi
    movq $60, %rax
    syscall

