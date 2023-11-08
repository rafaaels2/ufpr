#include <stdio.h>
#include <unistd.h>

void* topoInicialHeap;

void iniciaAlocador () {
    topoInicialHeap = sbrk(0);
}

void finalizaAlocador () {
    brk (topoInicialHeap);
}

void *alocaMem (int num_bytes) {
    void *topoAtual = sbrk(0);
    brk (topoAtual + num_bytes);

    return sbrk(0);
}

void imprimeEndereço () {
    printf ("# %p\n", sbrk(0));
}

int main () {
    iniciaAlocador ();

    alocaMem (50);
    alocaMem (50);
    alocaMem (50);

    finalizaAlocador ();
    return 0;
}