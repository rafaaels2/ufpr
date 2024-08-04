#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

#include "game.h"

#define IP_ADRESS "127.0.0.1"
#define N 4
#define BUFFER 1024
#define N_CARDS 40

int main (int argc, char **argv) {
    int run = 1;
    int id = atoi (argv[1]);
    char buffer[BUFFER];
    frame_t *frame;
    info_t *info;

    frame = frame_init (id);
    info  = info_init ();

    srand (time (NULL));
    
    while (run) {
        /* carteador */
        if (id == info -> dealer) {
            /* inicializa as cartas utilizadas e a manilha */
            int shackle = -1;
            card_t used[N_CARDS + 1];
            for (int i = 0; i < N_CARDS + 1; i++) {
                used[i].suit = -1; 
                used[i].value = -1;
            }

            /* incicializa o frame para o dealer */
            frame -> addr = id;
            frame -> src  = id;
            for (int i = 0; i < N; i++) frame -> dest[i] = -1;
            
            /* espera o carteador iniciar a partida */
            printf ("digite (d) para distribuir as cartas: ");
            fgets (buffer, sizeof (buffer), stdin);

            /* sorteia e distruibui as cartas para os jogadores */
            draw_cards (frame, info, used, id);

            /* sorteia e distribui carta vira */
            draw_vira (frame, info, used, &shackle, id);

            /* recolhe as apostas dos jogadores */
            get_bets (frame, info, id);

            /* realiza os lances da rodada */
            run_game (frame, info, shackle, id);

            /* brief encerra a rodada  */
            end_round (frame, info, id);
            
        }

        /* jogadores */
        else {
            recv_msg (frame, info, id);
            send_msg (frame, frame -> addr);
        }

        if (no_lives (info)) break;
    }

    /* verifica se algum jogador perdeu as vidas */
    printf ("fim de jogo !\n");

    return 0;
}