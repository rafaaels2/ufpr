//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#include "oper.h"
#include "sistema.h"
#include "utils.h"
#include "metodos.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <likwid.h>

#define UNROLL 4

//Multiplica matriz simetrica com vetor
void multMatSimVet(SistLinear_t *restrict SL, double *restrict vet, double *restrict res, int n){
   int k = SL->k;

   double *tab = malloc(sizeof(double)*n);
   memset(tab, 0, n*sizeof(double));

   /*
      Multplica valores da matriz pelo vetor, considerando que ela possui cada elemento uma única vez,
      e não mais duas. A primeira coluna é calculada fora para podermos fazer uma tabela com elementos
      simetricos, que existem apenas após a primeira linha

      maxj -> maior índice j possível de cada linha, considerando as posições originais na matriz
      js = jstart[i] -> índice da coluna original de onde estava o primeiro elemento não nulo da linha
      jend[i] -> índice da coluna original de onde estava o último elemento não nulo da linha
      j -> (1, 2, ..., maxj-1)
      js+j -> posição inicial do vetor + deslocamento do j
      i+j -> elementos simetricos da linha i
   */
   for (int i = 0; i < n; ++i) {
      int maxj = SL->Sim.jend[i]-SL->Sim.jstart[i]+1;
      int js = SL->Sim.jstart[i];

      //primeira coluna fora
      res[i] += SL->Sim.A[k*i] * vet[js];

      /* Multiplica valores das outras colunas e cria tabela com os valores simetricos */
      for(int j = 1; j < maxj; ++j){
         double elem = SL->Sim.A[k*i+j];

         res[i]   += elem * vet[js+j];
         tab[i+j] += elem * vet[i];
      }
   }

   /* Soma valores simetricos nos resultados */
   for(int i = 0; i < n; i++){
      res[i] += tab[i];
   }

   free(tab);
}

//Multiplica dois vetores (produto escalar)
double multVetVet(double *restrict v1, double *restrict v2, int n){
   double soma = 0.0f;

   for(int i = 0; i < n-n%UNROLL; i+=UNROLL){
      soma += v1[i]*v2[i]     + v1[i+1]*v2[i+1] +
              v1[i+2]*v2[i+2] + v1[i+3]*v2[i+3];
   }
   for(int i = n-n%UNROLL; i < n; ++i)
      soma += v1[i]*v2[i];

   return soma;
}

//Multiplica dois vetores, um deles como se fosse a diagonal de uma matriz e o outro normalmente
void multVetVetDiagonal (double *restrict vetDiagonal, double *restrict v2, double *restrict res, int n) {
   for (int i = 0; i < n; ++i) {
      res[i] = vetDiagonal[i] * v2[i];
   }
}

//Soma cada elemento de um vetor v1 com de o v2 multiplicado por m, devolve um vetor res
// res = v1 +m*v2
void somaVetVet(double *restrict v1, double m, double *restrict v2, double *restrict res, int n){
   for(int i = 0; i < n; ++i)
      res[i] = v1[i] + m*v2[i];
}

//Multiplica um vetor v1 com uma matriz, e o vetor resultante é multiplicado com um vetor v2
//retorna v1*A*v2
double multVetMatVet(double *v1, SistLinear_t *restrict SL, double *v2, int n){
   double *aux = malloc(n*sizeof(double));
   memset(aux, 0, n*sizeof(double));

   multMatSimVet(SL, v1, aux, n);

   //retorna a multiplicacao do vetor resultante com o segundo vetor
   double retorno = multVetVet(aux, v2, n);

   free(aux);

   return retorno;
}

//Multiplica o vetor v2 e a matriz A, e os valores vetor resultante, multiplicados por m, são somados com os valores de v1
//res = v1 + mAv2
void somaVetMatxVet(SistLinear_t *restrict SL, double *restrict v1, double *restrict v2, double m, double *restrict res, int n){
   double *aux = malloc(n*sizeof(double));
   memset(aux, 0, n*sizeof(double));

   multMatSimVet(SL, v2, aux, n);

   for(int i = 0; i < n; ++i){
      res[i] = v1[i] + m*aux[i];
   }

   free(aux);
}

// Encontra a maior diferenca relativa entre as respostas subsequentes
// retorna max ( |xi - xi-1| / |xi| )
double normamax (double *restrict v1, double *restrict v2, int n){
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
double normamaxAbs (double *restrict v1, double *restrict v2, int n){
   double diff;
   double maior = 0.0;

   for (int i = 0; i < n; i++) {
      diff = ( ABS(v1[i] - v2[i]));
      if (diff > maior)
         maior = diff;
   }

   return maior;
}

//Calcula residuo do sistema tridiagonal
double residuoDiag(SistLinear_t *restrict SL, double *restrict b, double *restrict x, double *restrict r, double *restrict tempR){
   LIKWID_MARKER_REGISTER("op2-v2");
   LIKWID_MARKER_START("op2-v2");

   int n = SL->n;
   int k = SL->k;

   *tempR = timestamp();

   /* 
      Primeiras linhas começam deslocadas pra direita
      k/2 -> primeira linha que não tem 0s á esquerda
      k/2-i -> local onde está o primeiro elemento não nulo, que vai multiplicar
      o primeiro elemento do vetor
      m -> elementos do vetor em sequência (0, 1, 2, ..., n-1)
    */
   for(int i = 0; i < k/2; ++i){
      int m = 0;
      for(int j = k/2-i; j < k; ++j){
         r[i] += SL->Diag.A[k*i+j] * x[m];
         m++;
      }
      r[i] -= SL->Diag.b[i];
   }

   /* 
      Usando os valores direto
      js = jstart[i] -> índice da coluna original de onde estava o primeiro elemento não nulo da linha
      jend[i] -> índice da coluna original de onde estava o último elemento não nulo da linha
      j-js -> elementos em sequencia(0, 1, 2, ..., n-1)
      j -> vai do primeiro ao último índice de onde estavam os elementos daquela linha, por ex:
      se o primeiro elemtno não nulo estava na coluna 1, ele está agora na coluna 0 mas ainda 
      deve mutliplicar com x[1], e elemento após esse deverá multiplicar x[2] e assim por diante
   */
   for(int i = k/2; i < n; ++i){
      int js = SL->Diag.jstart[i];

      for(int j = js; j <= SL->Diag.jend[i]; ++j){
         r[i] += SL->Diag.A[k*i+(j-js)] * x[j];
      }
      r[i] -= SL->Diag.b[i];
   }

   double ret = normaL2(r, n);

   LIKWID_MARKER_STOP("op2-v2");
   *tempR = timestamp() - *tempR;

   return ret;
}