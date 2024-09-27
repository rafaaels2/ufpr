//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#include "oper.h"
#include "utils.h"
#include "sistema.h"
#include "metodos.h"
#include <likwid.h>

//Multiplica dois vetores (produto escalar)
double multVetVet(double *v1, double *v2, int n){
   double soma = 0.0f;

   for(int i = 0; i < n; ++i){
      soma += v1[i]*v2[i];
   }

   return soma;
}
// Multiplica duas matrizes de mesma ordem
double **multMatMat (double **m1, double **m2, double **mRes, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      mRes[i][j] = 0.0;
      for (int k = 0; k < n; k++) {
        mRes[i][j] = mRes[i][j] + (m1 [i][k] * m2[k][j]);
      }
    }
  }

  return mRes;
}

//Soma cada elemento de um vetor v1 com de o v2 multiplicado por m, devolve um vetor res
// res = v1 +m*v2
void somaVetVet(double *v1, double m, double *v2, double *res, int n){
   for(int i = 0; i < n; ++i){
      res[i] = v1[i] + m*v2[i];
   }
}

//Multiplica uma Matriz por um vetor
// res = A*v
void multMatVet(double **A, double *v, double *res, int n){
   for(int i = 0; i < n; ++i){
      res[i] = 0.0f;
      for(int j = 0; j < n; ++j){
         res[i] += A[i][j]*v[j];
      }
   }
}

//Multiplica um vetor v1 com uma matriz, e o vetor resultante é multiplicado com um vetor v2
//retorna v1*A*v2
double multVetMatVet(double *v1, double **A, double *v2, int n){
   double aux[n];

   multMatVet(A, v1, aux, n);

   //retorna a multiplicacao do vetor resultante com o segundo vetor
   return multVetVet(aux, v2, n);
}

//Multiplica o vetor v2 e a matriz A, e os valores vetor resultante, multiplicados por m, são somados com os valores de v1
//res = v1 + mAv2
void somaVetMatxVet(double **A, double *v1, double *v2, double m, double *res, int n){
   double vNovo[n];
   multMatVet(A, v2, vNovo, n);

   for(int i = 0; i < n; ++i){
      res[i] = v1[i] + m*vNovo[i];
   }
}

double residuo(double **A, double *b, double *x, double *res, int n, double *tempR){
   *tempR = timestamp();

   LIKWID_MARKER_REGISTER("op2-v1");
   LIKWID_MARKER_START("op2-v1");

   double vNovo[n];
   multMatVet(A, x, vNovo, n);

   for(int i = 0; i < n; ++i){
      res[i] = vNovo[i] - b[i];
   }

   double ret = normaL2(res, n);

   LIKWID_MARKER_STOP("op2-v1");
   *tempR = timestamp() - *tempR;

   return ret;
}

// Encontra a maior diferenca relativa entre as respostas subsequentes
// retorna max ( |xi - xi-1| / |xi| )
double normamax (double *v1, double *v2, int n){
   double diff;
   double maior = 0.0;

   for (int i = 0; i < n; i++) {
      diff = ( ABS(v1[i] - v2[i]) / ABS(v1[i]) );
      if (diff > maior)
         maior = diff;
   }

   return maior;
}  

// Encontra a maior diferenca absoluta entre as respostas subsequentes
// retorna max ( |xi - xi-1| )
double normamaxAbs (double *v1, double *v2, int n){
   double diff;
   double maior = 0.0;

   for (int i = 0; i < n; i++) {
      diff = ( ABS(v1[i] - v2[i]));
      if (diff > maior)
         maior = diff;
   }

   return maior;
}