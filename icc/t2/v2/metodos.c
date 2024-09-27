//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <likwid.h>

#include "oper.h"
#include "sistema.h"
#include "utils.h"

#define UNROLL 4

//calcula norma L2 do residuo
double normaL2(double *r, int n){
   double soma = 0.0;
   for(int i = 0; i < n-n%UNROLL; i+=UNROLL){
      soma += r[i]*r[i]     + r[i+1]*r[i+1] + 
              r[i+2]*r[i+2] + r[i+3]*r[i+3];
   }
   /* residuo do calculo */
   for(int i = n-n%UNROLL; i < n; ++i) {
      soma   += r[i]*r[i];
   }

   return sqrt(soma);
}

/* Traduz um indice normal do vetor para uma posicao na transposta */
static inline double getTransp(double *A, int i, int j, int k, int n){
   int diff = j-i;

   if(diff > (k/2) || diff < -(k/2)){
      return 0.0;
   }
   else{
      int ind = (k/2)-diff; 
      return A[n*ind+j];
   }
}

/* Traduz um indice normal do vetor para uma posicao na matriz diagonal*/
static inline double getDiag(double *A, int i, int j, int k){
   int diff = j-i;

   if(diff > (k/2) || diff < -(k/2)){
      return 0.0;
   }
   else{
      int ind = (k/2)+diff; 
      return A[k*i+ind];
   }
}

void simetrizaSistema (SistLinear_t *SL) {
   double *M_trans = malloc(SL->k * SL->n * sizeof(double));
   memset(M_trans, 0, SL->n*SL->k*sizeof(double));
   
   int k = SL->k;
   int n = SL->n;

   if(!M_trans){
      fprintf(stderr, "Erro de alocação!\n");
      exit(ERRALLOC);
   }

   /* Cria matriz k*n transposta 
      indice eh ((n de linhas)*linha)+coluna
   */
   for (int i = 0; i < k-k%UNROLL; i+=UNROLL) {
      for(int j = 0; j < n; ++j){
         M_trans[n*(i)+j]   = SL->Diag.A[k*j+(i)];
         M_trans[n*(i+1)+j] = SL->Diag.A[k*j+(i+1)];
         M_trans[n*(i+2)+j] = SL->Diag.A[k*j+(i+2)];
         M_trans[n*(i+3)+j] = SL->Diag.A[k*j+(i+3)];
      }
   }
   for (int i = k-k%UNROLL; i < k; ++i) {
      for (int j = 0; j < n; ++j) {
         M_trans[n*i+j]   = SL->Diag.A[k*j+i];
      }
   }
   
   /* Cria matriz simetrica, com apenas diagonais da principal para cima
      Para cada coluna (i), calcula a linha (j)
      Cada coluna vai representar uma diagonal
      Na multiplicação, leva em conta o deslocamento (l) da linha, assim nao precisa calcular
      com os 0s (que nao estao nas matrizes)
      matriz é At*A
      indice eh (n de linhas(*linha)+coluna)

      l   -> (0, 1, ..., n-1)
      n-i -> numero de colunas eh diferente a cada linha da matriz
      j+i -> posição na matriz diagonal
   */
   for(int i = 0; i < k; ++i){
      for(int j = 0; j < n-i; ++j){
         SL->Sim.A[k*j+i] = 0.0;
         int l = 0;
         
         for(int l = 0; l < n; ++l){
            SL->Sim.A[k*j+i] += getTransp(M_trans, j, l, k, n) * getDiag(SL->Diag.A, l, j+i, k);
         }
      }
   }

   /* Salva inicio e final de cada linha da matriz simetrica*/
   for(int i = 0; i <= n-k; ++i){
      SL->Sim.jstart[i] = i;
      SL->Sim.jend[i] = k+i-1;
   }
   for(int i = n-k+1; i < n; ++i){
      SL->Sim.jstart[i] = i;
      SL->Sim.jend[i] = n-1;
   }

   // At * b
   /*
      Multiplica cada coluna da transposta por b[i]
   */
   for(int i = 0; i < n; ++i){
      for(int j = 0; j < n; ++j){
         SL->Sim.b[i] += getTransp(M_trans, i, j, k, n) * SL->Diag.b[j];
      }
   }

   free (M_trans);
}

//Realiza método do gradiente conjugado e retorna o tempo médio levado pelas iterações
//Criterio de parada eh apenas o num de iteracoes
double GradConjIt(SistLinear_t *restrict SL, double * restrict x, double *restrict M, FILE *restrict arq){
   LIKWID_MARKER_REGISTER("op1-v2");
   LIKWID_MARKER_START("op1-v2");

   int n = SL->n;

   double *r  = malloc(sizeof(double)*n);
   double *r1 = malloc(sizeof(double)*n);
   double *z  = malloc(sizeof(double)*n);
   double *z1 = malloc(sizeof(double)*n);
   double *p  = malloc(sizeof(double)*n);
   double *p1 = malloc(sizeof(double)*n);
   double *x1 = malloc(sizeof(double)*n);
   
   double alpha, beta;

   if(!r || !r1 || !z || !z1 || !p || !p1 || !x1){
      fprintf(stderr, "Erro de alocação!\n");
      exit(ERRALLOC);
   }

   double tempo = timestamp();

   //calcula R0
   somaVetMatxVet(SL, SL->Sim.b, x, -1, r, n);

   //z0 := M^(-1)*r
   multVetVetDiagonal(M, r, z, n);

   //p0 = z0
   memcpy(p, z, n * sizeof(double));

   int iter = 0;
   while(iter < SL->i){
      //alpha(k) := r(k)T*z(k) / p(k)T*A*p(k)
      alpha = (multVetVet(r, z, n))/(multVetMatVet(p, SL, p, n));
      if(isnan(alpha) || isinf(alpha)){
         break;
      }

      //x(k+1) := x(k) + a(k)*p(k)
      somaVetVet(x, alpha, p, x1, n);

      //r(k+1) := r(k) - a(k)*A*p(k)
      somaVetMatxVet(SL, r, p, -alpha, r1, n);

      //z(k+1) := M^(-1)*r1(k+1)
      multVetVetDiagonal(M, r1, z1, n);

      //beta(k) := r(k+1)T*z(k+1) / r(k)T*z(k)
      beta = multVetVet(r1, z1, n) / multVetVet(r, z, n);
      if(isnan(beta) || isinf(beta)){
         break;
      }

      //p(k+1) := z(k+1) + beta*p(k)
      somaVetVet(z1, beta, p, p1, n);

      //Escreve na saida a normamax da iteração
      fprintf(arq, "# iter %d: %.15g\n", iter+1, normamaxAbs(x1, x, n));

      //r = r1
      memcpy(r, r1, n * sizeof(double));
      //x = x1
      memcpy(x, x1, n * sizeof(double));
      //pk = pk1
      memcpy(p, p1, n * sizeof(double));
      //z = z1
      memcpy(z, z1, n * sizeof(double));

      iter++;
   }

   free(x1);
   free(r);
   free(r1);
   free(z);
   free(z1);
   free(p);
   free(p1);

   LIKWID_MARKER_STOP("op1-v2");

   //retorna tempo medio por iteração
   return (timestamp()-tempo) / iter;
}

//Realiza método do gradiente conjugado e retorna o tempo médio levado pelas iterações
//Criterio de parada eh o erro definido e o num de iteracoes
double GradConjErr(SistLinear_t *restrict SL, double *restrict x, double *restrict M, double err, FILE *restrict arq){
   
   int n = SL->n;

   double *r  = malloc(sizeof(double)*n);
   double *r1 = malloc(sizeof(double)*n);
   double *z  = malloc(sizeof(double)*n);
   double *z1 = malloc(sizeof(double)*n);
   double *p  = malloc(sizeof(double)*n);
   double *p1 = malloc(sizeof(double)*n);
   double *x1 = malloc(sizeof(double)*n);

   if(!r || !r1 || !z || !z1 || !p || !p1 || !x1){
      fprintf(stderr, "Erro de alocação!\n");
      exit(ERRALLOC);
   }

   double tempo = timestamp();

   //calcula R0
   somaVetMatxVet(SL, SL->Sim.b, x, -1, r, n);

   //z0 := M^(-1)*r
   multVetVetDiagonal(M, r, z, n);

   //p0 = z0
   memcpy(p, z, n * sizeof(double));

   int iter = 0;
   while(iter < SL->i){
      //alpha(k) := r(k)T*z(k) / p(k)T*A*p(k)
      double alpha = (multVetVet(r, z, n)) / (multVetMatVet(p, SL, p, n));
      if(isnan(alpha) || isinf(alpha)){
         break;
      }

      //x(k+1) := x(k) + a(k)*p(k)
      somaVetVet(x, alpha, p, x1, n);

      //r(k+1) := r(k) - a(k)*A*p(k)
      somaVetMatxVet(SL, r, p, -alpha, r1, n);

      // x ( max ( |xi - xi-1| / |xi| ) < ε )
      if (normamax (x1, x, SL -> n) < err){
         break;
      }

      //z(k+1) := M^(-1)*r(k+1)
      multVetVetDiagonal(M, r1, z1, n);

      //beta(k) := r(k+1)T*z(k+1) / r(k)T*z(k)
      double beta = multVetVet(r1, z1, n) / multVetVet(r, z, n);
      if(isnan(beta) || isinf(beta)){
         break;
      }

      //p(k+1) := z(k+1) + beta*p(k)
      somaVetVet(z1, beta, p, p1, n);
      
      //Escreve na saida a normamax da iteração
      fprintf(arq, "# iter %d: %.15g\n", iter+1, normamaxAbs(x1, x, n));

      //r = r1
      memcpy(r, r1, n * sizeof(double));
      //x = x1
      memcpy(x, x1, n * sizeof(double));
      //pk = pk1
      memcpy(p, p1, n * sizeof(double));
      //z = z1
      memcpy(z, z1, n * sizeof(double));

      iter++;
   }

   free(x1);
   free(r);
   free(r1);
   free(z);
   free(z1);
   free(p);
   free(p1);

   //retorna tempo medio por iteração
   return (timestamp()-tempo) / iter;
}