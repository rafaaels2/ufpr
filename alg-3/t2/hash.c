#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hash.h"

#define M 11
#define NULO 666

int hash_T1 (int num) {
    return num % M;
}

int hash_T2 (int num) {
    return floor (M * (num * 0.9 - floor((num * 0.9))));
}

int busca_inclusiva (tabela T1[], tabela T2[], int num) {
    int posicaoT1 = hash_T1 (num);

    if ( !(T1[posicaoT1].ocupado) )
        return NULO;

    return posicaoT1;
}

int busca_reclusiva (tabela T[], int num) {
    for (int i = 0; i < M; i++) {
        if (T[i].num == num) 
            return i;
    }

    return NULO;
}

void inclui (tabela T[], int num, int posicao) {
    T[posicao].num = num;
    T[posicao].ocupado = 1;
}

void remover (tabela T[], int posicao) {
    T[posicao].ocupado = 0;
}
