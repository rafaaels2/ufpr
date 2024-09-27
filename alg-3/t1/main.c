#include <stdio.h>
#include <stdlib.h>
#include "libavl.h"

int main () {
    int chave;
    int *count = malloc(sizeof(int));
    *count = 0;
    noh *raiz = NULL;

    chave = 4;
    raiz = inclui(chave, raiz, count);
    *count = 0;

    chave = 2;
    inclui(chave, raiz, count);
    *count = 0;

    chave = 6;
    inclui(chave, raiz, count);
    *count = 0;

    chave = 1;
    inclui(chave, raiz, count);
    *count = 0;
    
    chave = 3;
    inclui(chave, raiz, count);
    *count = 0;

    chave = 5;
    raiz = inclui(chave, raiz, count);
    *count = 0;

    chave = 7;
    raiz = inclui(chave, raiz, count);
    *count = 0;
    

    raiz = exclui(busca(6, raiz), raiz);

    printf("no - h\n");
    emordem(raiz);
    
    free(count);
    return 0;
}
