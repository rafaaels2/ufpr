//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sistema.h"

void liberaSisLin (SistLinear_t *SL)
{
  if (SL) {
    if (SL->Sim.A) {
      free(SL->Sim.A);
    }
    if (SL->Diag.A) {
      free(SL->Diag.A);
    }
    
    if (SL->Sim.b) free(SL->Sim.b);
    if (SL->Diag.b) free(SL->Diag.b);

    if (SL->Sim.jstart) free(SL->Sim.jstart);
    if (SL->Sim.jend) free(SL->Sim.jend);

    if (SL->Diag.jstart) free(SL->Diag.jstart);
    if (SL->Diag.jend) free(SL->Diag.jend);

    free(SL);
  }
}

SistLinear_t* alocaSisLin (unsigned int n, unsigned int k)
{
  SistLinear_t *SL = (SistLinear_t *) malloc(sizeof(SistLinear_t));
  
  if ( SL ) {
    SL->n = n;
    SL->k = k;

    /*  matriz simetrica tem n-k+1 linhas de k elementos, depois cada linha diminui 1
        até chegar no tamanho 1, isso pode ser calculado por uma soma de pa = tamanho(inicial+final) / 2
    */
    int tam = ((n-k+1)*k) + ((k-1)*(k))/2;
    SL->Sim.A  = (double *) malloc(n * tam * sizeof(double));

    SL->Diag.A = (double *) malloc(n * k * sizeof(double));
    SL->Diag.b = (double *) malloc(n * sizeof(double));
    SL->Sim.b  = (double *) malloc(n * sizeof(double));

    if (!(SL->Diag.A) || !(SL->Diag.A) || !(SL->Diag.b) || !(SL->Sim.b)) {
      liberaSisLin(SL);
      fprintf(stderr, "Erro de alocação!\n");
      return NULL;
    }

    //aloca vetores com posicoes de inicio e final das linhas
    SL->Diag.jstart = (int *) malloc(n * sizeof(int));
    SL->Diag.jend   = (int *) malloc(n * sizeof(int));
    SL->Sim.jstart   = (int *) malloc(n * sizeof(int));
    SL->Sim.jend     = (int *) malloc(n * sizeof(int));

    //zera primeira linha
    memset(SL->Diag.A,0, k*n*sizeof(double));
    memset(SL->Sim.A, 0, k*tam*sizeof(double));
    memset(SL->Sim.b, 0, n*sizeof(double));
  }
  
  return (SL);
}

void prnSisLinDiag (SistLinear_t *SL)
{
  int n=SL->n;
  int k=SL->k;

  for(int i=0; i < n; ++i) {
    for(int j=0; j < k; ++j)
      printf ("%10g", SL->Diag.A[k*i+j]);
    printf ("   |   %g\n", SL->Diag.b[i]);
  }
  printf("\n\n");
}

void prnSisLinSim (SistLinear_t *SL){
  int n=SL->n;
  int k=SL->k;

  int impar = 2;

  for (int i = 0; i <= n-k; ++i) {
    for(int j = 0; j < k; ++j){
      printf("%10g ", SL->Sim.A[k*i+j]);
    }
    printf("  |   %g\n", SL->Sim.b[i]);
  }

   /* linhas comecam a diminuir */
   for (int i = n-k+1; i < n; ++i) {
    for(int j = 0; j < n-i; ++j){
      printf("%10g ", SL->Sim.A[k*i+j]);
    }
    for(int j = n-i; j < k; ++j){
      printf("%10c ", 'x');
    }
    printf("  |   %g\n", SL->Sim.b[i]);
  }
  printf("\n\n");
}