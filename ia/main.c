#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#define MAX_TAM 1024
#define N_VIZINHOS 8


int n_clausulas = 0; 

int le_entrada (char *entrada, int *numeros, int *linhas, int *colunas) {
    int n = 0;
    
    while (n < 2 || n < (2 + numeros[0] * numeros[1])) {
        fgets (entrada, MAX_TAM, stdin);
        entrada[strcspn (entrada, "\n")] = '\0';

        char *aux = entrada;

        while (*aux) {
            if (isdigit (*aux)) {
                sscanf (aux, "%d", &numeros[n++]);

                while (isdigit (*aux)) aux++;
            }
            else {
                aux++;
            }
        }
    }

    *linhas  = numeros[0];
    *colunas = numeros[1];

    return n - 2;
}

/* define o limite de GB do sistema */
int limita_memoria (int tam) {
    struct rlimit lim;

    lim.rlim_cur = 1024L * 1024L * 1024L * tam; 
    lim.rlim_max = 1024L * 1024L * 1024L * tam;

    if (setrlimit (RLIMIT_AS, &lim) != 0) {
        perror ("# setrlimit falhou");
        return 0;
    }

    return 1;
}

void combinar(int *vizinhos, int n, int *combinacao, int inicio, int profundidade, int tamanho, FILE *arquivo, int sinal) {
    if (profundidade == tamanho) {
        n_clausulas++;

        for (int i = 0; i < tamanho; i++) {
            fprintf (arquivo, "%d ", combinacao[i]);
        }
        fprintf (arquivo, "0\n");
        return;
    }

    for (int i = inicio; i < n; i++) {
        combinacao[profundidade] = vizinhos[i] * sinal;  
        combinar (vizinhos, n, combinacao, i + 1, profundidade + 1, tamanho, arquivo, sinal);  
    }
}

void gerar_combinacoes(int *vizinhos, int n, int tamanho, FILE *arquivo, int sinal) {
    int *combinacao = malloc (sizeof(int) * tamanho);

    if (n < tamanho) 
        tamanho = n; 

    arquivo = fopen ("clausulas.txt", "a");

    combinar (vizinhos, n, combinacao, 0, 0, tamanho, arquivo, sinal);

    fclose(arquivo);

    free (combinacao);
}

int main () {
    limita_memoria (8);

    int k;
    int n, linhas, colunas;
    char *entrada = malloc (sizeof (char) * MAX_TAM);
    int *entrada_int = calloc (MAX_TAM, sizeof (float));

    double tempo;

    n = le_entrada (entrada, entrada_int, &linhas, &colunas);

    free (entrada);

    FILE *arquivo = fopen ("clausulas.txt", "w");
    fclose(arquivo);

    int *tabuleiro_t1 = malloc (sizeof (int) * n);
    for (int i = 0; i < n; i++)
        tabuleiro_t1[i] = entrada_int[i + 2];

    free (entrada_int);

    int **vizinhos = malloc (sizeof (int *) * n);

    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            int index = i * colunas + j;

            vizinhos[index] = calloc (N_VIZINHOS + 1, sizeof (int));

            /* pega vizinhos */
            int index_vizinhos = 0;

            if (i > 0) { 
                if (j > 0) 
                    vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + (j - 1)) + 1; 

                vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + j) + 1;               

                if (j < colunas - 1) 
                    vizinhos[index][index_vizinhos++] = ((i - 1) * colunas + (j + 1)) + 1;
            }

            if (j > 0) 
                vizinhos[index][index_vizinhos++] = (i * colunas + (j-1)) + 1;  

            if (j < colunas - 1) 
                vizinhos[index][index_vizinhos++] = (i * colunas + (j+1)) + 1; 

            if (i < linhas - 1) {  
                if (j > 0) 
                    vizinhos[index][index_vizinhos++] = ((i+1) * colunas + (j-1)) + 1; 

                vizinhos[index][index_vizinhos++] = ((i+1) * colunas + j) + 1;  

                if (j < colunas - 1) 
                    vizinhos[index][index_vizinhos++] = ((i+1) * colunas + (j+1)) + 1;  
            }

            vizinhos[index][N_VIZINHOS] = index_vizinhos;

            if (tabuleiro_t1[index] == 1) {
                /* --------------------------------------------------------------------------- */

                /* verifica se é >= 3 */
                gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 6, arquivo, 1);

                /* verifica se é <= 3 */
                gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 4, arquivo, -1);

                /* -x */
                arquivo = fopen ("clausulas.txt", "a");
                fprintf (arquivo, "%d 0\n", (index + 1) * -1);
                n_clausulas++;
                fclose (arquivo);

                /* --------------------------------------------------------------------------- */

                /* verifica se é >= 2 */
                gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 7, arquivo, 1);

                /* verifica se é <= 3 */
                gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 4, arquivo, -1);

                /* x */
                arquivo = fopen ("clausulas.txt", "a");
                fprintf (arquivo, "%d 0\n", (index + 1) * 1);
                n_clausulas++;
                fclose (arquivo);
            }
            else {
                /* --------------------------------------------------------------------------- */

                /* verifica se é <= 1 */
                gerar_combinacoes (vizinhos[index], vizinhos[index][N_VIZINHOS], 2, arquivo, -1);

                /* x */
                arquivo = fopen ("clausulas.txt", "a");
                fprintf (arquivo, "%d 0\n", (index + 1) * 1);
                n_clausulas++;
                fclose (arquivo);

                /* --------------------------------------------------------------------------- */
            }
        }
    }

    arquivo = fopen ("clausulas.txt", "r");
    FILE *arquivo_tmp = fopen ("tmp.txt", "w");

    fprintf (arquivo_tmp, "p cnf %d %d\n", n, n_clausulas);

    char buffer[MAX_TAM];
    while (fgets (buffer, sizeof (buffer), arquivo) != NULL) 
        fputs (buffer, arquivo_tmp);
    
    fclose (arquivo);
    fclose (arquivo_tmp);

    remove ("clausulas");
    rename ("tmp.txt", "clausulas.txt");

    free (tabuleiro_t1);
    for (int i = 0; i < n; i++)
        free (vizinhos[i]);
    free (vizinhos);

    return 0;
}