// GRR20211798 Rafael Gonçalves dos Santos

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

semaphore_t s_buffer, s_item, s_vaga;
queue_t *buffer;
task_t p1, p2, p3, c1, c2;
long item;

void produtor (void *arg) {
    while (1) {
        task_sleep (1000);

        item = random () % 99;

        sem_down (&s_vaga);
        sem_down (&s_buffer);
        printf ("%s produziu %ld\n", (char *) arg, item);
        queue_append ((queue_t**) &buffer, (queue_t*) item);
        sem_up (&s_buffer);
        sem_up (&s_item);
    }
}

void consumidor (void *arg) {
    while (1) {
        sem_down (&s_item);
        sem_down (&s_buffer);
        printf ("%s consumiu %ld\n", (char *) arg, (long) buffer);
        queue_remove ((queue_t**) &buffer, (queue_t*) buffer);
        sem_up (&s_buffer);
        sem_up (&s_vaga);

        task_sleep (1000);
    }
}

int main (int argc, char *argv[]) {
    ppos_init ();

    sem_init (&s_buffer, 1);
    sem_init (&s_item, 0);
    sem_init (&s_vaga, 5);

    task_init (&p1, produtor, "p1");
    task_init (&p2, produtor, "p2");
    task_init (&p3, produtor, "p3");
    task_init (&c1, consumidor, "\t\tc1");
    task_init (&c2, consumidor, "\t\tc2");

    task_exit (0);
}