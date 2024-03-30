// GRR20211798 Rafael Gonçalves dos Santos

#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define DEBUG

task_t mainTask, *currentTask;

void ppos_init () {
    currentTask = &mainTask;

    // desativa o buffer da saida padrao (stdout), usado pela função printf 
    setvbuf (stdout, 0, _IONBF, 0);

    
}

int task_init (task_t *task, void (*start_func)(void *), void *arg) {
    getcontext (&task -> context);
    
    char *stack = malloc (STACKSIZE);

    if (stack) {
        task -> context.uc_stack.ss_sp = stack;
        task -> context.uc_stack.ss_size = STACKSIZE;
        task -> context.uc_stack.ss_flags = 0;
        task -> context.uc_link = 0;
    }

    else {
        perror ("Erro na criação da pilha: ");
        exit (1) ;
    }

    makecontext (&task -> context, (void*)(*start_func), 1, arg);

    return 0;
}

void task_exit (int exit_code) {
    task_switch (&mainTask);
}

int task_switch (task_t *task) {
    task_t *aux = currentTask;
    currentTask = task;

    swapcontext (&aux -> context, &task -> context);

    return 0;
}

