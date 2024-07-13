#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "192.168.0.166"
#define PORT 32000
#define BUF_SIZE 1024

int main() {
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

