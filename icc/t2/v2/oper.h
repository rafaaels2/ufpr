//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#include "sistema.h"

#ifndef __OPER_H__
#define __OPER_H__

double multVetVet(double *restrict v1, double *restrict v2, int n);

void multVetVetDiagonal (double *restrict vetDiagonal, double *restrict v2, double *restrict res, int n);

double multVetMatVet(double *v1, SistLinear_t *restrict SL, double *v2, int n);

void somaVetVet(double *restrict v1, double m, double *restrict v2, double *restrict res, int n);

void somaVetMatxVet(SistLinear_t *restrict SL, double *restrict v1, double *restrict v2, double m, double *restrict res, int n);

double normamax (double *restrict v1, double *restrict v2, int n);

double normamaxAbs (double *restrict v1, double *restrict v2, int n);

void multMatSimVet(SistLinear_t *SL, double *vet, double *res, int n);

double residuoDiag(SistLinear_t *SL, double *b, double *x, double *r, double *tempR);

#endif