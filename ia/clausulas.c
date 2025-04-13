#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "comb.h"

#define MAX_TAM 1024
#define N_VIZINHOS 8

/* le entrada ignorando tabs, espaços e quebras de linhas */
int le_entrada (char *entrada, int *numeros, int *linhas, int *colunas) {
    int n = 0;
    
    /* verifica quantidade minima de entradas */
    while (n < 2 || n < (2 + numeros[0] * numeros[1])) {
        if (fgets (entrada, MAX_TAM, stdin) != NULL) {
            entrada[strcspn (entrada, "\n")] = '\0';

            char *aux = entrada;

            while (*aux) {
                /* le o valor se for um numero */
                if (isdigit (*aux)) {
                    sscanf (aux, "%d", &numeros[n++]);

                    while (isdigit (*aux)) aux++;
                }
                else {
                    aux++;
                }
            }
        }
    }

    *linhas  = numeros[0];
    *colunas = numeros[1];

    return n - 2;
}

int main () {
    int n, linhas, colunas, index, index_vizinhos, num, k, n1, n1Min;
    int entrada_int[MAX_TAM];

    char entrada[MAX_TAM];
    
    FILE *arquivo;
    FILE *arquivo_tmp;
    
    espec_t espec;

    /* cria o arquivo */
    arquivo = fopen ("clausulas.txt", "w");
    fclose(arquivo);

    /* le a entrada do STDIN */
    n = le_entrada (entrada, entrada_int, &linhas, &colunas);

    int vizinhos[n][N_VIZINHOS + 1];
    int saida[n];
    int saidaMin[n];
    int output[n];

    /* aloca as celulas no tabulerio */
    int tabuleiro_t1[n];
    for (int i = 0; i < n; i++)
        tabuleiro_t1[i] = entrada_int[i + 2];

    num = 0;

    /* pega os vizihos da cada celula */
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            index = i * colunas + j;

            index_vizinhos = 0;

            /* de cima */
            if (i > 0) { 
                /* diagonal superior direita */
                if (j > 0) 
                    vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + (j - 1)) + 1; 

                /* em cima */
                vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + j) + 1;               

                /* diagonal superior esquerda */
                if (j < colunas - 1) 
                    vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + (j + 1)) + 1;
            }

            /* esquerda */
            if (j > 0) 
                vizinhos[index][index_vizinhos++] = (i * colunas + (j-1)) + 1;  

            /* direita */
            if (j < colunas - 1) 
                vizinhos[index][index_vizinhos++] = (i * colunas + (j+1)) + 1; 

            /* de baixo */
            if (i < linhas - 1) {  
                /* diagonal inferior esquerda */
                if (j > 0) 
                    vizinhos[index][index_vizinhos++] = ((i+1) * colunas + (j-1)) + 1; 

                /* abaixo */
                vizinhos[index][index_vizinhos++] = ((i+1) * colunas + j) + 1;  

                /* diagonal inferior direita */
                if (j < colunas - 1) 
                    vizinhos[index][index_vizinhos++] = ((i+1) * colunas + (j+1)) + 1;  
            }

            /* atribui o numero de vizinhos */
            vizinhos[index][N_VIZINHOS] = index_vizinhos;

            if (tabuleiro_t1[index] == 1) {
                /* 1) Loneliness: A cell with fewer than 2 live neighbours (at least 7 dead neighbours) at time t0 is dead at time t1. */
                espec = defSpec (espec, 1, 0, 0);
                num = gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 7, arquivo, espec, &num);

                /* 2) Stagnation: A dead cell with exactly two live neighbours at time t0 will still be dead at time t1 */
                espec = defSpec (espec, -1, 1, index + 1);
                num = gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 2, arquivo, espec, &num);

                /* 3) Overcrowding: A cell with four or more live neighbours at time t0 will be dead at time t1 */
                espec = defSpec (espec, -1, 0, 0);
                num = gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 4, arquivo, espec, &num);
            }
            else {
                /* 4) Preservation: A cell that is alive at time t0 with exactly two live neighbours will remain alive at time t1 */
                espec = defSpec (espec, -1, 1, (index + 1) * -1);
                num = gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 2, arquivo, espec, &num);

                /* 5) Life: A cell with exactly 3 live neighbours at time t0 will be alive at time t1, irrespective of its prior state */
                espec = defSpec (espec, -1, 1, 0);
                num = gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 3, arquivo, espec, &num);
            }
        }
    }

    /* ------ colocar cabecalho do satsolver ------ */

    /* cria arquivo temporario */
    arquivo = fopen ("clausulas.txt", "r");
    arquivo_tmp = fopen ("tmp.txt", "w");

    /* escreve cabecalho */
    fprintf (arquivo_tmp, "p cnf %d %d\n", n, num);

    /* copia o arquivo original para o temporario */
    char buffer[MAX_TAM];
    while (fgets (buffer, sizeof (buffer), arquivo) != NULL) 
        fputs (buffer, arquivo_tmp);
    
    /* fecha arquivos */
    fclose (arquivo);
    fclose (arquivo_tmp);

    /* remove o antigo arquivo e muda o temporario para original */
    remove ("clausulas.txt");
    rename ("tmp.txt", "clausulas.txt");

    n1Min = n;

    /* linha de comando do programa */
    int ret = system ("./mergesat clausulas.txt > saida.txt");

    /* enquanto for sat */
    while (WEXITSTATUS (ret) == 10) {
        arquivo = fopen ("saida.txt", "r");

        n1 = 0;
        k  = 0;

        /* le as respostas do satsolver e as armazena no vetor *saida */
        while (fgets (buffer, sizeof (buffer), arquivo) != NULL) {
            if (buffer[0] == 'v'){
                buffer[strcspn (buffer, "\n")] = '\0';
                
                char *aux = buffer;

                while (*aux) {
                    /* le o valor se for um numero */
                    if (isdigit (*aux) || *aux == '-') {
                        sscanf (aux, "%d", &saida[k++]);

                        while (isdigit (*aux) || *aux == '-') aux++;
                    }
                    else {
                        aux++;
                    }
                }
            }
        }

        fclose (arquivo);

        /* calcula a quantidade de celulas vivas na saida */
        for (int i = 0; i < linhas * colunas; i++) {
            if (saida[i] > 0) {
                n1++;
            }
        }

        /* define saida com menor numero de 1's */
        if (n1 < n1Min) {
            for (int i = 0; i < n; i++)
                saidaMin[i] = saida[i];

            n1Min = n1;
        }

        /* coloca a nova clausula */

        arquivo_tmp = fopen ("clausulas.txt", "a");

        /* negacao da resposta encontrada pelo satsolver */
        for (int i = 0; i < n; i++)
            fprintf (arquivo_tmp, "%d ", saida[i] * -1);
        fprintf (arquivo_tmp, "0\n");
        num++;

        fclose (arquivo_tmp);

        /* ------ colocar cabecalho do satsolver ------ */

        arquivo = fopen ("clausulas.txt", "r");
        arquivo_tmp = fopen ("tmp.txt", "w");

        /* escreve cabecalho */
        fprintf (arquivo_tmp, "p cnf %d %d\n", n, num);
        
        /* pula a primeira linha */
        if (fgets (buffer, sizeof (buffer), arquivo) == NULL) {}

        /* copia o arquivo original para o temporario */
        while (fgets (buffer, sizeof (buffer), arquivo) != NULL) 
            fputs (buffer, arquivo_tmp);
        
        /* fecha arquivos */
        fclose (arquivo);
        fclose (arquivo_tmp);

        /* remove o antigo arquivo e muda o temporario para original */
        remove ("clausulas.txt");
        rename ("tmp.txt", "clausulas.txt");

        /* coloca a nova clausula */
        ret = system ("./mergesat clausulas.txt > saida.txt");
    }

    remove ("clausulas.txt");

    /* copia vetor para output, impressao do saidaMin com problema ??? */
    for (int i = 0; i < n; i++) 
        output[i] = saidaMin[i];

    /* imprime a saida */
    printf("%d %d\n", linhas, colunas);
    for (int i = 0; i < n; i++) {
        if (output[i] < 0)
            printf("0 ");
        else
            printf("1 ");
        if ((i + 1) % colunas == 0) {
            printf("\n");
        }
    }
    
    return 0;
}