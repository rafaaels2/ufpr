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

        return inicioHeap;
    }
    
    atualHeap = inicioHeap;

    /* aloca em espaço vazio */
    while (atualHeap != topoHeap) {
        if (*((long int*) (atualHeap)) != 1) {
            if (*((long int*) (atualHeap + 8)) >= num_bytes) {
                /* coloca o valor de ocupado */
                ocupado = atualHeap;
                *ocupado = 1;
                
                /* coloco o endereço do início do bloco */
                /* SERA Q ISSO EH NECESSARIO */
                // bloco = atualHeap + 16;
                
                /* dividir um bloco em 2 */
                dif = *((long int*) (atualHeap + 8)) - num_bytes;
                if (dif > 16) {
                    /* atualiza tamanho do bloco */
                    tamanho = atualHeap + 8;
                    *tamanho = num_bytes;

                    /* novo bloco desocupado */
                    /* + 16 (informações gerenciais do bloco anterior) + num_bytes (espaço do bloco anterior) */
                    ocupado = atualHeap + 16 + num_bytes;
                    *ocupado = 0;

                    /* tamanho do novo bloco */
                    /* + 24 (informações gerenciais do bloco anterior + ocupado do bloco atual) + num_bytes (espaço do bloco anterior) */
                    tamanho = atualHeap + 24 + num_bytes;
                    *tamanho = dif - 16;

                    /* coloco o endereço do início do bloco */
                    /* SERA Q ISSO EH NECESSARIO */
                    // bloco = atualHeap + 16;
                } 

                return atualHeap;
            }
        }

        atualHeap = atualHeap + 16 + *((long int*) (atualHeap + 8));
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

int *liberaMem (void *bloco) {
    long int *ocupado;

    /* indica que o bloco está livre */
    ocupado = bloco;
    *ocupado = 0;
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

    printf ("Aloca bloco TAM 50\n");
    a = alocaMem (50);
    printf ("# INICIO BLOCO A: %p\n", a);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    imprimeMapa ();
    printf ("\n");
    
    printf ("Aloca bloco TAM 5\n");
    b = alocaMem (5);
    printf ("# INICIO BLOCO B: %p\n", b);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    imprimeMapa ();
    printf ("\n");

    printf ("Libera bloco TAM A\n");
    liberaMem (a);
    imprimeMapa ();
    printf ("\n");

    printf ("Aloca bloco TAM 10\n");
    c = alocaMem (10);
    printf ("# INICIO BLOCO C: %p\n", c);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    imprimeMapa ();
    printf ("\n");
    
    printf ("Aloca bloco TAM 8\n");
    d = alocaMem (8);
    printf ("# INICIO BLOCO D: %p\n", d);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    imprimeMapa ();
    printf ("\n");

    printf ("Libera bloco TAM D\n");
    liberaMem (d);
    imprimeMapa ();
    printf ("\n");

    printf ("Aloca bloco TAM 25\n");
    e = alocaMem (25);
    printf ("# INICIO BLOCO D: %p\n", e);
    printf ("# TOPO DA HEAP:   %p\n", topoHeap);
    imprimeMapa ();
    printf ("\n");

    finalizaAlocador ();
    
    return 0;
}