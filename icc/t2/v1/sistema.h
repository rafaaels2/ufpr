//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#ifndef __SISTEMA_H__
#define __SISTEMA_H__

// Estrutura para definiçao de um sistema linear qualquer
typedef struct {
   double **A; // coeficientes
   double *b; // termos independentes
   unsigned int n; // tamanho do SL
   unsigned int i; //numero maximo de iteracoes
} SistLinear_t;

void liberaSisLin (SistLinear_t *SL);

SistLinear_t* alocaSisLin (unsigned int n);

SistLinear_t *lerSisLin ();

void prnSisLin (SistLinear_t *SL);

#endif