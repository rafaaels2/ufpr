#include <stdio.h>
#include <stdlib.h>

#ifndef __COMB_H__
#define __COMB_H__

/* define as especificacoes das clausulas */
typedef struct {
   int sinal;       /* define o sinal dos elementos de c */
   int composto;    /* define se (n - c) esta na clausula */
   int estado;      /* define se x esta na clausula */
} espec_t;

/* inicializa as especificacoes */
espec_t defSpec (espec_t espec, int sinal ,int composto, int estado);

/* define o tamanho da combinacao e invoca combina */
int gerar_combinacoes (int *viz, int n, int tam, FILE *arq, espec_t espec, int *num);

/* funcao recursiva que gera as combinacoez */
void combinar (int *viz, int n, int *comb, int ini, int prof, int tam, FILE *arq, espec_t espec, int *num);

#endif