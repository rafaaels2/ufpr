#include <stdio.h>
#include <unistd.h>

void *topoHeap;
void *inicioHeap;

struct noh {
    long int ocupado;
    long int tamanho;
    void *bloco;
} typedef no;

void iniciaAlocador () {
    topoHeap = sbrk(0);
    inicioHeap = topoHeap;
}

/* void finalizaAlocador () {
    brk (topoInicialHeap);
} */

void *alocaMem (long int num_bytes) {
    if (inicioHeap == topoHeap) {
        brk(inicioHeap + (16 + num_bytes));

        long int *ocupado = inicioHeap;
        *ocupado = 1;
        printf ("# ocupado: %p\n", ocupado);
        printf ("# ocupado: %ld\n", *ocupado);

        long int *tamanho = inicioHeap + 16;
        *tamanho = num_bytes;
        printf ("# tamanho: %p\n", tamanho);
        printf ("# tamanho: %ld\n", *tamanho);

        void *bloco = inicioHeap + 32;
        printf ("# bloco: %p\n", bloco);

        topoHeap = sbrk(0);

        return sbrk(0);
    }

    return sbrk(0);
}

int main () {
    printf ("\n");
    
    iniciaAlocador ();
    printf ("# topo inicial: %p\n", topoHeap);
    printf ("# incio: %p\n", inicioHeap);

    alocaMem(100);
    printf ("# topo atual: %p\n", topoHeap);
    printf ("# inicio: %p\n", inicioHeap);
    printf ("# inicio: %ld\n", *((long int*)(inicioHeap + 32)));
    
    return 0;
}