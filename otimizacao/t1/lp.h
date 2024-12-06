#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TAM 1024

#ifndef __LP_H__
#define __LP_H__

typedef struct lp_solve {
    float *objetivo;
    float *limites_inf;
    float **restricoes;   
} lp_solve_t;

/*
 * @brief le os valores da entrada
 *
 * @param *entrada string da entrada
 * @param *num vetor contendo os numeros da entrada
 * 
 * @return numero de elementos do vetor *num
 */
int le_entrada (char *entrada, float *num);

/*
 * @brief libera a memoria alocada durante o programa
 *
 * @param *entr string da entrada
 * @param *entr_f vetor contendo os numeros da entrada de tam MAX_TAM
 * @param *num vetor contendo os numeros da entrada de tam n
 * @param o programa linear
 */
void libera_mem (char *entr, float *entr_f, float *num, lp_solve_t *lp_solve);

/*
 * @brief copia um vetor (float)
 *
 * @param *v1 vetor 1
 * @param *v2 vetor 2
 * @param *tam tamanho do vetor
 */
void copia_vetor (float *v1, float *v2, int tam);

/*
 * @brief copia um vetor (float)
 *
 * @param *lp_solve o programa linear
 * @param *n_pro nuemro de produtos
 * @param *n_comp numero de componentes
 * 
 * @return o programa linear
 */
lp_solve_t *lp_init (lp_solve_t *lp_solve, int n_pro, int n_comp);

/*
 * @brief modela um pl e imprime uma entrada para um lp_solver
 *
 * @param *lp_solve programa linear resultante
 * @param *num vetor contendo os numeros da entrada
 * 
 * @return o programa linear
 */
lp_solve_t *modela_problema (lp_solve_t *lp_solve, float *num);

#endif