#include <stdio.h>
#include <unistd.h>

void *topoHeap;
void *inicioHeap;

void iniciaAlocador () {
    topoHeap = sbrk(0);
    inicioHeap = topoHeap;
}

void finalizaAlocador () {
    brk (inicioHeap);
} 

void *alocaMem (long int num_bytes) {
    long int *ocupado;
    long int *tamanho;
    long int maiorTamanho = 0;
    void *maiorHeap;
    void *bloco;
    void *atualHeap;
    long int dif;

    /* se lista vazia */
    if (inicioHeap == topoHeap) {
        /* aloca o tamanho do noh */
        brk(inicioHeap + (16 + num_bytes));

        /* coloca o valor de ocupado */
        ocupado = inicioHeap;
        *ocupado = 1;

        /* coloca o valor de tamanho */
        tamanho = inicioHeap + 8;
        *tamanho = num_bytes;
        
        /* coloco o endereço do início do bloco */
        /* SERA Q ISSO EH NECESSARIO */
        // bloco = inicioHeap + 16;

        /* atualiza topo da heap */
        topoHeap = sbrk(0);

        atualHeap = inicioHeap;

        return atualHeap;
    }
    
    atualHeap = inicioHeap;

    /* aloca em espaço vazio */
    while (atualHeap != topoHeap) {
        if (*((long int*) (atualHeap)) != 1) {
            if (*((long int*) (atualHeap + 8)) >= num_bytes) {

                if (*((long int*) (atualHeap + 8)) > maiorTamanho) {
                    maiorTamanho =  *((long int*) (atualHeap + 8));
                    maiorHeap = atualHeap;
                }

                /* coloca o valor de ocupado */
            }
        }

        atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    }

    if (maiorTamanho != 0) {
        ocupado = maiorHeap;
        *ocupado = 1;
        
        /* dividir um bloco em 2 */
        dif = *((long int*) (maiorHeap + 8)) - num_bytes;
        if (dif > 16) {
            /* atualiza tamanho do bloco */
            tamanho = maiorHeap + 8;
            *tamanho = num_bytes;

            /* novo bloco desocupado */
            /* + 16 (informações gerenciais do bloco anterior) + num_bytes (espaço do bloco anterior) */
            ocupado = maiorHeap + 16 + num_bytes;
            *ocupado = 0;

            /* tamanho do novo bloco */
            /* + 24 (informações gerenciais do bloco anterior + ocupado do bloco atual) + num_bytes (espaço do bloco anterior) */
            tamanho = maiorHeap + 24 + num_bytes;
            *tamanho = dif - 16;
        } 

        return maiorHeap;
    }

    /* aloca mais espaço na heap */

    /* aloca o tamanho do noh */
    brk(atualHeap + (16 + num_bytes));

    /* coloca o valor de ocupado */
    ocupado = atualHeap;
    *ocupado = 1;
    
    /* coloca o valor de tamanho */
    tamanho = atualHeap + 8;
    *tamanho = num_bytes;

    /* coloco o endereço do início do bloco */
    /* SERA Q ISSO EH NECESSARIO */
    // bloco = atualHeap + 16;

    /* atualiza topo da heap */
    topoHeap = sbrk(0);

    return atualHeap;
}

void fusaoNos () {
    long int contador;
    long int *tamanho;
    void *atualHeap, *antigaHeap;

    atualHeap = inicioHeap;
    contador = 0;

    while (atualHeap != topoHeap) {
        if (*((long int*) (atualHeap)) == 0) {
            contador = contador + 1;

            if (contador == 2) {
                tamanho = antigaHeap + 8;
                *tamanho = *((long int*) (atualHeap + 8)) + *((long int*) (antigaHeap + 8)) + 16;

                contador = contador - 1;
            }
            else {
                antigaHeap = atualHeap;
            }
        }
        else {
            contador = 0;
        }

        atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    }
}

int *liberaMem (void *bloco) {
    long int *ocupado;

    /* indica que o bloco está livre */
    ocupado = bloco;
    *ocupado = 0;

    fusaoNos ();
}

void imprimeMapa () {
    void *atualHeap;

    atualHeap = inicioHeap;

    while (atualHeap != topoHeap) {
        printf ("################");

        if (*((long int*) (atualHeap)) == 1) {
            for (int i = 0; i < *((long int*) (atualHeap + 8)); i++) {
                printf ("+");
            }
        }

        if (*((long int*) (atualHeap)) == 0) {
            for (int i = 0; i < *((long int*) (atualHeap + 8)); i++) {
                printf ("-");
            }
        }
        
        atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
    }
    printf ("\n");
}

int main () {
    void *a, *b, *c, *d, *e;

    printf ("Manipulando a Heap\n");

    iniciaAlocador ();

    printf ("# INICIO DA HEAP: %p\n", inicioHeap);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    printf ("\n");

    printf ("Aloca bloco TAM 10\n");
    a = alocaMem (60);
    imprimeMapa ();
    printf ("\n");

    printf ("Aloca bloco TAM 10\n");
    b = alocaMem (10);
    imprimeMapa ();
    printf ("\n");

    printf ("Aloca bloco TAM 10\n");
    c = alocaMem (50);
    imprimeMapa ();
    printf ("\n");

    liberaMem (a);
    imprimeMapa ();
    printf ("\n");

    liberaMem (c);
    imprimeMapa ();
    printf ("\n");

    printf ("Aloca bloco TAM 10\n");
    c = alocaMem (10);
    imprimeMapa ();
    printf ("\n");

    liberaMem (c);
    imprimeMapa ();
    printf ("\n");

    liberaMem (b);
    imprimeMapa ();
    printf ("\n");
    
    finalizaAlocador ();
    
    return 0;
}