//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#ifndef __OPER_H__
#define __OPER_H__

double multVetVet(double *v1, double *v2, int n);

double **multMatMat (double **m1, double **m2, double **mRes, int n);

double multVetMatVet(double *v1, double **A, double *v2, int n);

void somaVetVet(double *v1, double m, double *v2, double *res, int n);

void multMatVet(double **A, double *v, double *res, int n);

void somaVetMatxVet(double **A, double *v1, double *v2, double m, double *res, int n);

double normamax (double *v1, double *v2, int n);

double normamaxAbs (double *v1, double *v2, int n);

double residuo(double **A, double *b, double *x, double *res, int n, double *tempR);

#endif