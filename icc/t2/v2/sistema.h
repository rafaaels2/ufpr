//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#ifndef __SISTEMA_H__
#define __SISTEMA_H__

/* Estrutura de uma matriz com apenas as diagonais */
typedef struct {
   double *A;                // coeficientes
   double *b;               // termos independetes originais
   int *jstart;     // posicoes de inicio
   int *jend;       // posicoes de fim
} MatDiag_t;

/* Estrutura de uma matriz simetrica */
typedef struct {
   double *A;               // coeficientes
   double *b;               // termos independetes modificados
   int *jstart;      // posicoes de inicio
   int *jend;        // posicoes de fim
} MatSim_t;

// Estrutura para definiçao de um sistema linear qualquer
typedef struct {
   MatDiag_t Diag;      // Matriz com diagonais (Matriz original)
   MatSim_t Sim;       // Matriz principal simetrizada
   unsigned int n;      // tamanho do SL
   unsigned int i;      //numero maximo de iteracoes
   unsigned int k;      //numero maximo de iteracoes
} SistLinear_t;

void liberaSisLin (SistLinear_t *SL);

SistLinear_t* alocaSisLin (unsigned int n, unsigned int k);

void prnSisLinDiag (SistLinear_t *SL);

void prnSisLinSim (SistLinear_t *SL);

#endif