#include "rainhas.h"
#include <stdio.h>
#include <stdlib.h>

/* estrutura para as casas proibida */
typedef struct noh_casa {
  casa casa_proibida;
  unsigned int n_casas, n_iteracoes;
  struct noh_casa *next, *prev;
} noh_casa;

/* estrutura para os vertices do grafo */
typedef struct noh_grafo {
  unsigned int vertice, coluna, linha, removido;
  struct noh_grafo *next;
} noh_grafo;

/* estrutura para o grafo */
typedef struct grafo {
  unsigned int n_vertices, vertice_atual;
  struct noh_grafo **nohs;
} grafo;

unsigned int n_rainhas = 0, init = 0, *nao;
noh_casa **proibidas;
grafo tabuleiro;

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
 * @brief inicializa as estruturas das casas proibidas
 *
 * @param *c vetor das casas proibidas
 * @param *r vetor resultado
 * @param n numero de linhas do tabuleiro
 * @param k numero de casas proibidas
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

/*
 * @brief inicializa as estruturas do grafo
 *
 * @param *r vetor resultado
 * @param n numero de linhas do tabuleiro
 * 
 * @return *r
*/
unsigned int *inicializa_grafo (unsigned int *r, unsigned int n);

int eh_casa_proibida (noh_casa **casas, unsigned int linha, unsigned int coluna) {
    noh_casa *aux = casas[linha];

    for (unsigned int i = 0; i < casas[linha] -> n_iteracoes; i++) 
        aux = aux -> next;

    if (aux != NULL && aux -> casa_proibida.coluna == coluna) {
        casas[linha] -> n_iteracoes++;
        return 1;
    }

    return 0;
}

unsigned int *inicializa (casa *c, unsigned int *r, unsigned int n, unsigned int k) {
    noh_casa *noh, *aux;

    /* zera o vetor resposta */
    for (unsigned int i = 0; i < n; i++) 
        r[i] = 0;

    /* aloca vetor de listas */
    proibidas = (noh_casa **) malloc (n * sizeof (noh_casa *));
    if (proibidas == NULL) {
        fprintf (stderr, "# erro ao alocar vetor\n");
        return r;
    }

    /* aloca o primeiro noh de cada lista */
    for (unsigned int i = 0; i < n; i++) {
        proibidas[i] = (noh_casa *) malloc (sizeof (noh_casa));
        if (proibidas[i] == NULL) {
            fprintf (stderr, "# erro ao alocar noh\n");
            return r;
        }
            
        /* incializa o noh */
        proibidas[i] -> casa_proibida.linha = 0;
        proibidas[i] -> casa_proibida.coluna = 0;
        proibidas[i] -> n_casas = 0;
        proibidas[i] -> n_iteracoes = 0;
        proibidas[i] -> next = NULL;
        proibidas[i] -> prev = NULL;
    }
        
    /* aloca os nohs das casas proibidas */
    for (unsigned int i = 0; i < k; i++) {
        noh = (noh_casa *) malloc (sizeof (noh_casa));
        if (proibidas == NULL) {
            fprintf (stderr, "# erro ao alocar noh\n");
            return r;
        }
            
        /* inicializa o noh */
        noh -> casa_proibida = c[i];
        noh -> n_casas = 0;
        noh -> n_iteracoes = 0;
        noh -> next = NULL;
        noh -> prev = NULL;
            
        /* insere o noh na lista de maneira ordenada */
        aux = proibidas[noh -> casa_proibida.linha - 1];
        while (aux -> next != NULL) {
            if (aux -> next -> casa_proibida.coluna > noh -> casa_proibida.coluna) {
                aux -> next -> prev = noh;
                noh -> next = aux -> next;
                break;
            }

            aux = aux -> next;
        }

        noh -> prev = aux;
        aux -> next = noh;
            
        /* incrementa o numero de casas proibidas da linha */
        proibidas[noh -> casa_proibida.linha - 1]->n_casas++;
    }

    return r;
}

void desaloca_memoria (unsigned int n) {
    noh_casa *aux;
    
    /* desaloca memoria dos nohs das listas */
    for (unsigned int i = 0; i < n; i++) {
        aux = proibidas[i];
        if (proibidas[i] -> n_casas > 0) {
            for (unsigned int j = 0; j < proibidas[i] -> n_casas - 1; j++) {
                aux = aux -> next;
            }

            for (unsigned int j = 0; j < proibidas[i] -> n_casas - 1; j++) {
                aux -> next -> next = NULL;
                aux -> next -> prev = NULL;
                free (aux -> next);
                aux = aux -> prev;
            }

            aux -> next -> next = NULL;
            aux -> next -> prev = NULL;
            free (aux -> next);
            aux -> next = NULL;
            free (aux);
        }
        else
            free (aux);
    }
    
    /* desaloca memoria do vetor */
    free (proibidas);
}

unsigned int *inicializa_grafo (unsigned int *r, unsigned int n) {
    noh_grafo *aux, *noh;
    unsigned int linha = 0, coluna = 1;
    int diagonal_e, diagonal_d;

    /* aloca vetor de listas */
    tabuleiro.nohs = (noh_grafo **) malloc ((n * n) * sizeof (noh_grafo *));
    if (tabuleiro.nohs == NULL) {
        fprintf (stderr, "# erro ao alocar vetor\n");
        return r;
    }
    
    /* inicializa informacoes do grafo */
    tabuleiro.n_vertices = n * n;
    tabuleiro.vertice_atual = 0;
    
    /* inicializa informacoes do vetor de proibidas */
    proibidas[0] -> n_iteracoes++;
    
    /* laco entre todos os vertices do grafo */
    for (unsigned int i = 0; i < (n * n); i++) {
        /* aloca vertice */
        tabuleiro.nohs[i] = (noh_grafo *) malloc (sizeof (noh_grafo));
        if (tabuleiro.nohs[i] == NULL) {
            fprintf (stderr, "# erro ao alocar noh\n");
            return r;
        }
        
        /* inicaliza o vertice */
        tabuleiro.nohs[i] -> vertice = i;
        tabuleiro.nohs[i] -> next = NULL;
        tabuleiro.nohs[i] -> coluna = coluna;
        tabuleiro.nohs[i] -> linha = linha;
        tabuleiro.nohs[i] -> removido = 0;
        
        /* inicializa a lista de vizinhos (arcos) */

        /* vizinhos da mesma linha */
        aux = tabuleiro.nohs[i];
        for (unsigned int j = 0; j < (n - coluna); j++) {
            noh = (noh_grafo *) malloc (sizeof (noh_grafo));
            if (noh == NULL) {
                fprintf (stderr, "# erro ao alocar noh\n");
                return r;
            }
            
            noh -> vertice = tabuleiro.nohs[i] -> vertice + j + 1;
            noh -> next = NULL;

            aux -> next = noh;
            aux = aux -> next;
        }
        
        diagonal_e = (int) coluna - 1;
        diagonal_d = (int) coluna + 1;

        /* vizinhos das linhas abaixo */
        for (unsigned int j = 1; j < (n - linha); j++) {
            /* caso a rainha ataque na diagonal esquerda */
            if (diagonal_e > 0) {
                noh = (noh_grafo *) malloc (sizeof (noh_grafo));
                if (noh == NULL) {
                    fprintf (stderr, "# erro ao alocar noh\n");
                    return r;
                }

                noh -> vertice = tabuleiro.nohs[i] -> vertice + (n - 1) * j;
                noh -> next = NULL;

                aux -> next = noh;
                aux = aux -> next;
            }   
            
            /* rainha ataca abaixo */
            noh = (noh_grafo *) malloc (sizeof (noh_grafo));
            if (noh == NULL) {
                fprintf (stderr, "# erro ao alocar noh\n");
                return r;
            }

            noh -> vertice = tabuleiro.nohs[i] -> vertice + n * j;
            noh -> next = NULL;

            aux -> next = noh;
            aux = aux -> next;
            
            /* caso a rainha ataque na diagonal direita */
            if (diagonal_d <= (int) n) {
                noh = (noh_grafo *) malloc (sizeof (noh_grafo));
                if (noh == NULL) {
                    fprintf (stderr, "# erro ao alocar noh\n");
                    return r;
                }

                noh -> vertice = tabuleiro.nohs[i] -> vertice + (n + 1) * j;
                noh -> next = NULL;

                aux -> next = noh;
                aux = aux -> next;
            }

            diagonal_e--;
            diagonal_d++; 
        }
        
        /* verifica se o vertice eh uma casa proibida */
        if (eh_casa_proibida (proibidas, linha, coluna)) {
            tabuleiro.nohs[i] -> removido = 1;
            tabuleiro.n_vertices--;
        }
        
        /* reajusta a coluna e a linha*/
        if (coluna == n) {
            coluna = 0;
            linha++;
            if (linha != n)
                proibidas[linha] -> n_iteracoes++;
        }
        
        coluna++;
    }

    return r;
}
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

unsigned int *rainhas_bt(unsigned int n, unsigned int k, casa *c, unsigned int *r) {
    unsigned int diagonal_e, diagonal_d, cima, linha, possivel, n_comparacoes;

    /* inicializacao */
    if (n_rainhas == 0) 
        inicializa (c, r, n, k);

    /* condicao de sucesso */ 
    if (n_rainhas == n) {
        desaloca_memoria (n);
        n_rainhas = 0;
        return r;
    }
  
    /* 
     * linha a ser avaliada 
     * linha 0 = linha 1 e consequentemente... 
    */
    linha = n_rainhas;
    
    /* incrementa o numero de iteracoes para localizar o proximo noh */
    proibidas[linha] -> n_iteracoes++;

    /* laco entre as colunas */
    for (unsigned int coluna = 1; coluna <= n; coluna++) {
        /* atribuicao dos ataques da rainha */
        diagonal_e = coluna - 1, 
        diagonal_d = coluna + 1, 
        cima       = coluna;

        /* flag para possibilidade de rainha */
        possivel = 1;
        
        /* verifica se a casa atual eh proibida */
        if (eh_casa_proibida (proibidas, linha, coluna)) {
            n_comparacoes = n_rainhas;
            possivel = 0;
        }
        else
            n_comparacoes = 0;

        /* laco para comparacao entre as linhas anteriores*/
        for (unsigned int i = n_comparacoes; i < n_rainhas; i++) {
            /* verifica se a possivel rainha ataca outra */
            if (r[linha - 1 - i] == diagonal_e  || 
                r[linha - 1 - i] == diagonal_d  || 
                r[linha - 1 - i] == cima) {
                    possivel = 0;
                    break;
            }

            /* atualiza as cordenadas de ataque */
            diagonal_e--;
            diagonal_d++;
        }
        
        if (possivel) {
            /* adiciona a rainha ao tabuleiro */
            r[linha] = coluna;
            n_rainhas++;
            
            /* backtracking */
            unsigned int *retorno = rainhas_bt (n, k, c, r);

            /* retorno bem sucedido */
            if (retorno != nao) 
                return r;
            
            /* retira a rainha do tabuleiro */
            r[linha] = 0;
            n_rainhas--;
            proibidas[linha + 1] -> n_iteracoes = 0;
        }
    }
    
    /* desaloca memoria */
    if (n_rainhas == 0) 
        desaloca_memoria (n);

    /* retorno mal sucedido */
    return nao;
}

//------------------------------------------------------------------------------
// computa uma resposta para a instância (n,c) do problema das n
// rainhas com casas proibidas usando a modelagem do problema como
// conjunto independente de um grafo
//
// n, c e r são como em rainhas_bt()

unsigned int *rainhas_ci(unsigned int n, unsigned int k, casa *c, unsigned int *r) {
    noh_grafo *aux;
    unsigned int linha = 0, coluna = 1, n_vizinhos = 0;
    unsigned int *vizinhos = (unsigned int *) malloc ((3 * n) * sizeof (unsigned int));
    
    /*
    * Grafo C = tabuleiro
    * Grafo I = r
    */

    /* inicializacao */
    if (init == 0) {
        inicializa (c, r, n, k);

        inicializa_grafo (r, n);

        desaloca_memoria (n);

        init = 1;
    }
    
    /* 
     * linha a ser avaliada 
     * linha 0 = linha 1 e consequentemente... 
    */
    linha = n_rainhas;
    
    /* 
     * condicao de sucesso 
     * |I| = n
    */
    if (n_rainhas == n) 
        return r;
        
    /* 
     * retorno mal sucedido
     * |I| + |C| < n
     * ou caso a linha que esta sendo avaliada nao seja a proxima 
     */
    if (((n_rainhas + tabuleiro.n_vertices) < n) || linha != tabuleiro.nohs[tabuleiro.vertice_atual] -> linha) 
        return nao;
    
    /* procura vertice que pertence a C */
    while (tabuleiro.nohs[tabuleiro.vertice_atual] -> removido) 
        tabuleiro.vertice_atual++;
    
    /* remove o vertice e seus vizinhos de C */
    aux = tabuleiro.nohs[tabuleiro.vertice_atual];
    while (aux != NULL) {
        /* verifica se o vertice ja foi removido */
        if (!tabuleiro.nohs[aux -> vertice] -> removido) {
            tabuleiro.nohs[aux -> vertice] -> removido = 1;
            vizinhos[n_vizinhos] = aux -> vertice;
            tabuleiro.n_vertices--;
            n_vizinhos++;
        }

        aux = aux -> next;
    }
    
    /* adiciona o vertice a I */
    r[linha] = tabuleiro.nohs[tabuleiro.vertice_atual] -> coluna;
    n_rainhas++;
    
    /* encontra proximo vertice a ser avaliado */
    coluna = tabuleiro.nohs[tabuleiro.vertice_atual] -> coluna;
    if (linha != (n - 1)) {
        for (unsigned int i = 0; i < n; i++) {
            if (!tabuleiro.nohs[tabuleiro.vertice_atual + (n - coluna + 1)  + i] -> removido) {
                tabuleiro.vertice_atual = tabuleiro.vertice_atual + (n - coluna + 1)  + i;
                break;
            }
        }
    }
    
    /* 
    * Grafo I (r) com o vertice adicionado
    * Grafo C (tabuleiro) sem o vertice e seus vizinhos
    */
    unsigned int *retorno = rainhas_ci (n, k, c, r);

    /* retorno bem sucedido */
    if (retorno != nao) 
        return r;
    
    /* retira a rainha do tabuleiro */
    r[linha] = 0;
    n_rainhas--;

    /* recoloca o vertice e seus vizinhos no tabuleiro */
    for (unsigned int i = 0; i < n_vizinhos; i++) {
        tabuleiro.nohs[vizinhos[i]] -> removido = 0;
        tabuleiro.n_vertices++;
    }
    
    /* avalia o proximo vertice */
    if (n_vizinhos > 1)  
        tabuleiro.vertice_atual = vizinhos[1];
    
    /* 
     * retorno mal sucedido
     * caso a linha que esta sendo avaliada nao seja a proxima 
     * ou nao existam vizinhoh a serem avaliados
     */
    if (linha != tabuleiro.nohs[vizinhos[1]] -> linha || n_vizinhos == 1) {
        return nao;
    }
    
    /* 
    * Grafo I (r) sem o vertice adicionado
    * Grafo C (tabuleiro) com o vertice e seus vizinhos
    */
    return rainhas_ci (n, k, c, r);
}