// Rafael Gonçalves dos Santos - GRR20211798

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 33000
#define SERVER_IP "192.168.0.166"
#define BUF_SIZE 1024

int main (int argc, char **argv) {
    if (strcmp (argv[1], "e") == 0) {
        int sockfd;
        struct sockaddr_in server_addr;
        char buffer[BUF_SIZE];

        // Criar socket datagrama
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Erro ao criar socket");
            exit(EXIT_FAILURE);
        }

        // Configurar estrutura do servidor
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_port = htons(PORT);

        while (1) {
            printf("Digite uma mensagem para enviar ao servidor: ");
            fgets(buffer, BUF_SIZE, stdin);

            // Enviar mensagem ao servidor
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));

            // Limpar buffer
            memset(buffer, 0, BUF_SIZE);
        }

        // Fechar socket
        close(sockfd);

        return 0;
    }

    else if (strcmp (argv[1], "r") == 0) {
        int sockfd;
        struct sockaddr_in server_addr, client_addr;
        socklen_t client_len = sizeof(client_addr);
        char buffer[BUF_SIZE];

        // Criar socket datagrama
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Erro ao criar socket");
            exit(EXIT_FAILURE);
        }

        // Configurar estrutura do servidor
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(PORT);

        // Associar socket a um endereço/porta
        if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            perror("Erro ao associar endereço ao socket");
            exit(EXIT_FAILURE);
        }

        printf("Servidor esperando por mensagens...\n");

        while (1) {
            // Receber mensagem do cliente
            if (recvfrom(sockfd, buffer, BUF_SIZE, 0, (struct sockaddr *) &client_addr, &client_len) < 0) {
                perror("Erro ao receber mensagem");
                exit(EXIT_FAILURE);
            }

            printf("Mensagem recebida do cliente: %s\n", buffer);
        }

        // Fechar socket
        close(sockfd);

        return 0;
    }

    return 0;
}