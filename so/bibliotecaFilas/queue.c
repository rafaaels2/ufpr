// Rafael Gonçalves dos Santos - 20211798

#include <stdio.h>
#include "queue.h"

int queue_size (queue_t *queue) {
    queue_t *aux;
    int tam;

    // fila vazia
    if (queue == NULL) {
        return 0;
    }
    
    aux = queue;
    tam = 1;

    // percorre toda fila até chegar no elemento que possui o next sendo o inicio da fila (ultimo elemento)
    while (aux->next != queue) {
        aux = aux->next;
        tam++;
    }

    return tam;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*)) {
    queue_t *aux = queue;

    printf ("%s", name);
    printf ("[");

    // imprime o primeiro elemento
    print_elem (queue);

    // percorre toda fila até chegar no elemento que possui o next sendo o inicio da fila (ultimo elemento)
    while (aux->next != queue) {
        printf (" ");

        aux = aux->next;

        print_elem (aux);
    }

    printf ("]");
    printf ("\n");
}

int queue_append (queue_t **queue, queue_t *elem) {
    queue_t *aux;
    
    // -------------------------------------------------------- //

    // fila inexistente
    if (queue == NULL) {
        fprintf (stderr, "Fila não existe\n");
        return 1;
    }

    // elemento inexistente
    if (elem == NULL) {
        fprintf (stderr, "Elemento não existe\n");
        return 1;
    }

    // elemento pertence a outra fila
    if (elem->prev != NULL || elem->next != NULL) {
        fprintf (stderr, "Elemento pertence a outra fila\n");
        return 1;
    }

    // -------------------------------------------------------- //

    // fila vazia
    if (*queue == NULL) {
        *queue = elem;

        elem->prev = elem;
        elem->next = elem;

        return 0;
    }

    // aux = ultimo elemento da fila
    aux = (*queue)->prev;

    // o prev do primeiro elemento eh o novo elemento
    (*queue)->prev = elem;

    // o next do ultimo elemento eh o novo elemento
    aux->next = elem;

    // o prev do novo elemento eh o ultimo elemento 
    elem->prev = aux;

    // o next do novo elemento eh o primeiro elemento
    elem->next = *queue;

    return 0;
}

int queue_contains (queue_t **queue, queue_t *elem) {
    queue_t *aux;

    // elemento eh o primeiro da fila
    if (*queue == elem) {
        return 1;
    }

    aux = *queue;

    // percorre toda fila até chegar no elemento que possui o next sendo o inicio da fila (ultimo elemento)
    while (aux->next != *queue) {
        if (aux == elem) {
            return 1;
        }

        aux = aux->next;
    }

    return 0;               
}

int queue_remove (queue_t **queue, queue_t *elem) {
    queue_t *aux;

    // -------------------------------------------------------- //

    // fila inexistente
    if (queue == NULL) {
        fprintf (stderr, "Fila não existe\n");
        return 1;
    }

    // elemento inexistente
    if (elem == NULL) {
        fprintf (stderr, "Elemento não existe\n");
        return 1;
    }

    // fila vazia
    if (*queue == NULL) {
        fprintf (stderr, "Fila Vazia\n");
        return 1;
    }

    // elemento não pertence a fila
    if (!queue_contains (queue, elem)) {
        fprintf (stderr, "Elemento não pertence a fila\n");
        return 1;
    }

    // -------------------------------------------------------- //

    // se remover o primeiro altera o inicio da fila
    if (elem == *queue) {
        *queue = elem->next;
    }

    // aux = prev do elemento
    aux = elem->prev;

    // next do prev do elemento eh o next do elemento
    aux->next = elem->next;

    // aux = next do elemento
    aux = elem->next;
    
    // prev do next do elemento eh o prev do elemento
    aux->prev = elem->prev;

    // tira o elemento da fila
    elem->prev = NULL;
    elem->next = NULL;

    return 0;
}