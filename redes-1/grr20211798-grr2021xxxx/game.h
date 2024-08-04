#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef FRAME_H
#define FRAME_H

#define MAX_CARDS 10
#define BUFFER 1024
#define N 4
#define IP_ADRESS "127.0.0.1"

/* representa as informacoes da carta */
typedef struct card_t {                                     
    int value;    /* indice do vetor que inidica o valor */
    int suit;     /* indice do vetor que indica o naipe */
} card_t;

/* representa as informacoes da aposta */
typedef struct bet_t {
    int n_bets;     /* numero de apostas feitas na rodada */
    int starter;    /* addr do jogador "pe" */
    int bets[N];    /* vetor que recolhe as apostas */
} bet_t;

/* representa as informacoes da rodada */
typedef struct round_t {
    int n_plays;        /* numero de jogadas feitas no lance */
    int starter;        /* addr do jogador "pe" */
    int n_round;        /* numero do lance na rodada */
    card_t cards[N];    /* vetor que recolhe as cartas */
} round_t;

/* representa as informacoes crucias do jogo */
typedef struct info_t {
    int dealer;       /* addr do carteador */
    int starter;      /* addr do jogador "pe" */
    int n_cards;      /* numero de cartas iniciais da rodada */
    int crr_round;    /* numero da rodada atual */
    int op;           /* flag para definir a operacao */
    int lifes[N];     /* vetor das vidas dos jogadores */
    int bets[N];      /* vetor das apostas dos jogadores */
    int wins[N];      /* vetor das vitorias em laces dos jogadores */
} info_t;

/* union com os possiveis dados do frame */
typedef union data_t {
    char msg[BUFFER];    /* mensagem a ser transmitida */
    card_t card;
    bet_t bet;
    round_t round;
    info_t info;
} data_t;

/* tipos de mensagem do frame */
typedef enum type_t {
    MSG,
    BETS,
    VIRA,
    CARD,
    ROUND,
    END,
    NOT,
} type_t;

/* representa as informacoes no frame */
typedef struct frame_t {
    int status;     /* verificar se a msg concluiu seu ciclo corretamente */
    int src;        /* addr de quem envia a mensagem */
    int addr;       /* addr atual do frame */
    int dest[N];    /* vetor dos addr destinos */
    int recv[N];     /* vetor do controle de recebimento */
    type_t type;
    data_t data;
} frame_t ;

/*
 * @brief pegar o indice do vetor das portas
 *
 * @param addr, id do jogador 
 * @param type, se vai receber ou enviar
 * 
 * @return > -1, indice do vetor
 * @return -1, porta invalida
 */
int get_port (int addr, int type);

/* 
 * @brief atribuir os destinos da mensagem
 *
 * @param *dest, vetor dos destinos
 * @addr, id do jogador
 */
void get_dest (int *dest, int addr);

/*
 * @brief verifica se o jogador eh destino da mensagem
 * 
 * @param *dest, vetor de destinos
 * @param addr, id do jogador
 * 
 * @return 1, jogador eh destino
 * @return 0, jogador nao eh destino
 */
int verify_dest (int *dest, int addr);

/*
 * @brief recebe e trata a mensagem
 * 
 * @param *frame, mensagem 
 * @param addr, id do jogador
 * 
 * @return 1, recebimento deu certo
 */
int recv_msg (frame_t *frame, info_t *info, int addr);

/*
 * @brief envia a mensagem
 * 
 * @param *frame, mensagem
 * @param addr, id do jogador
 * 
 * @return 1, envio deu certo
 */
int send_msg (frame_t *frame, int addr);

/*
 * @brief inicializa o frame
 * 
 * @param addr, id do jogador
 * 
 * @return frame inicializado
 */
frame_t *frame_init (int addr);

/*
 * @brief pegar a impressao da carta
 * 
 * @param card, carta a ser impressa
 * 
 * @return impressao da carta
 */
char *card_print (card_t card);

/*
 * @brief verificar se a carta sorteada ja foi usada
 * 
 * @param *used, vetor de cartas em uso
 * @param card, carta sorteada
 * 
 * @return 0, carta esta sendo utilizada
 * @return 1, carta disponivel
 */
int verify_card (card_t *used, card_t card);

/*
 * @brief sorteia uma nova carta
 * 
 * @param *used, vetor de cartas em uso
 * 
 * @retunr carta sorteada
 */
card_t get_card (card_t *used);

/*
 * @brief compara duas cartas
 * 
 * @param *card_1, maior carta
 * @param card_2, carta a ser comparada
 * @param manilha da rodada
 * 
 * @return 1, card_2 eh maior
 * @return 0, card_1 eh maior
 */
int cmp_card (card_t *card_1, card_t card_2, int shackle);

/*
 * @brief sorteia e distruibui as cartas para os jogadores
 *
 * @param *frame, mensagem
 * @param *info, informacoes do jogo
 * @param *used, vetor de cartas utilizadas
 * @param addr, id do jogador
 * 
 * @return vetor das cartas do carteados
 */
void draw_cards (frame_t *frame, info_t *info, card_t *used, int addr);

/*
 * @brief sorteia e distribui a carta vira
 *
 * @param *frame, mensagem
 * @param *used, vetor de cartas utilizadas
 * @param *shackle, ponteiro para a manilha
 * @param  addr, id do jogador
 * 
 */
void draw_vira (frame_t *frame, info_t *info, card_t *used, int *shackle, int addr);

/*
 * @brief recolhe as apostas dos jogadores
 * 
 * @param *frame, mensagem
 * @param *info, informacoes do jogo
 * @param addr, id do joagdor
 * 
 */
void get_bets (frame_t *frame, info_t *info, int addr);

/*
 * @brief informa algo aos jogadores
 * 
 * @param *frame, mensagem
 * @addr, id do jogador
 */
void inform (frame_t *frame, info_t *info, int addr);

/*
 * @brief realiza os lances da rodada
 *
 * @param *frame, mensagem
 * @param *info, informacoes do jogo
 * @param shackle, manilha do lance
 * @param addr, id do jogaodr
 */
void run_game (frame_t *frame, info_t *info, int shackle, int addr);

/*
 * @brief encerra a rodada 
 * 
 * @param *frame, mensagem
 * @param *info, informacoes do jogo
 * @param id, do jogador
 */
void end_round (frame_t *frame, info_t *info, int addr);

/*
 * @brief verifica se algum jogador perdeu as vidas
 *
 * @param info, infomracoes do jogo
 * 
 * @return 1, se algum jogador perdeu as vidas
 * @return 0, se nenhum jogador perdeu as vidas
 */
int no_lives (info_t *info);

/*
 * @brief inicializa as informacoes do jogo
 * 
 * @return info inicializado
 */
info_t *info_init ();

#endif