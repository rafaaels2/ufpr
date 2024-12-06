/*  Rafael Gonçalves dos Santos - GRR20211798 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lp.h"

int main (int argc, char **argv) {
    int n;
    char  *entrada = malloc (sizeof (char) * MAX_TAM);
    float *entrada_float = calloc (MAX_TAM, sizeof (float));

    n = le_entrada (entrada, entrada_float);

    float *numeros = malloc (sizeof (float) * n);
    for (int i = 0; i < n; i++)
        numeros[i] = entrada_float[i];

    lp_solve_t *lp_solve = malloc (sizeof (lp_solve_t));

    lp_solve -> objetivo    = malloc (sizeof (float) * numeros[0]);
    lp_solve -> limites_inf = malloc (sizeof (float) * numeros[0]);
    lp_solve -> restricoes  = malloc (sizeof (float *) * numeros[1]);
    for (int i = 0; i < numeros[1]; i++)
        lp_solve -> restricoes[i] = malloc (sizeof (float) * (numeros[0] + 1));

    lp_solve = modela_problema (lp_solve, numeros);

    libera_mem (entrada, entrada_float, numeros, lp_solve);

    return 0;
}
