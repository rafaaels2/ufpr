#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hash.h"

#define M 11
#define NULO 666

int main () {
    tabela *T1 = malloc (sizeof (tabela) * 11);
    tabela *T2 = malloc (sizeof (tabela) * 11);

    int vet[200];
    int num, posicaoT1, posicaoT2;
    char operacao;

    int tam = 0, menor, aux;

    // inicializao das tabelas hash
    for (int i = 0; i < 11; i++) {
        T1[i].ocupado = 0;
        T2[i].ocupado = 0;
    }

    while (!feof (stdin)) {
        operacao = getchar ();           // pega o caracter da operacao
        scanf ("%d", &num);              // pega a chave
        
        if (operacao == 'i') {

            posicaoT1 = busca_inclusiva (T1, T2, num);    // devolve o indice da tebela

            if (posicaoT1 == NULO) {            
                posicaoT1 = hash_T1 (num);
                inclui (T1, num, posicaoT1);
            }
            else {
                posicaoT2 = hash_T2 (T1[posicaoT1].num);
                inclui (T2, T1[posicaoT1].num, posicaoT2);
                inclui (T1, num, posicaoT1);
            }
        }
        else if (operacao == 'r') {
            posicaoT2 = busca_reclusiva (T2, num);
            posicaoT1 = busca_reclusiva (T1, num);

            if (posicaoT2 != NULO) 
                remover (T2, posicaoT2);

            else 
                remover (T1, posicaoT1);
        }
    }

    for (int i = 0; i < 11; i++) {
        if (T2[i].ocupado) {
            vet[tam] = T2[i].num;
            tam++;
        }
    }

    for (int i = 0; i < tam - 1; i++) {
        menor = i;
        for (int j = 1; j < tam; j++) {
            if (vet[i] < vet[menor])
                menor = j;
        }
        aux = vet[i];
        vet[i] = vet[menor];
        vet[menor] = aux;
    }

    for (int i = 0; i < 11; i++) {
        if (T1[i].ocupado) {
            printf ("%d,T1,%d\n", T1[i].num, i);
        }
    }

    free (T1);
    free (T2);

    return 0;
} 
