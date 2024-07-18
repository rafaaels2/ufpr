// Rafael Gonçalves dos Santos - GRR20211798

/*
 * AJEITAR OS NOMES DA VARIAVEIS 
 * DEIXAR EM INGLES
 * DIMINUIR O TAMAMHO
 * 
 * CARTAS_NUMEROS = 8 =  Q, 9 = J, 10 = K, A = 1
 * CARTAS_NAIPES  = 1 = ouro, 2 = espadas, 3 = copas, 4 = paus
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include "carta.h"
#include "socket.h"

#define N 4
#define IP_ADRESS "127.0.0.1"
#define BUFFER_SIZE 1024

int main (int argc, char **argv) {
    int envia = 0, dealer = 0, remetente = 0, n_cards = 0;
    
    /* inicializa as cartas */
    card cards[5], used_cards[20];
    for (int i = 0; i < 5; i++) {
        cards[i].value = -1;
        cards[i].suit  = -1;
    }
    
    package_t package;

    if (atoi (argv[1]) == 0) {
        envia = 1;
        dealer = 1;
    }

    srand (time (NULL));

    printf ("# jogador %d\n", atoi (argv[1]));
    
    if (dealer) {
        for (int i = 0; i < 20; i++) {
            used_cards[i].value = -1;
            used_cards[i].suit  = -1;
        }
        for (int i = 0; i < 25; i++) {
            if (envia) {
                if (remetente == atoi (argv[1])) {
                    cards[++n_cards] = get_card (used_cards);
                    print_card (cards[n_cards]);
                    used_cards[i] = cards[n_cards];
                    remetente++;
                }

                send_message (package, argv[1]);
            }
            
            recieve_message (package, argv[1]);
        }
    }

    return 0;
}