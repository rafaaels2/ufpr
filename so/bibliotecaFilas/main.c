// Rafael Gonçalves dos Santos - 20211798

// SAIDAS DE ERRO
// 1 - erro na inserção

#include <stdio.h>
#include "queue.h"

#define N 4

typedef struct filaint_t {
   struct filaint_t *prev;  // ptr para usar cast com queue_t
   struct filaint_t *next;  // ptr para usar cast com queue_t
   int id;
} filaint_t;

filaint_t item[N];
filaint_t *fila, *aux;

// imprime um elmento da fila
void print_elem (void *ptr) {
    filaint_t *elem = ptr;

    if (!elem) {
        return;
    }

    elem->prev ? printf ("%d", elem->prev->id) : printf ("*");
    printf ("<%d>", elem->id);
    elem->next ? printf ("%d", elem->next->id) : printf ("*");
}

int main (int argc, char **argv) {
    // inicializa a fila
    fila = NULL;

    // inicializa os itens da fila 
    for (int i = 0; i < N; i++) {
        item[i].id   = i;
        item[i].prev = NULL;
        item[i].next = NULL;
    }

    for (int i = 0; i < N; i++) {
        if (queue_append ((queue_t **) &fila, (queue_t*) &item[i]) != 0) {
            return 1;
        }
    }

    queue_remove ((queue_t **) &fila, (queue_t*) &item[0]);

    queue_print ("Saida gerada: ", (queue_t*)fila, print_elem);

    queue_remove ((queue_t **) &fila, (queue_t*) &item[0]);

    queue_print ("Saida gerada: ", (queue_t*)fila, print_elem);

    return 0;
}