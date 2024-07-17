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

int main (int argc, char **argv) {
    int envia = 0;

    if (atoi (argv[1]) == 0) 
        envia = 1;

    while (1) {
        if (envia) {
            int sockfd;
            struct sockaddr_in server_addr;
            package_t package;

            package.remetente = 0;
            package.destinatario = 2;
            package.recebido = 0;
            package.lido = 0;
            strcpy (package.message, "BASTÃO CHEGOU !!!");

            if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror ("erro ao criar socket");
                exit (0);
            }

            memset (&server_addr, 0, sizeof (server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);

            switch(atoi (argv[1])) {
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
            
            sleep (5);

            envia = 0;

            close (sockfd);
        }
        
        int sockfd, n;
        struct sockaddr_in server_addr, client_addr;
        socklen_t client_len = sizeof (client_addr);
        package_t package;

        if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror ("erro ao criar socket");
            exit (0);
        }

        memset (&server_addr, 0, sizeof (server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr (IP_ADRESS);

        switch(atoi (argv[1])) {
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

        printf ("remetente: %d\n", package.remetente);

        if (n < 0) {
            perror ("erro ao receber mensagem");
            exit (0);
        }
        else {
            if (package.destinatario == atoi (argv[1])) {
                package.message[n] = '\0';
                printf ("# %s\n", package.message);
                
                package.lido = 1;
                package.recebido = 1;

                printf ("recebido: %d lido: %d\n", package.recebido, package.lido);

                switch(atoi (argv[1])) {
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

                sleep (5);

                envia = 1;
            }
            else if (package.remetente == atoi (argv[1])) {
                printf ("# mensagem voltou\n");

                if (package.recebido && package.lido)
                    printf ("DEI CERTO !!!");

                exit (0);
            }

            else {
                printf ("# destino errado\n");

                switch(atoi (argv[1])) {
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

                sleep (5);
            }
        }

        close (sockfd);

        envia = 1;
    }

    return 0;
}