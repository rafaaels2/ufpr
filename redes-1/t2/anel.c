#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define IP_ADRESS "127.0.0.1"

/*
 * 0 > 32001 && 0 < 32000
 * 1 > 32002 && 1 < 32001
 * 2 > 32003 && 2 < 32002
 * 3 > 32000 && 3 < 32003 
 */

typedef struct package_t {
    int remetente, destinatario, recebido, lido;
    char message[1024];
} package_t ;

int ports[4] = {32000, 32001, 32002, 32003};

void send_message (package_t package, int id) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("erro ao criar socket");
        exit (0);
    }

    memset (&server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);

    switch(id) {
        case 0:
            server_addr.sin_port = htons (ports[1]);
            break;
        case 1:
            server_addr.sin_port = htons (ports[2]);
            break;
        case 2:
            server_addr.sin_port = htons (ports[3]);
            break;
        case 3:
            server_addr.sin_port = htons (ports[0]);
            break;
    }

    sendto (sockfd, &package, sizeof (package), 0, (struct sockaddr *) &server_addr, sizeof (server_addr));
    
    printf ("# pacote enviado\n");

    close (sockfd);
}

void recieve_message (package_t package, int id) {
    int sockfd, n;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof (client_addr);

    if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("erro ao criar socket");
        exit (0);
    }

    memset (&server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);

    switch(id) {
        case 0:
            server_addr.sin_port = htons (ports[0]);
            break;
        case 1:
            server_addr.sin_port = htons (ports[1]);
            break;
        case 2:
            server_addr.sin_port = htons (ports[2]);
            break;
        case 3:
            server_addr.sin_port = htons (ports[3]);
            break;
    }

    if (bind (sockfd, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
        perror("erro ao realizar a bind");
        exit(0);
    }
    
    n = recvfrom (sockfd, &package, sizeof (package), 0, (struct sockaddr *) &client_addr, &client_len);
    
    printf ("# pacote recebido\n");

    if (n < 0) {
        perror ("erro ao receber mensagem");
        exit (0);
    }
    else {
        if (package.destinatario == id) {
            package.message[n] = '\0';
            printf ("# %s\n", package.message);
            
            package.lido = 1;
            package.recebido = 1;

            send_message (package, id);
        }
        else if (package.remetente == id) {
            printf ("# mensagem voltou\n");

            if (package.recebido && package.lido)
                printf ("# deu certo\n");
            
            close (sockfd);

            exit (0);
        }
        else {
            printf ("# destino errado\n");

            send_message (package, id);
        }
    }

    close (sockfd);
}

int main (int argc, char **argv) {
    int envia = 0;

    if (atoi (argv[1]) == 0) 
        envia = 1;
    
    package_t package;

    package.remetente = 0;
    package.destinatario = 2;
    package.recebido = 0;
    package.lido = 0;
    strcpy (package.message, "bastao chegou");

    while (1) {
        if (envia) 
            send_message (package, atoi (argv[1]));

        recieve_message (package, atoi (argv[1]));
    }

    return 0;
}