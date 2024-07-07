#ifndef RAINHAS_H
#define RAINHAS_H

//------------------------------------------------------------------------------
// representa uma casa do tabuleiro
//

typedef struct casa {
  unsigned int linha, coluna;
} casa;

/* estrutura para as casas proibida */
typedef struct noh_casa {
  casa casa_proibida;
  unsigned int n_casas, n_iteracoes;
  struct noh_casa *next, *prev;
} noh_casa;

/* estrutura para os vertices do grafo */
typedef struct noh_grafo {
  unsigned int vertice, coluna, linha;
  struct noh_grafo *next;
} noh_grafo;

typedef struct grafo {
  unsigned int n_vertices, vertice_atual;
  struct noh_grafo **nohs;
} grafo;

/*
 * @brief imprime o vetor de listas
 *
 * @param **proibidas vetor de listas
 * @param n tamanho do vetor
*/
void print_proibidas (noh_casa **proibidas, unsigned int n);

/*
 * @brief verifica se a casa eh proibida
 *
 * @param **casas vetor de listas
 * @param linha valor da linha - 1
 * @param coluna valor da coluna
 * 
 * @return 1 eh uma casa proibida
 * @return 0 nao eh uma casa proibida
*/
int eh_casa_proibida (noh_casa **casas, unsigned int linha, unsigned int coluna);

/*
 * @brief inicializa as estruturas
 *
 * @param *c vetor das casas proibidas
 * @param *r vetor resultado
 * @param n numero de linhas do tabuleiro
 * @param numero de casas proibidas
 * 
 * @return *r
*/
unsigned int *inicializa (casa *c, unsigned int *r, unsigned int n, unsigned int k);

/*
 * @brief desaloca a memoria do vetor de listas
 *
 * @param n numero de linhas do tabuleiro
*/
void desaloca_memoria (unsigned int n);

//------------------------------------------------------------------------------
// computa uma resposta para a instância (n,c) do problema das n rainhas 
// com casas proibidas usando backtracking
//
//    n é o tamanho (número de linhas/colunas) do tabuleiro
//
//    c é um vetor de k 'struct casa' indicando as casas proibidas
//
//    r é um vetor de n posições (já alocado) a ser preenchido com a resposta:
//      r[i] = j > 0 indica que a rainha da linha i+1 fica na coluna j;
//      r[i] = 0     indica que não há rainha nenhuma na linha i+1
//
// devolve r

unsigned int *rainhas_bt(unsigned int n, unsigned int k, casa *c, unsigned int *r);

//------------------------------------------------------------------------------
// computa uma resposta para a instância (n,c) do problema das n rainhas 
// com casas proibidas usando backtracking
//
// n, c, r e o valor de retorno são como em rainhas_bt

unsigned int *rainhas_ci(unsigned int n, unsigned int k, casa *c, unsigned int *r);

#endif
