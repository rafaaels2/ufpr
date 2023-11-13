.section .data
    inicioHeap: .quad 0
    topoHeap: .quad 0

    printManipulando: .string "Manipulando a Heap\n"
    printInicio: .string "# INICIO DA HEAP: %p\n"
    printTopo: .string "# TOPO DA HEAP:   %p\n"
    testeInt: .string "# NUMBER:         %ld\n"
    testeEnd: .string "# ENDEREÇO:       %p\n"
    newLine: .string "\n"
    infoGerenciais: .string "################"
    ocupado: .string "+"
    desocupado: .string "-"
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
# -16(%rbp) = long int *tamanho
# -24(%rbp) = void *bloco
# -32(%rbp) = void *atualHeap
# -40(%rbp) = long int dif

alocaMem:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp
    subq $40, %rsp

    # inicioHeap e topoHeap em resgistradores
    movq inicioHeap, %rax
    movq topoHeap, %rbx

    # if (inicioHeap == topoHeap)
    cmpq %rbx, %rax
    jne notIf

    # brk(inicioHeap + (16 + num_bytes));
    addq $16, %rdi
    addq %rax, %rdi
    movq $12, %rax
    syscall
    movq %rax, topoHeap

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

    # atualHeap = inicioHeap;
    movq inicioHeap, %rax
    movq %rax, -32(%rbp)

    # return atualHeap;
    jmp fimAlocaMem

notIf:
    # atualHeap = inicioHeap;
    movq inicioHeap, %rax
    movq %rax, -32(%rbp)

    jmp whileAloca

whileAloca:
    # atualHeap e topoHeap em resgistradores
    movq -32(%rbp), %rax
    movq topoHeap, %rbx

    # while (atualHeap != topoHeap)
    cmpq %rax, %rbx
    je notWhile

    # if (*((long int*) (atualHeap)) != 1)
    movq -32(%rbp), %rax
    cmpq $1, (%rax)
    je alocaIf

    # if (*((long int*) (atualHeap + 8)) >= num_bytes)
    movq -32(%rbp), %rax
    addq $8, %rax
    movq 16(%rbp), %rbx
    cmpq %rbx, (%rax)
    jl alocaIf

    # ocupado = atualHeap;
    movq -32(%rbp), %rax
    movq %rax, -8(%rbp)

    # *ocupado = 1;
    movq $1, (%rax)

    # dif = *((long int*) (atualHeap + 8)) - num_bytes;
    movq -32(%rbp), %rax    # atualHeap    = rax
    addq $8, %rax           # atualHeap    = atualHeap + 8
    movq (%rax), %rbx       # (%atualHeap) = rbx 
    movq %rbx, -40(%rbp)    # rbx          = dif
    movq 16(%rbp), %rcx     # num_bytes    = rcx
    subq %rcx, -40(%rbp)    # dif          = dif - rcx

    # if (dif > 16)
    cmpq $16, -40(%rbp)
    jle fimAlocaMem

    # tamanho = atualHeap + 8;
    movq -32(%rbp), %rax
    addq $8, %rax
    movq %rax, -16(%rbp)

    # *tamanho = num_bytes;
    movq 16(%rbp), %rbx
    movq %rbx, (%rax)

    # ocupado = atualHeap + 16 + num_bytes;
    movq -32(%rbp), %rax
    addq $16, %rax
    addq 16(%rbp), %rax
    movq %rax, -8(%rbp)

    # *ocupado = 0;
    movq $0, (%rax)

    # tamanho = atualHeap + 24 + num_bytes;
    movq -32(%rbp), %rax
    addq $24, %rax
    addq 16(%rbp), %rax
    movq %rax, -8(%rbp)

    # *tamanho = dif - 16;
    subq $16, -40(%rbp)
    movq -40(%rbp), %rbx
    movq %rbx, (%rax)

    # return atualHeap;
    jmp fimAlocaMem

alocaIf:
    # atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    movq -32(%rbp), %rbx
    addq $8, %rbx
    movq (%rbx), %rbx
    movq -32(%rbp), %rax
    addq $16, %rax
    addq %rbx, %rax
    movq %rax, -32(%rbp)

    jmp whileAloca

notWhile:
    # brk(inicioHeap + (16 + num_bytes));
    addq $16, %rdi
    addq %rax, %rdi
    movq $12, %rax
    syscall
    movq %rax, topoHeap

    # ocupado = atualHeap;
    movq -32(%rbp), %rax
    movq %rax, -8(%rbp)

    # *ocupado = 1;
    movq $1, (%rax)

    # tamanho = atualHeap + 8;
    movq -32(%rbp), %rax
    addq $8, %rax
    movq %rax, -16(%rbp)

    # *tamanho = num_bytes;
    movq 16(%rbp), %rbx
    movq %rbx, (%rax)

    # return atualHeap;
    jmp fimAlocaMem

fimAlocaMem:
    # fim da funcao
    movq -32(%rbp), %rax
    addq $40, %rsp
    popq %rbp
    ret 

# -8(%rbp)   = long int *atualHeap
liberaMem:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp

    # ocupado = bloco;
    movq 16(%rbp), %rax
    movq %rax, -8(%rbp)

    # *ocupado = 0;
    movq $0, (%rax)

    # fim da funcao
    addq $8, %rsp
    popq %rbp
    ret 

# -8(%rbp)   = void *atualHeap
imprimeMapa:
    # inicio da funcao
    pushq %rbp
    movq %rsp, %rbp
    subq $8, %rsp

    # atualHeap = inicioHeap;
    movq inicioHeap, %rax
    movq %rax, -8(%rbp)

    jmp whileImprime

whileImprime:
    # atualHeap e topoHeap em resgistradores
    movq -8(%rbp), %rax
    movq topoHeap, %rbx

    # while (atualHeap != topoHeap)
    cmpq %rax, %rbx
    je fimImprimeMapa

    # printf ("################");
    movq $infoGerenciais, %rdi
    call printf

    # int i = 0;
    movq $0, %r8  

    # if (*((long int*) (atualHeap)) == 1)
    movq -8(%rbp), %rax
    cmpq $1, (%rax)
    jne forDesocupado       

    jmp forOcupado

forOcupado:
    # rax = atualHeap + 8
    movq -8(%rbp), %rax     
    addq $8, %rax           

    # i < *((long int*) (atualHeap + 8))
    cmpq %r8, (%rax)
    jle imprimeIF

    # printf ("+");
    movq $ocupado, %rdi
    call printf

    # i++
    addq $1, %r8
    jmp forOcupado

forDesocupado:
    # rax = atualHeap + 8
    movq -8(%rbp), %rax     
    addq $8, %rax 

    # i < *((long int*) (atualHeap + 8))
    cmpq %r8, (%rax)
    jle imprimeIF

    # printf ("+");
    movq $desocupado, %rdi
    call printf

    # i++
    addq $1, %r8
    jmp forDesocupado

imprimeIF:
    # atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    movq -8(%rbp), %rbx
    addq $8, %rbx
    movq (%rbx), %rbx
    movq -8(%rbp), %rax
    addq $16, %rax
    addq %rbx, %rax
    movq %rax, -8(%rbp)

    jmp whileImprime

fimImprimeMapa:
    # printf ("\n");
    movq $newLine, %rdi
    call printf

    # fim da funcao
    addq $8, %rsp
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

    # a = alocaMem (10)
    movq %rax, -8(%rbp)

    # imprimeMapa ()
    pushq %rdi
    call imprimeMapa
    popq %rdi

    # alocaMem (10);
    movq $10, %rdi
    pushq %rdi
    call alocaMem
    popq %rdi

    # b = alocaMem (10)
    movq %rax, -16(%rbp)

    # imprimeMapa ()
    pushq %rdi
    call imprimeMapa
    popq %rdi

    # libera bloco b
    movq -16(%rbp), %rdi
    pushq %rdi
    call liberaMem
    popq %rdi

    # imprimeMapa ()
    pushq %rdi
    call imprimeMapa
    popq %rdi

    # libera bloco a
    movq -8(%rbp), %rdi
    pushq %rdi
    call liberaMem
    popq %rdi

    # imprimeMapa ()
    pushq %rdi
    call imprimeMapa
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
