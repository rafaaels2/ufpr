// GRR20211798 Rafael Gonçalves dos Santos

#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

// 0 = Tarefa Finalizada
// 1 = Tarefa Pronta
// 2 = Tarefa Rodando
// 3 = Tarefa Suspensa

// inicializa as tarefas
task_t dispacherTask, mainTask, *currentTask, *readyQueue, *asleepQueue;

// variaveis globais de controle
int id = 0, nTasks = -1, sysTimer = 0, lock = 0;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização do timer
struct itimerval timer;

void print_elem (void *ptr) {
   task_t *elem = ptr ;

   if (!elem)
      return ;

   printf ("%d", elem->id) ;
}

void awake () {
    int size;
    task_t *task, *aux;

    size = queue_size ((queue_t*) asleepQueue);
    
    task = asleepQueue;

    // verifica as tarefas que podem ser acordadas
    for (int i = 0; i < size; i++) {
        aux = task -> next;

        // verifica se a tarefa pode ser acordada
        if (task -> awakeTime <= sysTimer) 
            task_awake (task, (task_t**) &asleepQueue);

        // verifica a proxima tarefa da fila
        task = aux;
    }
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
    int size;
    task_t *proxima;

    size = queue_size ((queue_t*) asleepQueue);    

    while (nTasks > 0 || (size > 0)) {
        #ifdef DEBUG
        queue_print ("Prontas: ", (queue_t*) readyQueue, print_elem);
        #endif

        // pega a proxima tarefa a ser executada
        proxima = scheduler ();

        if (proxima != NULL) {
            /* queue_print ("# PRONTAS: ", (queue_t*) readyQueue, print_elem); */
            // remove a task atual da fila
            queue_remove ((queue_t**) &readyQueue, (queue_t*) proxima);
            nTasks--;
            /* printf ("# %d x PRONTAS\n", proxima -> id);
            queue_print ("# PRONTAS: ", (queue_t*) readyQueue, print_elem); */
            
            // define o quantum para a tarefa atual
            proxima -> quantum = 20;

            // dispacher sai da execução
            dispacherTask.status = 1;

            // troca o contxeto para a proxima tarefa
            task_switch (proxima);
        }

        // verifica se possuem tarefas a serem acordadas
        awake ();

        // recalcula o tamanho da fila
        size = queue_size ((queue_t*) asleepQueue);
    }   

    task_exit (0);
}

void tratador (int signum) {
    #ifdef DEBUG
        printf ("# QUANTUM: %d\n", currentTask -> quantum);
    #endif

    // decrementa o quantum
    if (!currentTask -> sysProcess && currentTask -> status == 2) 
        currentTask -> quantum--;
    
    // caso o quantum do processo tenha acabado
    if (currentTask -> quantum == 0) 
        task_yield ();

    // incrementa 1ms no timer
    sysTimer++;
}

void ppos_init () {
    // registra a ação para o sinal de timer SIGALRM (sinal do timer)
    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ") ;
        exit (1) ;
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000 ;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0 ;         // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000 ;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;      // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Erro em setitimer: ") ;
        exit (1) ;
    }

    // inicializa a mainTask
    mainTask.prev = NULL;
    mainTask.prev = NULL;
    mainTask.id = id;
    mainTask.status = 2;
    mainTask.prioEstatica = 0;
    mainTask.prioDinamica = 0;
    mainTask.sysProcess = 0;
    mainTask.quantum = 20;
    mainTask.initTime = sysTimer;
    mainTask.nActivations = 1;
    mainTask.processTime = 0;
    mainTask.initProcessTime = mainTask.initTime;
    mainTask.suspQueue = NULL;
    mainTask.awakeTime = 0;

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
    task -> sysProcess = 1;
    task -> quantum = -1;
    task -> nActivations = 0;
    task -> processTime = 0;
    task -> initTime = sysTimer;
    task -> suspQueue = NULL;
    task -> awakeTime = 0;

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

    // adiciona o contador do numero de tarefas
    nTasks++;

    // tratamento de a tarefa não for o DISPACHER
    if (task -> id != 1) {
        // atualiza o status para uma tarefa que não é do sistema
        task -> sysProcess = 0;

        // coloca a tarefa na fila de prontas
        queue_append ((queue_t**) &readyQueue, (queue_t*) task);
    }

    #ifdef DEBUG
    printf ("# task %d criada por %d\n", id, currentTask -> id) ;
    #endif

    return id;
}

void task_exit (int exit_code) {
    #ifdef DEBUG
        printf ("# tarefa %d terminou\n", currentTask -> id);
    #endif

    // tempo que a tarefa se encerrou
    currentTask -> exitTime = sysTimer;

    // altera o status para FINALIZADA
    currentTask -> status = 0;

    printf ("Task %d exit: execution time %4d ms, processor time %4d ms, %d activations\n", 
    currentTask -> id, (currentTask -> exitTime - currentTask -> initTime), currentTask -> processTime, currentTask -> nActivations);

    while (currentTask -> suspQueue != NULL) {
        // passa o primeiro elemento da fila e a fila
        task_awake ((task_t*) currentTask -> suspQueue, (task_t**) &currentTask -> suspQueue);
    }

    /* queue_print ("Suspensas: ", (queue_t*) asleepQueue, print_elem); */

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

    // tempo que se encerrou o uso do processador
    aux -> suspProcessTime = sysTimer;
    aux -> processTime = aux -> processTime + (aux -> suspProcessTime - aux -> initProcessTime);

    // altera o status da tarefa atual para RODANDO
    task -> status = 2;

    // incrementa o numero de ativações
    task -> nActivations++;
    task -> initProcessTime = sysTimer;
 
    // realiza a troca de contexto
    if (swapcontext (&aux -> context, &task -> context) != 0) {
        perror ("Erro no swapcontext: ");
        return -1;
    }

    return 0;
}

void enter_cs (int *lock) {
    while (__sync_fetch_and_or (lock, 1));
}

void leave_cs (int *lock) {
    (*lock) = 0;
}

void task_suspend (task_t **queue) {
    // tarefa suspensa
    currentTask -> status = 3;

    /* queue_print ("# SUSPENSAS: ", (queue_t*) asleepQueue, print_elem); */

    // coloca a tarefa atual na fila de suspensas pela Task
    queue_append ((queue_t**) queue, (queue_t*) currentTask);

    /* printf ("# %d -> SUSPENSAS\n", currentTask -> id); */
    /* queue_print ("# SUSPENSAS: ", (queue_t*) asleepQueue, print_elem);
    queue_print ("$ SUSPENSAS: ", (queue_t*) *queue, print_elem); */

    leave_cs (&lock);

    // retorna pro dispacher
    task_switch (&dispacherTask);
    
}

void task_awake (task_t *task, task_t **queue) {
    /* queue_print ("FILA: ", (queue_t*) *queue, print_elem);
    printf ("ID: %d\n", task -> id); */
    /* queue_print ("# SUSPENSAS: ", (queue_t*) asleepQueue, print_elem); */

    // remove a tarefa da fila de suspensas
    queue_remove ((queue_t**) queue, (queue_t*) task);

    /* printf ("# %d x SUSPENSAS\n", task -> id);
    queue_print ("# SUSPENSAS: ", (queue_t*) asleepQueue, print_elem); */
    // altera o status para pronta
    task -> status = 1;

    /* queue_print ("# PRONTAS: ", (queue_t*) readyQueue, print_elem); */
    // adiciona na fila de prontas
    queue_append ((queue_t**) &readyQueue, (queue_t*) task);

    /* printf ("# %d -> PRONTAS\n", task -> id);
    queue_print ("# PRONTAS: ", (queue_t*) readyQueue, print_elem); */
    nTasks++;
}

void task_yield () {
    #ifdef DEBUG
    printf ("# task %d chamou a CPU\n", currentTask -> id) ;
    #endif

    // altera o status da tarefa atual para PRONTA
    currentTask -> status = 1;

    // adiciona a tarefa atual na fila
    queue_append ((queue_t**) &readyQueue, (queue_t*) currentTask);
    nTasks++;

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

unsigned int systime () {
    return sysTimer;
}

void task_sleep (int t) {
    // calcula o tempo para acordar a tarefa
    currentTask -> awakeTime = sysTimer + t;

    // coloca a tarefa atual na fila de de dormindo
    task_suspend (&asleepQueue);
}

int task_wait (task_t *task) {
    if (task == NULL || task -> status == 0)
        return -1;

    task_suspend ((task_t **) &task -> suspQueue);

    return task -> id;
}

int sem_init (semaphore_t *s, int value) {
    if (s == NULL)
        return -1;
    
    // inicializa as variaveis do semaforo
    s -> cont = value;
    s -> tasksQueue = NULL;

    return 0;
}

int sem_down (semaphore_t *s) {
    enter_cs (&lock);

    if (s == NULL) 
        return -1;

    s -> cont--;

    // coloca a tarefa na fila do semaforo
    // suspende a tarefa
    if (s -> cont < 0) 
        task_suspend ((task_t **) &s -> tasksQueue);
    
    leave_cs (&lock);

    return 0;
}

int sem_up (semaphore_t *s) {
    enter_cs (&lock);

    if (s == NULL) 
        return -1;

    s -> cont++;

    // coloca a tarefa na fila de prontas
    if (s -> cont <= 0)
        task_awake ((task_t*) s -> tasksQueue, (task_t**) &s -> tasksQueue);
    
    leave_cs (&lock);

    return 0;
}

int sem_destroy (semaphore_t *s) {
    enter_cs (&lock);

    // acorda todas as tarefas
    while (currentTask -> suspQueue != NULL) {
        // passa o primeiro elemento da fila e a fila
        task_awake ((task_t*) s -> tasksQueue, (task_t**) &s -> tasksQueue);
    }

    // destroi o semaforo
    s = NULL;

    leave_cs (&lock);

    return 0;
}