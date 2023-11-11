.section .data
    inicioHeap: .quad 0
    topoHeap: .quad 0

    printManipulando: .string "Manipulando a Heap\n"
    printInicio: .string "# INICIO DA HEAP: %p\n"
    printTopo: .string "# TOPO DA HEAP:   %p\n"
    newLine: .string "\n"
.section .text
.globl main

iniciaAlocador:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp

    # inicioHeap = sbrk(0)
    movq $12, %rax
    movq $0, %rdi
    syscall
    movq %rax, inicioHeap

    # inicioHeap = topoHeap;
    movq %rax, topoHeap

    # fim da funcao
    popq %rbp
    ret

finalizaAlocador:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp

    # brk (inicioHeap);
    movq $12, %rax
    movq inicioHeap, %rdi
    syscall

    # fim da funcao
    popq %rbp
    ret

alocaMem:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp
    subq $40, %rsp

    movq inicioHeap, %rax
    movq topoHeap, %rbx
    cmpq %rbx, %rax
    jne fimIf

    # fim da funcao
    popq %rbp
    ret
#
#
#
#
# ----------------------------------------------- FUNCOES AQUI -----------------------------------------------
#
#
#
#
main:
    #
    #
    # Tabela:
    # a = -8(%rbp)
    # b = -16(%rbp)
    #
    #
    pushq %rbp
    movq %rsp, %rbp

    # void *a, *b;
    subq $16, %rsp

    # printf ("Manipulando a Heap\n");
    movq $printManipulando, %rdi
    call printf

    # iniciaAlocador();
    call iniciaAlocador

    # printf ("# INICIO DA HEAP: %p\n", inicioHeap);
    movq $printInicio, %rdi
    movq inicioHeap, %rsi
    call printf

    # printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    movq $printTopo, %rdi
    movq topoHeap, %rsi
    call printf

    # printf ("\n");
    movq $newLine, %rdi
    call printf

    # alocaMem (10);
    pushq $10
    call alocaMem
    addq $8 , % rsp

    #
    #
    #
    #
    #
    #
    # ----------------------------------------------- PRINTS E CHAMADAS DE FUNCAO NA MAIN AQUI -----------------------------------------------
    #
    #
    #
    #
    #
    #

    # finalizaAlocador ();
    call finalizaAlocador

    # return 0;
    movq $60, %rax
    movq $0, %rdi
    syscall
