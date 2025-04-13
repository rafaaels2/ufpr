#include "lp.h"

int le_entrada (char *entrada, float *num) {
    int n = 0;
    
    /* verifica se ha o numero suficeinte de entradas  */
    while (n < 2 || n < ((2) + (num[0]) + (2 * num[1]) + (num[0] * num[1]))) {
        /* le a entrada dada no stdin e define seu final */
        if (fgets (entrada, MAX_TAM, stdin) == NULL) {}
        entrada[strcspn (entrada, "\n")] = '\0';

        char *aux = entrada;
        
        /* percorre toda string *entrada e atribui os valores ao vetor *num */
        while (*aux) {
            if (isdigit (*aux)) {
                sscanf (aux, "%f", &num[n++]);

                while (isdigit (*aux) || (strncmp (aux, ".", 1) == 0)) aux++;
            }
            else {
                aux++;
            }
        }
    }

    return n;
}

void libera_mem (char *entr, float *entr_f, float *num, lp_solve_t *lp_solve) {
    free (entr);
    free (entr_f);
    free (lp_solve -> objetivo);
    free (lp_solve -> limites_inf);
    for (int i = 0; i < num[1]; i++)
        free (lp_solve -> restricoes[i]);
    free (lp_solve -> restricoes);
    free (lp_solve);
    free (num);
}

void copia_vetor (float *v1, float *v2, int tam) {
    for (int i = 0; i < tam; i++)
        v2[i] = v1[i];
}

lp_solve_t *lp_init (lp_solve_t *lp_solve, int n_pro, int n_comp) {
    lp_solve -> objetivo    = malloc (sizeof (float) * n_pro);
    lp_solve -> limites_inf = malloc (sizeof (float) * n_pro);
    lp_solve -> restricoes  = malloc (sizeof (float *) * n_comp);
    for (int i = 0; i < n_comp; i++)
        lp_solve -> restricoes[i] = malloc (sizeof (float) * (n_pro + 1));
}

lp_solve_t *modela_problema (lp_solve_t *lp_solve, float *num) {
    int xi = 0;
    int li = 0;
    int ri = 0;

    /* OBJETIVO ------------------------------------------------------------- */

    /* calcula o lucro por litro de cada produto */
    for (int i = 0; i < num[0]; i++) {
        float valor = 0;
        float custo = 0;
        
        /* pega o valor do produto */
        valor = num[i + 2];

        /* calcula o custo dos componentes para o produto */
        for (int j = 0; j < num[1]; j++) 
            custo += 
            /* custo do componente */
            (num[(j * 2) + 2 + (int) num[0]]) * 
            /* quantida do componente */
            (num[j + (i * (int) num[1]) + 2 + (int) num[0] + 2 * (int) num[1]]);

        lp_solve -> objetivo[xi++] = valor - custo;
    }

    /* imprime a função objetivo */
    printf ("max:");
    for (int i = 0; i < num[0] - 1; i++)
        printf (" %.2fx%d +", lp_solve -> objetivo[i], i + 1);
    printf (" %.2fx%d", lp_solve -> objetivo[xi - 1], xi);
    printf (";\n");

    /* OBJETIVO ------------------------------------------------------------- */

    /* LIMITES INFERIORES --------------------------------------------------- */
    
    /* atribuit os limites inferiores (0) e imprime */
    for (int i = 0; i < num[0]; i++) {
        lp_solve -> limites_inf[li++] = 0;
        printf ("x%d >= %.2f;\n", i + 1, lp_solve -> limites_inf[i]);
    }   

    /* LIMITES INFERIORES --------------------------------------------------- */

    /* RESTRICOES ----------------------------------------------------------- */

    /* calcula as restricoes e as quantidade de uso de cada componente */
    for (int i = 0; i < num[1]; i++) {
        ri = 0;

        /* pega a quantidade do componente para cada um dos produtos */
        for (int j = 0; j < num[0]; j++) {
            lp_solve -> restricoes[i][ri++] = 
            num[2 + (int) num[0] + (2 * (int) num[1]) + (j * (int) num[1]) + (i * 1)];
        }
        /* pega o limite superior de uso do componente */
        lp_solve -> restricoes[i][ri] = num[3 + (int) num[0] + (i * 2)];
    }

    /* imprime as inequcoes dos limites de uso dos componentes */
    for (int i = 0; i < num[1]; i++) {
        for (int j = 0; j < num[0] - 1; j++) {
            printf ("%.2fx%d + ", lp_solve -> restricoes[i][j], j + 1);
        }
        printf ("%.2fx%d", lp_solve -> restricoes[i][ri - 1], ri);
        printf (" <= %.2f;\n", lp_solve -> restricoes[i][ri]);
    }

    /* RESTRICOES ----------------------------------------------------------- */

    return lp_solve;
}