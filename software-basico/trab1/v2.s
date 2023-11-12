.section .data
    inicioHeap: .quad 0
    topoHeap: .quad 0

    printManipulando: .string "Manipulando a Heap\n"
    printInicio: .string "# INICIO DA HEAP: %p\n"
    printTopo: .string "# TOPO DA HEAP:   %p\n"
    testeInt: .string "# NUMBER:         %ld\n"
    testeEnd: .string "# ENDEREÇO:       %p\n"
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

# -8(%rbp)  = long int *ocupado
# -16(%rbp)  = long int *tamanho
# -24(%rbp) = void *bloco
# -32(%rbp) = void *atualHeap
# -40(%rbp) = long int dif

alocaMem:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp
    subq $40, %rsp

    # inicioHead e topoHead em resgistradores
    movq inicioHeap, %rax
    movq topoHeap, %rbx

    # if (inicioHeap == topoHeap)
    cmpq %rbx, %rax
    # jne end_if

    # brk(inicioHeap + (16 + num_bytes));
    addq $16, %rdi
    addq %rax, %rdi
    movq $12, %rax
    syscall
    movq %rax, topoHeap

    # printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    movq $printTopo, %rdi
    movq %rax, %rsi
    call printf

    # ocupado = inicioHeap;
    movq inicioHeap, %rax
    movq %rax, -8(%rbp)

    # *ocupado = 1;
    movq $1, (%rax)

    # tamanho = inicioHeap + 8;
    movq inicioHeap, %rax
    addq $8, %rax
    movq %rax, -16(%rbp)

    # *tamanho = num_bytes;
    movq 16(%rbp), %rbx
    movq %rbx, (%rax)

    # return inicioHeap;
    jmp fimAloca

fimAloca:
    # fim da funcao
    addq $40, %rsp
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
    movq $10, %rdi
    pushq %rdi
    call alocaMem
    popq %rdi

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
    # printf ("\n");
    call finalizaAlocador

    # return 0;
    movq $60, %rax
    movq $0, %rdi
    syscall
