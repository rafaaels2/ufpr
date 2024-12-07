/*  Rafael Gonçalves dos Santos - GRR20211798 */

#include "lp.h"

int main (int argc, char **argv) {
    int n;
    char  *entrada = malloc (sizeof (char) * MAX_TAM);
    float *entrada_float = calloc (MAX_TAM, sizeof (float));

    /* le a entrada e pega o numero de elementos */
    n = le_entrada (entrada, entrada_float);

    /* inicializa o vetor dos valores com tamanho n */
    float *numeros = malloc (sizeof (float) * n);
    copia_vetor (entrada_float, numeros, n);

    /* inicializa o lp */
    lp_solve_t *lp_solve = malloc (sizeof (lp_solve_t));
    lp_init (lp_solve, numeros[0], numeros[1]);

    /* modela o problema para o lp_solver */
    lp_solve = modela_problema (lp_solve, numeros);

    libera_mem (entrada, entrada_float, numeros, lp_solve);

    return 0;
}
