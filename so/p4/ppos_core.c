// GRR20211798 Rafael Gonçalves dos Santos

#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

// 0 = Tarefa Finalizada
// 1 = Tarefa Pronta
// 2 = Tarefa Rodando
// 3 = Tarefa Suspensa

task_t dispacherTask, mainTask, *currentTask, *readyQueue;
int id = 0, nTasks = -1;

void print_elem (void *ptr) {
   task_t *elem = ptr ;

   if (!elem)
      return ;

   printf ("%d", elem->id) ;
}

task_t *scheduler () {
    int size;
    task_t *aux, *maiorPrio;

    size = queue_size ((queue_t*) readyQueue);

    // se a fila for vazia retorna nulo
    if (size == 0)
        return NULL;
    
    maiorPrio = readyQueue;
    aux = readyQueue -> next;

    // encontra a tarefa com maior prioridade dinamica
    for (int i = 1; i < size; i++) {
        if (aux -> prioDinamica < maiorPrio -> prioDinamica) {
            maiorPrio -> prioDinamica = maiorPrio -> prioDinamica - 1;
            maiorPrio = aux;
        }

        else {
            // verifica se a atrefa ja tem pioridade maxima
            if (aux -> prioDinamica != -20) {
                aux -> prioDinamica = aux -> prioDinamica - 1;
            }
        }

        aux = aux -> next;
    }

    // regenera a tarefa escolhida para a prioridade original
    maiorPrio -> prioDinamica = maiorPrio -> prioEstatica;

    return maiorPrio;
}

void dispacher (void * arg) {
    task_t *proxima;

    while (nTasks > 0) {
        #ifdef DEBUG
        queue_print ("Prontas: ", (queue_t*) readyQueue, print_elem);
        #endif

        // pega a proxima tarefa a ser executada
        proxima = scheduler ();

        if (proxima != NULL) {
            // remove a task atual da fila
            queue_remove ((queue_t**) &readyQueue, (queue_t*) proxima);
            nTasks--;

            // troca o contxeto para a proxima tarefa
            task_switch (proxima);

            // // altera o status da tarefa atual para SUSPENSA
            proxima -> status = 3;
        }
    }   

    exit (0);
}

void ppos_init () {
    /* DÚVIDAS EM RELAÇÃO A INICIALIZAÇÃO DA MAIN */

    // inicializa a mainTask
    mainTask.prev = NULL;
    mainTask.prev = NULL;
    mainTask.id = id;
    mainTask.status = 2;

    // atribui a mainTask para a tarefa atual
    currentTask = &mainTask;

    // inicializa a fila de prontas
    readyQueue = NULL;

    // inicializa o dispacher
    task_init (&dispacherTask, dispacher, "Dispacher");
    
    // desativa o buffer da saida padrao (stdout), usado pela função printf 
    setvbuf (stdout, 0, _IONBF, 0);

    #ifdef DEBUG
    printf ("# sistema inicializado\n") ;
    #endif
}

int task_id () {
    return currentTask -> id;
}

int task_init (task_t *task, void (*start_func)(void *), void *arg) {
    // inicializa a tarefa
    task -> prev = NULL;
    task -> next = NULL;
    task -> id = ++id;
    task -> status = 1;
    task -> prioEstatica = 0;
    task -> prioDinamica = 0;

    // salva o contexto atual na task iniciada
    if (getcontext (&task -> context) != 0) {
        perror ("Erro no getcontext: ");
        return -1;
    }
    
    // aloca memoria para o contexto
    char *stack = malloc (STACKSIZE);

    if (stack) {
        // inicializa o contexto
        task -> context.uc_stack.ss_sp = stack;
        task -> context.uc_stack.ss_size = STACKSIZE;
        task -> context.uc_stack.ss_flags = 0;
        task -> context.uc_link = 0;
    }

    else {
        perror ("Erro na criação da pilha: ");
        return -1;
    }

    // passa a funcao para a funcao e seus respectivos parametros para a task
    makecontext (&task -> context, (void*)(*start_func), 1, arg);

    // adiciona a tarefa nova na fila de prontas
    nTasks++;
    if (task -> id != 1) 
        queue_append ((queue_t**) &readyQueue, (queue_t*) task);

    #ifdef DEBUG
    printf ("# task %d criada por %d\n", id, currentTask -> id) ;
    #endif

    return id;
}

void task_exit (int exit_code) {
    // altera o status da tarefa atual para FINALIZADA
    currentTask -> status = 0;

    #ifdef DEBUG
    printf ("# tarefa %d terminou\n", currentTask -> id);
    #endif

    // retorna para o dispacher
    task_switch (&dispacherTask);
}

int task_switch (task_t *task) {
    // atualiza o contexto atual
    task_t *aux = currentTask;
    currentTask = task;

    #ifdef DEBUG
    printf ("# troca de task %d -> %d\n", aux -> id, currentTask -> id) ;
    #endif

    // altera o status da tarefa atual para RODANDO
    currentTask -> status = 2;

    // realiza a troca de contexto
    if (swapcontext (&aux -> context, &task -> context) != 0) {
        perror ("Erro no swapcontext: ");
        return -1;
    }

    return 0;
}

void task_yield () {
    #ifdef DEBUG
    printf ("# task %d chamou a CPU\n", currentTask -> id) ;
    #endif

    // altera o status da tarefa atual para PRONTA
    currentTask -> status = 1;

    // adiciona a tarefa atual na fila
    if (currentTask -> id != 0) {
        queue_append ((queue_t**) &readyQueue, (queue_t*) currentTask);
        nTasks++;
    }

    // volta para o dispacher
    task_switch (&dispacherTask);
}

void task_setprio (task_t *task, int prio) {
    // verifica se prio está entre o intervalo correto
    if (prio < -20)
        prio = -20;

    else if (prio > 20)
        prio = 20;

    // define a prioridade da tarefa atual
    if (task == NULL) {
        currentTask -> prioEstatica = prio;
        currentTask -> prioDinamica = prio;
    }

    // define a prioridade da tarefa passada como parametro
    else {
        task -> prioEstatica = prio;
        task -> prioDinamica = prio;
    }
}

int task_getprio (task_t *task) {
    if (task == NULL)
        return currentTask -> prioEstatica;

    return task -> prioEstatica;
}