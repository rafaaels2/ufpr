#ifndef SOCKET_H
#define SOCKET_H

#include "carta.h"

typedef struct package_t {
    int remetente, destinatario, recebido, lido;
    card current_card;
} package_t ;

void send_message (package_t package, int id);

void recieve_message (package_t package, int id);

#endif