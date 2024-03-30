// GRR20211798 Rafael Gonçalves dos Santos

#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t mainTask, *currentTask;
int id = 0;

void ppos_init () {
    // id da tarefa main
    mainTask.id = 0;

    // contexto atual é o contextoMain
    currentTask = &mainTask;

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
    // define o id da tarefa
    task -> id = ++id;

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

    #ifdef DEBUG
    printf ("# task %d criada por %d\n", id, currentTask -> id) ;
    #endif

    return id;
}

void task_exit (int exit_code) {
    // retorna para a main
    task_switch (&mainTask);
}

int task_switch (task_t *task) {
    // atualiza o contexto atual
    task_t *aux = currentTask;
    currentTask = task;

    #ifdef DEBUG
    printf ("# troca de task %d -> %d\n", aux -> id, currentTask -> id) ;
    #endif

    // realiza a troca de contexto
    if (swapcontext (&aux -> context, &task -> context) != 0) {
        perror ("Erro no swapcontext: ");
        return -1;
    }

    return 0;
}

