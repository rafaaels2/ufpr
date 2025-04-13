#include "comb.h"

espec_t defSpec (espec_t espec, int sinal ,int composto, int estado) {
    espec.sinal = sinal;
    espec.composto = composto;
    espec.estado = estado;

    return espec;
}

int gerar_combinacoes (int *viz, int n, int tam, FILE *arq, espec_t espec, int *num) {
    int *comb = malloc (sizeof (int) * tam);

    /* verifica se o numero de vizinhos eh menor que as n celulas da combinacao */
    if (n < tam) 
        tam = n; 

    arq = fopen ("clausulas.txt", "a");

    combinar (viz, n, comb, 0, 0, tam, arq, espec, num);

    fclose (arq);
    free (comb);

    return *num;
}

void combinar (int *viz, int n, int *comb, int ini, int prof, int tam, FILE *arq, espec_t espec, int *num) {
    int vizinho_vivo;

    /* define uma combinacao */
    if (prof == tam) {
        /* aumenta o numero de clausulas */
        *num = *num + 1;    

        /* adiciona a clausula no arquivo */
        for (int i = 0; i < tam; i++) 
            fprintf (arq, "%d ", comb[i]);

        /* elementos fora da combinacao */
        if (espec.composto) {
            for (int i = 0; i < n; i++) {
                vizinho_vivo = 0;

                for (int j = 0; j < tam; j++) {
                    if (comb[j] == viz[i] * espec.sinal) {
                        vizinho_vivo = 1;
                        break;
                    }
                }

                if (!vizinho_vivo) 
                    fprintf (arq, "%d ", viz[i] * -espec.sinal); 
            }
        }

        /* elemento x */
        if (espec.estado != 0) 
            fprintf (arq, "%d ", espec.estado);

        fprintf (arq, "0\n");

        return;
    }

    /* gera as combincaoes */
    for (int i = ini; i < n; i++) {
        comb[prof] = viz[i] * espec.sinal;  
        combinar (viz, n, comb, i + 1, prof + 1, tam, arq, espec, num);  
    }
}