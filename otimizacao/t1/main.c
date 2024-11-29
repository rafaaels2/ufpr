/*  Rafael Gonçalves dos Santos - GRR20211798 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TAM 1024

typedef struct lp_solve {
    float *objetivo;
    float *limites_inf;
    float **restricoes;   
} lp_solve_t;

int le_entrada (char *entrada, float *numeros) {
    int n = 0;
    
    while (n < 2 || n < ((2) + (numeros[0]) + (2 * numeros[1]) + (numeros[0] * numeros[1]))) {
        fgets (entrada, MAX_TAM, stdin);
        entrada[strcspn (entrada, "\n")] = '\0';

        char *aux = entrada;

        while (*aux) {
            if (isdigit (*aux)) {
                sscanf (aux, "%f", &numeros[n++]);

                while (isdigit (*aux) || (strncmp (aux, ".", 1) == 0)) aux++;
            }
            else {
                aux++;
            }
        }
    }

    return n;
}

lp_solve_t *modela_problema (lp_solve_t *lp_solve, float *numeros) {
    int xi = 0;
    int li = 0;
    int ri = 0;

    /* objetivo */
    for (int i = 0; i < numeros[0]; i++) {
        float valor = 0;
        float custo = 0;

        valor = numeros[i + 2];

        for (int j = 0; j < numeros[1]; j++) 
            custo += (numeros[(j * 2) + 2 + (int) numeros[0]]) * (numeros[j + (i * (int) numeros[1]) + 2 + (int) numeros[0] + 2 * (int) numeros[1]]);

        lp_solve -> objetivo[xi++] = valor - custo;
    }

    printf ("max:");
    for (int i = 0; i < numeros[0] - 1; i++)
        printf (" %.2fx%d +", lp_solve -> objetivo[i], i + 1);
    printf (" %.2fx%d", lp_solve -> objetivo[xi - 1], xi);
    printf (";\n");

    /* limites inferiores */
    for (int i = 0; i < numeros[0]; i++) {
        lp_solve -> limites_inf[li++] = 0;
    }

    for (int i = 0; i < numeros[0]; i++)
        printf ("x%d >= %.2f;\n", i + 1, lp_solve -> limites_inf[i]);

    /* restricoes */
    for (int i = 0; i < numeros[1]; i++) {
        ri = 0;

        for (int j = 0; j < numeros[0]; j++) {
            lp_solve -> restricoes[i][ri++] = numeros[2 + (int) numeros[0] + (2 * (int) numeros[1]) + (j * (int) numeros[1]) + (i * 1)];
        }
        lp_solve -> restricoes[i][ri] = numeros[3 + (int) numeros[0] + (i * 2)];
    }

    for (int i = 0; i < numeros[1]; i++) {
        for (int j = 0; j < numeros[0] - 1; j++) {
            printf ("%.2fx%d + ", lp_solve -> restricoes[i][j], j + 1);
        }
        printf ("%.2fx%d", lp_solve -> restricoes[i][ri - 1], ri);
        printf (" <= %.2f;\n", lp_solve -> restricoes[i][ri]);
    }

    return lp_solve;
}

int main (int argc, char **argv) {
    int n;
    char  *entrada = malloc (sizeof (char) * MAX_TAM);
    float *entrada_float = calloc (MAX_TAM, sizeof (float));

    n = le_entrada (entrada, entrada_float);

    free (entrada);

    float *numeros = malloc (sizeof (float) * n);
    for (int i = 0; i < n; i++)
        numeros[i] = entrada_float[i];

    free (entrada_float);

    lp_solve_t *lp_solve = malloc (sizeof (lp_solve_t));

    lp_solve -> objetivo    = malloc (sizeof (float) * numeros[0]);
    lp_solve -> limites_inf = malloc (sizeof (float) * numeros[0]);
    lp_solve -> restricoes  = malloc (sizeof (float *) * numeros[1]);
    for (int i = 0; i < numeros[1]; i++)
        lp_solve -> restricoes[i] = malloc (sizeof (float) * (numeros[0] + 1));

    lp_solve = modela_problema (lp_solve, numeros);

    free (lp_solve -> objetivo);
    free (lp_solve -> limites_inf);
    for (int i = 0; i < numeros[1]; i++)
        free (lp_solve -> restricoes[i]);
    free (lp_solve -> restricoes);
    free (lp_solve);
    free (numeros);

    return 0;
}
