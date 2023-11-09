#include <stdio.h>
#include <unistd.h>

void *topoHeap;
void *inicioHeap;

void iniciaAlocador () {
    topoHeap = sbrk(0);
    inicioHeap = topoHeap;
}

void finalizaAlocador () {
    brk (inicioHeap);
} 

void *alocaMem (long int num_bytes) {
    long int *ocupado;
    long int *tamanho;
    void *bloco;
    void *atualHeap;

    /* se lista vazia */
    if (inicioHeap == topoHeap) {
        /* aloca o tamanho do noh */
        brk(inicioHeap + (16 + num_bytes));

        /* coloca o valor de ocupado */
        ocupado = inicioHeap;
        *ocupado = 1;

        /* coloca o valor de tamanho */
        tamanho = inicioHeap + 8;
        *tamanho = num_bytes;
        
        /* coloco o endereço do início do bloco */
        bloco = inicioHeap + 16;

        /* atualiza topo da heap */
        topoHeap = sbrk(0);

        return bloco;
    }
    
    atualHeap = inicioHeap;

    while (atualHeap != topoHeap) {
        if (*((long int*) (atualHeap)) != 1) {
            if (*((long int*) (atualHeap + 8)) >= num_bytes) {
                /* coloca o valor de ocupado */
                ocupado = atualHeap;
                *ocupado = 1;
                
                /* coloco o endereço do início do bloco */
                bloco = atualHeap + 16;

                return bloco;
            }
        }

        atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    }

    /* aloca o tamanho do noh */
    brk(atualHeap + (16 + num_bytes));

    /* coloca o valor de ocupado */
    ocupado = atualHeap;
    *ocupado = 1;
    
    /* coloca o valor de tamanho */
    tamanho = atualHeap + 8;
    *tamanho = num_bytes;

    /* coloco o endereço do início do bloco */
    bloco = atualHeap + 16;

    /* atualiza topo da heap */
    topoHeap = sbrk(0);

    return bloco;
}

int *liberaMem (void *bloco) {
    long int *ocupado;

    /* indica que o bloco está livre */
    ocupado = bloco - 16;
    *ocupado = 0;
}

int main () {
    printf ("\n");
    
    iniciaAlocador ();
    printf ("# incio: %p\n", inicioHeap);
    printf ("# topo inicial: %p\n", topoHeap);

    void *bloco = alocaMem(100);
    printf ("# topo atual: %p\n", topoHeap);

    liberaMem (bloco);

    void *bloco2 = alocaMem(101);
    printf ("# topo atual: %p\n", topoHeap);

    void *bloco3 = alocaMem(99);
    printf ("# topo atual: %p\n", topoHeap);

    void *bloco4 = alocaMem(100);
    printf ("# topo atual: %p\n", topoHeap);

    finalizaAlocador ();
    
    return 0;
}