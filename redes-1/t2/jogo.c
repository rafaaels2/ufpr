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

#define N 4
#define IP_ADRESS "192.168.0.166"
#define BUFFER_SIZE 1024

int main (int argc, char **argv) {
    int envia = 0, dealer = 0, n_times, n_cards;

    /* portas para envio das informacoes */
    int ports[N] = {32000, 32001, 32002, 32003};
    int port     = ports[atoi (argv[1])], next_port;

    /* inicializa as cartas */
    card cards[5];
    for (int i = 0; i < 5; i++) {
        cards[i].value = -1;
        cards[i].suit  = -1;
    }
    
    /* jogador 0 eh sempre o primeiro dealer */
    if ((port % 32000) == 0) {
        envia  = 1;
        dealer = 1;

        /* VALOR TESTE, NÃO EH CONSTANTE */
        n_times = N * 5;

        /* indica a proxima porta que ira enviar a carta */
        next_port = 32000;
    }

    srand (time (NULL));

    printf ("# jogador %d\n", atoi (argv[1]));
    
    while (1) {
        if (envia) {
            /* inicializa socket */
            int sockfd;
            struct sockaddr_in server_addr;
            card aux_card;

            /* VALOR MAXIMO, MAS BOM USAR MALLOC */
            card used_cards[20];
            for (int i = 0; i < 20; i++) {
                used_cards[i].value = -1;
                used_cards[i].suit  = -1;
            }

            if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror ("erro ao criar socket");
                exit (0);
            }

            memset (&server_addr, 0, sizeof (server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);
            /* inicializa socket */
            
            /* distribui n_cartas */
            for (int i = 0; i < n_times; i++) {
                /* verifica a porta que ira receber */
                if (next_port == port) {
                    cards[i * N] = get_card (used_cards);

                    print_card (cards[i * N]);

                    used_cards[i] = cards[i * N];

                    next_port++;
                    if (next_port == 32004)
                        next_port = 32000;
                }

                else {
                    server_addr.sin_port = htons(next_port);

                    aux_card = get_card (used_cards);
                    
                    sendto (sockfd, &aux_card, sizeof (aux_card), 0, (struct sockaddr *) &server_addr, sizeof (server_addr));

                    used_cards[i] = aux_card;

                    next_port++;
                    if (next_port == 32004)
                        next_port = 32000;
                }
            }

            exit (0);

            envia = 0;

            close (sockfd);
        }

        else if (!envia) {
            int sockfd, n;
            struct sockaddr_in server_addr, client_addr;
            socklen_t client_len = sizeof (client_addr);
            card aux_card;

            if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror ("erro ao criar socket");
                exit (0);
            }

            memset (&server_addr, 0, sizeof (server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
            server_addr.sin_port = htons (port);

            if (bind (sockfd, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
                perror("erro ao realizar a bind");
                exit(0);
            }
            
            /* VALOR TESTE, NÃO EH CONSTATE */
            for (int i = 0; i < 5; i++) {
                n = recvfrom (sockfd, &aux_card, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_len);

                if (n < 0) {
                    perror ("erro ao receber mensagem");
                    exit (0);
                }

                print_card (aux_card);
            }

            close (sockfd);
        }
    }

    return 0;
}