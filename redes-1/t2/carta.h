#ifndef CARTA_H
#define CARTA_H

typedef struct card {
    int value, suit;
} card;

void print_card (card current_card);

int verify_card (card *used_cards, card current_card);

card get_card (card *used_cards);

#endif