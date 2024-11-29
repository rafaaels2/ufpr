#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/resource.h>

#define MAX_TAM 1024

int le_entrada (char *entrada, int *numeros) {
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

    return n;
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

    printf ("Memória máxima definida para %d GB\n", tam);

    return 1;
}

int main () {
    limita_memoria (8);

    int n;
    char *entrada = malloc (sizeof (char) * MAX_TAM);
    int *entrada_int = calloc (MAX_TAM, sizeof (float));

    n = le_entrada (entrada, entrada_int);

    free (entrada);

    /* 
     * posicao 0 = numero de linhas 
     * posicao 1 = numero de colunas
     * posicao [x] = estado da celula
     * 
     * celulas vao de 1 ate n, representadas por: tabuleiro_t1[x + 1]
     * ex: celula 1 = tabuleiro_t1[2]
     * ex: celula 8 = tabuleiro_t1[9]
     */
    int *tabuleiro_t1 = malloc (sizeof (int) * n);
    for (int i = 0; i < n; i++)
        tabuleiro_t1[i] = entrada_int[i];

    free (entrada_int);
    
    /* inicializa tabuleiro t0 */
    int *tabuleiro_t0 = malloc (sizeof (int) * n);
    tabuleiro_t0[0] = tabuleiro_t1[0];
    tabuleiro_t0[1] = tabuleiro_t1[0];

    free (tabuleiro_t1);
    free (tabuleiro_t0);

    return 0;
}