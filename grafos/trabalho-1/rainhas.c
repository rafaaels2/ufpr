#include "rainhas.h"
#include <stdio.h>
#include <stdlib.h>

unsigned int n_rainhas = 0, init = 0, *nao;
noh_casa **proibidas;
grafo tabuleiro;

void print_proibidas (noh_casa **casas, unsigned int n) {
    noh_casa *aux;
    printf ("\n# VETOR DE LISTAS\n");
    for (unsigned int i = 0; i < n; i++) {
        aux = casas[i];
        printf ("# [%d] = ", i);

        while (aux -> next != NULL) {
            printf ("c: %d", aux -> casa_proibida.coluna);
            aux = aux -> next;
            printf (" > ");
        }

        printf ("c: %d ", aux -> casa_proibida.coluna);
        printf ("| n: %d\n", casas[i] -> n_casas);
    }
}

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
    noh_grafo *aux, *noh;
    unsigned int linha = 0, coluna = 1, n_vizinhos = 0;
    unsigned int vizinhos[2 * n + 1];
    int diagonal_e, diagonal_d;
    
    if (init == 0) {
        inicializa (c, r, n, k);

        /* aloca vetor de listas */
        tabuleiro.nohs = (noh_grafo **) malloc ((n * n) * sizeof (noh_grafo *));
        if (tabuleiro.nohs == NULL) {
            fprintf (stderr, "# erro ao alocar vetor\n");
            return r;
        }

        tabuleiro.n_vertices = n * n;
        tabuleiro.vertice_atual = 0;
        
        proibidas[0] -> n_iteracoes++;

        for (unsigned int i = 0; i < (n * n); i++) {
            tabuleiro.nohs[i] = (noh_grafo *) malloc (sizeof (noh_grafo));
            if (tabuleiro.nohs[i] == NULL) {
                fprintf (stderr, "# erro ao alocar noh\n");
                return r;
            }
            
            tabuleiro.nohs[i] -> vertice = i;
            tabuleiro.nohs[i] -> next = NULL;
            tabuleiro.nohs[i] -> coluna = coluna;
            tabuleiro.nohs[i] -> linha = linha;
            tabuleiro.nohs[i] -> removido = 0;

            /* cria a lista na linha */
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

            /* cria a lista nas linhas abaixo s*/
            for (unsigned int j = 1; j < (n - linha); j++) {
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

                noh = (noh_grafo *) malloc (sizeof (noh_grafo));
                if (noh == NULL) {
                    fprintf (stderr, "# erro ao alocar noh\n");
                    return r;
                }

                noh -> vertice = tabuleiro.nohs[i] -> vertice + n * j;
                noh -> next = NULL;

                aux -> next = noh;
                aux = aux -> next;

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
            
            if (eh_casa_proibida (proibidas, linha, coluna)) {
                tabuleiro.nohs[i] -> removido = 1;
                tabuleiro.n_vertices--;
            }

            if (coluna == n) {
                coluna = 0;
                linha++;
                if (linha != n)
                    proibidas[linha] -> n_iteracoes++;
            }
            
            coluna++;
        }
        init = 1;
    }

    linha = n_rainhas;

    if (n_rainhas == n) {
        return r;
    }

    if (((n_rainhas + tabuleiro.n_vertices) < n) || linha != tabuleiro.nohs[tabuleiro.vertice_atual] -> linha) {
        return nao;
    }

    while (tabuleiro.nohs[tabuleiro.vertice_atual] -> removido) {
        tabuleiro.vertice_atual++;
    }
    
    /* remove do grafo C */
    aux = tabuleiro.nohs[tabuleiro.vertice_atual];
    while (aux != NULL) {
        if (!tabuleiro.nohs[aux -> vertice] -> removido) {
            tabuleiro.nohs[aux -> vertice] -> removido = 1;
            vizinhos[n_vizinhos] = aux -> vertice;
            tabuleiro.n_vertices--;
            n_vizinhos++;
        }

        aux = aux -> next;
    }
    
    /* adiciona a I */
    r[linha] = tabuleiro.nohs[tabuleiro.vertice_atual] -> coluna;
    exit(0);
    n_rainhas++;
    
    /* encontra proximo vertice */
    coluna = tabuleiro.nohs[tabuleiro.vertice_atual] -> coluna;
    if (linha != (n - 1)) {
        for (unsigned int i = 0; i < n; i++) {
            if (!tabuleiro.nohs[tabuleiro.vertice_atual + (n - coluna + 1)  + i] -> removido) {
                tabuleiro.vertice_atual = tabuleiro.vertice_atual + (n - coluna + 1)  + i;
                break;
            }
        }
    }

    unsigned int *retorno = rainhas_ci (n, k, c, r);

    /* retorno bem sucedido */
    if (retorno != nao) {
        return r;
    }
    
    /* retira a rainha do tabuleiro */
    r[linha] = 0;
    n_rainhas--;
    for (unsigned int i = 0; i < n_vizinhos; i++) {
        tabuleiro.nohs[vizinhos[i]] -> removido = 0;
        tabuleiro.n_vertices++;
    }

    if (n_vizinhos > 1)  
        tabuleiro.vertice_atual = vizinhos[1];

    if (linha != tabuleiro.nohs[vizinhos[1]] -> linha || n_vizinhos == 1) {
        return nao;
    }
    
    return rainhas_ci (n, k, c, r);
}