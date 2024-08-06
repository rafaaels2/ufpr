#include "game.h"

int ports[4] = {32000, 32001, 32002, 32003};
int values[10] = {52, 53, 54, 55, 81, 74, 75, 65, 50, 51};
int suits[4] = {1, 2, 3, 4};

/* variaveis para armazenar as cartas de cada jogador */
card_t cards[MAX_CARDS]; int n_card = 0;

/* FRAME ------------------------------------------------------------------ */
int get_port (int addr, int type) {
    /* envio */
    if (type == 1) {
        if (addr == 0) return 1;
        if (addr == 1) return 2;
        if (addr == 2) return 3;
        if (addr == 3) return 0;
    }
    
    /* recebimento */
    if (type == 0) {
        if (addr == 0) return 0;
        if (addr == 1) return 1;
        if (addr == 2) return 2;
        if (addr == 3) return 3;
    }

    return -1;
}

void get_dest (int *dest, int addr) {
    if (addr == 0) { dest[0] = 1; dest[1] = 2; dest[2] = 3; }
    if (addr == 1) { dest[0] = 2; dest[1] = 3; dest[2] = 0; }
    if (addr == 2) { dest[0] = 3; dest[1] = 0; dest[2] = 1; }
    if (addr == 3) { dest[0] = 0; dest[1] = 1; dest[2] = 2; }
}

int verify_dest (int *dest, int addr) {
    for (int i = 0; i < N - 1; i++) 
        if (dest[i] == addr)
            return 1;

    return 0;
}

int recv_msg (frame_t *frame, info_t *info, int addr) {
    int sockfd;
    struct sockaddr_in dest_addr, src_addr;
    socklen_t src_len = sizeof (src_addr);
    
    /* inicializa o socket */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);

    memset (&dest_addr, 0, sizeof (dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);
    dest_addr.sin_port = htons (ports[get_port (addr, 0)]);
    
    /* define a porta da mensagem */
    bind (sockfd, (struct sockaddr *) &dest_addr, sizeof (dest_addr));
    
    /* recebe a mensagem */
    recvfrom (sockfd, frame, sizeof (*frame), 0, (struct sockaddr *) 
              &src_addr, &src_len);

    /* problema ao receber pacote */
    /* if (n < 0) {
        int i = 0;

        while (frame -> recv[i] != -1) i++;
        frame -> recv[i] = 0;
    } */

    // sleep (1);

    #ifdef DEBUG
    printf ("# pacote recebido\n");
    #endif 

    /* verifica se o pacote nao esta vazio */
    if (frame -> type != NOT) {
        /* pacote retornou ao destino */
        if (frame -> src == addr) {
            int i = 0;

            /* assume que todos receberam a mensagen */
            frame -> status = 1;

            /* verifica se todos os destinatarios receberam a mensagem */
            while (frame -> recv[i] != -1 && i != 3) {
                if (!frame -> recv[i]) 
                    frame -> status = 0;

                i++;
            }

            /* reset o vetor de recebimento */
            for (int i = 0; i < N - 1; i++) frame -> recv[i]  = -1;
        }

        /* verifica se a mensagem eh para o jogador atual */
        else if (verify_dest (frame -> dest, addr) /* || n >= 0 */) {
            int i = 0;

            /* marca a mensagem como recebida */
            while (frame -> recv[i] != -1) i++;
            frame -> recv[i] = 1;

            /* mensagem da carta sorteada */
            if (frame -> type == CARD) {
                printf ("%d. ", n_card);

                cards[n_card++] = frame -> data.card;

                printf ("%s", card_print (frame -> data.card));
            }
            
            /* mensagem da carta vira */
            else if (frame -> type == VIRA) {
                cards[MAX_CARDS - 1] = frame -> data.card;

                printf ("\nV. ");
                printf ("%s", card_print (frame -> data.card));
                printf ("\n");
            }
            
            /* mensagem da aposta para a rodada */
            else if (frame -> type == BETS) {
                int aux = frame -> data.bet.starter;

                /* imprime as apostas feitas pelos jogadores ate aqui */
                for (int i = 0; i < frame -> data.bet.n_bets; i++) {
                    printf ("jogador %d faz %d\n", aux, 
                            frame -> data.bet.bets[aux]);

                    if (++aux == 4) aux = 0;
                }

                char buffer[BUFFER];

                /* recolhe a aposta do jogador atual */
                printf ("faz quantas: ");
                fgets (buffer, sizeof (buffer), stdin);
                printf ("\n");

                /* coloca a aposta no vetor de apostas */
                frame -> data.bet.bets[addr] = atoi (buffer);
                frame -> data.bet.n_bets++;
            }

            /* mensagem para todos os jogadores */
            else if (frame -> type == MSG)
                printf ("%s", frame -> data.msg);

            /* mensagem das jogadas do lance */
            else if (frame -> type == ROUND) {
                /* verifica se o jogador pode realizar a jogada */
                if ((frame -> data.round.starter == addr || 
                     frame -> data.round.n_plays != 0)   && 
                     frame -> data.round.n_plays != N) {

                    printf ("LANCE %d ----------\n", 
                    frame -> data.round.n_round);

                    /* imprime as cartas restantes do jogador */
                    for (int i = 0; i < n_card; i++) {
                        if (cards[i].value != -1) {
                            printf ("%d. ", i);
                            printf ("%s", card_print (cards[i]));
                        }
                    }

                    /* imprime a carta vira */
                    printf ("\nV. ");
                    printf ("%s", card_print (cards[MAX_CARDS - 1]));
                    printf ("\n");

                    int aux = frame -> data.round.starter;

                    /* imprime as cartas ja jogadas pelos jogadores ate aqui */
                    for (int i = 0; i < frame -> data.round.n_plays; i++) {
                        printf ("jogador %d jogou ", aux);
                        printf ("%s", card_print (frame -> data.round.cards[aux]));

                        if (++aux == 4) aux = 0;
                    }

                    char buffer[BUFFER];

                    /* recolhe a jogada do jogador atual */
                    printf ("\nqual carta (indice): ");
                    fgets (buffer, sizeof (buffer), stdin);
                    printf ("\n");

                    /* caso jogador digite uma carta nao possivel */
                    while (cards[atoi (buffer)].value == -1 || atoi (buffer) >= (MAX_CARDS - 1)) {
                        printf ("\nqual carta (indice): ");
                        fgets (buffer, sizeof (buffer), stdin);
                        printf ("\n");
                    }

                    /* coloca a jogada no vetor de cartas */
                    frame -> data.round.cards[addr] = cards[atoi (buffer)];
                    frame -> data.round.n_plays++;

                    /* retira a carta */
                    cards[atoi (buffer)].value = -1;
                }
            }

            /* mensagem de final da rodada */
            else if (frame -> type == END) {
                *info = frame -> data.info;

                for (int i = 0; i < MAX_CARDS; i++) cards[i].value = -1;
                n_card = 0;
            }
        }
    }

    close (sockfd);

    return 1;
}

int send_msg (frame_t *frame, int addr) {
    int sockfd;
    struct sockaddr_in dest_addr;
    
    /* inicializa o socket */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);

    memset (&dest_addr, 0, sizeof (dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);
    dest_addr.sin_port = htons (ports[get_port (addr, 1)]);

    // sleep (1);

    /* redefine o endereco atual da mensagem */
    frame -> addr = get_port (addr, 1);

    /* envia a mensagem */
    sendto (sockfd, frame, sizeof (*frame), 0, (struct sockaddr *) 
            &dest_addr, sizeof (dest_addr));

    #ifdef DEBUG
    printf ("# pacote enviado\n");
    #endif

    close (sockfd);

    return 1;
}

frame_t *frame_init (int addr) {
    frame_t *frame = malloc (sizeof (frame_t));
    if (!frame) {
        perror ("erro ao alocar memoeria");
        exit (0);
    }

    for (int i = 0; i < N; i++) frame -> dest[i] = -1;
    for (int i = 0; i < N; i++) frame -> recv[i]  = -1;
    frame -> status = 0;
    frame -> type = NOT;

    return frame;
}

/* CARDS ------------------------------------------------------------------ */
char *card_print (card_t card) {
    char suit_draw[4];

    /* atribui o naipe */
    if      (suits[card.suit] == 1) strcpy (suit_draw, "♦");
    else if (suits[card.suit] == 2) strcpy (suit_draw, "♠");
    else if (suits[card.suit] == 3) strcpy (suit_draw, "♥");
    else if (suits[card.suit] == 4) strcpy (suit_draw, "♣");
    
    char *result = malloc ((14 * sizeof (char)));
    if (!result) {
        perror ("erro ao alocar memoria");
        exit (0);
    }

    /* pega a string final da carta */
    sprintf (result, "| %c   %s |\n", (char) values[card.value], suit_draw);
    
    return result;
}

int verify_card (card_t *used, card_t card) {
    int i = 0;

    /* laco entre todas as cartas de `used` */
    while (used[i].value != -1) {
        if (used[i].value == card.value && used[i].suit  == card.suit) 
            return 0;

        i++;
    }

    return 1;
}

card_t get_card (card_t *used) {
    card_t card;

    /* sorteia os indices para formar a carta */
    card.value = rand() % (9 - 0 + 1) + 0;
    card.suit  = rand() % (3 - 0 + 1) + 0;

    while (!verify_card (used, card)) {
        card.value = rand() % (9 - 0 + 1) + 0;
        card.suit  = rand() % (3 - 0 + 1) + 0;
    }

    return card;
}

int cmp_card (card_t card_1, card_t card_2, int shackle) {
    /* compara cartas iguais */
    if (card_1.value == card_2.value && card_1.suit < card_2.suit) 
        return 1;

    /* verifica se eh manilha */
    else if (card_2.value == shackle)
        return 1;

    /* compara os valores das cartas */
    else if (card_1.value < card_2.value && card_1.value != shackle) 
        return 1;
    
    return 0;
}

void draw_cards (frame_t *frame, info_t *info, card_t *used, int addr) {
    card_t card;

    /* inicializa a mensagem para o carteador */
    frame -> type = CARD;
    frame -> dest[0] = addr;
    
    for (int i = 0; i < N * info -> n_cards; i++) {
        /* pega a carta e a armazena */
        card = get_card (used);
        used[i] = card;
        
        /* caso a carta seja do carteador */
        if (frame -> dest[0] == addr) {
            /* imprime e armazena a carta sorteada */
            printf ("%d. ", n_card);
            cards[n_card++] = card;
            printf ("%s", card_print (card));
        }

        /* caso a carta seja de outro jogador */
        else {
            /* envia a carta e espera a resposta */
            frame -> data.card = card;
            send_msg (frame, addr);
            recv_msg (frame, info, addr);

            /* verifica a integridade do envio */
            while (frame -> status != 1) {
                /* reenvia a mensagem */
                send_msg (frame, addr);
                recv_msg (frame, info, addr);
            }

            #ifdef DEBUG
            printf ("# pacote retornou\n");
            #endif

            frame -> status = 0;
        }

        /* define o proximo destino */
        if (++frame -> dest[0] == 4) frame -> dest[0] = 0;
    }
}

void draw_vira (frame_t *frame, info_t *info, card_t *used, int *shackle, int addr) {
    card_t vira;

    /* inicializa a mensagem */
    frame -> type = VIRA;
    get_dest (frame -> dest, addr);

    /* sortea a carta vira */
    vira = get_card (used);
    cards[MAX_CARDS - 1] = vira;

    /* envia a carta e espera a resposta */
    frame -> data.card = vira;
    send_msg (frame, addr);
    recv_msg (frame, info, addr);

    /* verifica a integridade do envio */
    while (frame -> status != 1) {
        /* reenvia a mensagem */
        send_msg (frame, addr);
        recv_msg (frame, info, addr);
    }

    #ifdef DEBUG
    printf ("# pacote retornou\n");
    #endif

    frame -> status = 0;

    /* carteador imprime a carta vira */
    printf ("\nV. ");
    printf ("%s", card_print (vira));
    printf ("\n");

    /* calcula a manilha */
    *shackle = vira.value + 1;
    if (*shackle == 10) *shackle = 0;
}

void inform (frame_t *frame, info_t *info, int addr) {
    /* inicializa a mensagem */
    frame -> type = MSG;
    get_dest (frame -> dest, addr);

    /* envia a carta e espera a resposta */
    send_msg (frame, addr);
    recv_msg (frame, info, addr); 

    /* verifica a integridade do envio */
    while (frame -> status != 1) {
        /* reenvia a mensagem */
        send_msg (frame, addr);
        recv_msg (frame, info, addr);
    }

    #ifdef DEBUG
    printf ("# pacote retornou\n");
    #endif

    frame -> status = 0;

    printf ("%s", frame -> data.msg);
}

void get_bets (frame_t *frame, info_t *info, int addr) {
    int sum = info -> n_cards;
    char buffer[BUFFER];

    /* inicializa a mensagem */
    frame -> type = BETS;
    get_dest (frame -> dest, addr);
    frame -> data.bet.n_bets = 0;
    frame -> data.bet.starter = info -> starter;
    for (int i = 0; i < N; i++) frame -> data.bet.bets[i] = -1;

    /* laco ate a soma das apostas ser diferente do nuemero de cartas */
    while (sum == info -> n_cards) {
        /* envia a carta e espera a resposta */
        send_msg (frame, addr);
        recv_msg (frame, info, addr);

        /* verifica a integridade do envio */
        while (frame -> status != 1) {
            /* reenvia a mensagem */
            send_msg (frame, addr);
            recv_msg (frame, info, addr);
        }

        #ifdef DEBUG
        printf ("# pacote retornou\n");
        #endif

        frame -> status = 0;

        int aux = frame -> data.bet.starter;
        
        /* imprime as apostas feitas pelos jogadores */
        for (int i = 0; i < frame -> data.bet.n_bets; i++) {
            printf ("jogador %d faz %d\n", aux, 
            frame -> data.bet.bets[aux]);

            if (++aux == 4) aux = 0;
        }

        /* recolhe a aposta do carteador */
        printf ("faz quantas: ");
        fgets (buffer, sizeof (buffer), stdin);
        printf ("\n");

        /* coloca a aposta no vetor de apostas */
        frame -> data.bet.bets[addr] = atoi (buffer);
        frame -> data.bet.n_bets++;

        /* adiciona as apostas nas informacoes do jogo */
        for (int i = 0; i < N; i++) info -> bets[i] = frame -> data.bet.bets[i];

        /* soma valor das apostas */
        sum = frame -> data.bet.bets[0] + frame -> data.bet.bets[1] + 
              frame -> data.bet.bets[2] + frame -> data.bet.bets[3];

        frame -> data.bet.n_bets = 0;
    }

    /* informa as apostas feitas */
    sprintf (frame -> data.msg, "jogador 0 faz %d\njogador 1 faz %d\njogador 2 faz %d\njogador 3 faz %d\n\n", 
            frame -> data.bet.bets[0], frame -> data.bet.bets[1], frame -> data.bet.bets[2], frame -> data.bet.bets[3]);
    inform (frame, info, addr);
}

int get_winner (frame_t *frame, info_t *info, int shackle) {
    int gtr_addr;
    card_t grt_card;

    grt_card = frame -> data.round.cards[0];
    gtr_addr = 0;

    /* laco entre todas as jogadas */
    for (int i = 1; i < N; i++) {
        if (cmp_card (grt_card, frame -> data.round.cards[i], shackle)) {
            gtr_addr = i;
            grt_card = frame -> data.round.cards[i];
        }
    }

    return gtr_addr;
}

void run_game (frame_t * frame, info_t *info, int shackle, int addr) {
    char buffer[BUFFER];

    /* inicializa mensagem */
    get_dest (frame -> dest, addr);

    while (info -> n_cards != info -> crr_round) {
        /* inicializa a mensagem do lance */
        frame -> type = ROUND;
        frame -> data.round.n_plays = 0;
        frame -> data.round.starter = info -> starter;
        frame -> data.round.n_round = ++info -> crr_round;

        /* laco ate todos jogarem */
        while (frame -> data.round.n_plays != N) {
            /* envia a carta e espera a resposta */
            send_msg (frame, addr);
            recv_msg (frame, info, addr);

            /* verifica a integridade do envio */
            while (frame -> status != 1) {
                /* reenvia a mensagem */
                send_msg (frame, addr);
                recv_msg (frame, info, addr);
            }

            #ifdef DEBUG
            printf ("# pacote retornou\n");
            #endif

            frame -> status = 0;

            /* verifica se todos ja jogaram */
            if (frame -> data.round.n_plays != N) {
                /* imprime as cartas do jogador */
                printf ("LANCE %d ----------\n", frame -> data.round.n_round);
                for (int i = 0; i < MAX_CARDS - 1; i++) {
                    if (cards[i].value != -1) {
                        printf ("%d. ", i);
                        printf ("%s", card_print (cards[i]));
                    }
                }

                /* imprime a carta vira */
                printf ("\nV. ");
                printf ("%s", card_print (cards[MAX_CARDS - 1]));
                printf ("\n");

                int aux = frame -> data.round.starter;

                /* imprime as jogadas feitas pelos jogadores */
                for (int i = 0; i < frame -> data.round.n_plays; i++) {
                    printf ("jogador %d jogou ", aux);
                    printf ("%s", card_print (frame -> data.round.cards[aux]));

                    if (++aux == 4) aux = 0;
                }

                /* recolhe a jogada do carteador */
                printf ("\nqual carta (indice): ");
                fgets (buffer, sizeof (buffer), stdin);
                printf ("\n");

                while (cards[atoi (buffer)].value == -1 || atoi (buffer) >= (MAX_CARDS - 1)) {
                    printf ("\nqual carta (indice): ");
                    fgets (buffer, sizeof (buffer), stdin);
                    printf ("\n");
                }

                /* coloca a jogada no vetor de cartas */
                frame -> data.round.cards[addr] = cards[atoi (buffer)];
                frame -> data.round.n_plays++;

                /* retira a carta */
                cards[atoi (buffer)].value = -1;
            }
        }

        /* pega o id do jogador vencedor */
        int gtr_addr = get_winner (frame, info, shackle);

        /* informa as cartas jogadas */
        sprintf (frame -> data.msg, "jogador 0 jogou %sjogador 1 jogou %sjogador 2 jogou %sjogador 3 jogou %s\n", 
                 card_print (frame -> data.round.cards[0]), card_print (frame -> data.round.cards[1]), 
                 card_print (frame -> data.round.cards[2]), card_print (frame -> data.round.cards[3]));
        inform (frame, info, addr);

        /* informa o vencedor do lance */
        sprintf (frame -> data.msg, "jogador %d venceu o lance %d\n\n", gtr_addr, info -> crr_round);
        inform (frame, info, addr);

        /* atualiza as infomacoes do jogo */
        info -> starter = gtr_addr;
        info -> wins[gtr_addr]++;
    }

    /* calcula as vidas restantes de cada jogador */
    int diff;
    for (int i = 0; i < N; i++) {
        diff = info -> bets[i] - info -> wins[i];
        if (diff < 0)
            diff = diff * -1;

        info -> lifes[i] = info -> lifes[i] - diff;
    }

    /* informa as vidas restantes de cada jogador */
    sprintf (frame -> data.msg, "jogador 0 tem %d vida\njogador 1 tem %d vidas\njogador 2 tem %d vidas\njogador 3 tem %d vidas\n\n", 
            info -> lifes[0], info -> lifes[1], info -> lifes[2], info -> lifes[3]);
    inform (frame, info, addr);
}

void end_round (frame_t *frame, info_t *info, int addr) {
    /* informa o fim da rodada */
    strcpy (frame -> data.msg, "FIM DA RODADA\n\n");
    inform (frame, info, addr);

    /* inicializa a mensagem */
    frame -> type = END;
    get_dest (frame -> dest, addr);

    /* define o proximo dealer */
    if (++info -> dealer == 4) info -> dealer = 0;
    while (info -> lifes[info -> dealer] <= 0)
        info -> dealer++;

    /* define o proximo starter */
    info -> starter = info -> dealer + 1;
    if (info -> starter == 4) info -> starter = 0;
    while (info -> lifes[info -> starter] <= 0)
        info -> starter++;

    /* reinicia a contagem dos lances */
    info -> crr_round = 0;

    /* reinicia as apostas e os numeros de lances vencidos */
    for (int i = 0; i < N; i++) info -> bets[i] = 0;
    for (int i = 0; i < N; i++) info -> wins[i] = 0;

    /* diminui o numero de cartas para a proxima rodada */
    if (info -> op == 0) {
        /* se ja estive no minimo, aumenta e troca a operação */
        if (--info -> n_cards == 0) {
            info -> n_cards = info -> n_cards + 2;
            info -> op = 1;
        }
    }

    /* aumenta o numero de cartas para a proxima rodada */
    else {
        if (++info -> n_cards == 6) {
            /* se ja estive no minimo, aumenta e troca a operação */
            info -> n_cards = info -> n_cards - 2;
            info -> op = 0;
        }
    }

    /* envia a mensagem e espera a resposta */
    frame -> data.info = *info;
    send_msg (frame, addr);
    recv_msg (frame, info, addr);

    /* verifica a integridade do envio */
    while (frame -> status != 1) {
        /* reenvia a mensagem */
        send_msg (frame, addr);
        recv_msg (frame, info, addr);
    }

    #ifdef DEBUG
    printf ("# pacote retornou\n");
    #endif

    frame -> status = 0;

    /* reinicia as cartas do carteador */
    for (int i = 0; i < MAX_CARDS; i++) cards[i].value = -1;
    n_card = 0;
}

int no_lives (info_t *info) {
    /* laco entre as vidas de cada jogador */
    for (int i = 0; i < N; i++) 
        if (info -> lifes[i] <= 0) 
            return 1;

    return 0;
}


/* INFO ------------------------------------------------------------------- */
info_t *info_init () {
    info_t *info = malloc (sizeof (info_t));
    if (!info) {
        perror ("erro ao alocar memoria");
        exit (0);
    }

    info -> dealer = 0;
    info -> starter = 1;
    info -> n_cards = 5;
    info -> crr_round = 0;
    info -> op = 0;
    for (int i = 0; i < N; i++) info -> lifes[i] = 12;
    for (int i = 0; i < N; i++) info -> bets[i] = 0;
    for (int i = 0; i < N; i++) info -> wins[i] = 0;

    /* inicializa o vetor que armazena as cartas */
    n_card = 0;
    for (int i = 0; i < MAX_CARDS; i++) cards[i].value = -1;

    return info;
}