//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#ifndef __METODOS_H__
#define __METODOS_H__

#include "sistema.h"
#include <stdio.h>

double normaL2(double *r, int n);

void simetrizaSistema (SistLinear_t *SL);

double GradConjIt(SistLinear_t *SL, double *x, double **M, FILE *arq);

double GradConjErr(SistLinear_t *SL, double *x, double **M, double err, FILE *arq);

#endif