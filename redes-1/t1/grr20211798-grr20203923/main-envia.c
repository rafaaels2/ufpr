#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/time.h>
#include <unistd.h>

#include "mensagem.h"
#include "dados.h"

#define TAM_MSG 63
#define TAM_MIN 14
#define OFFSET 3

int main(int argc, char *argv[]) {
    // lo: LOOPBACK(Maquina envia pra si mesma)
    int socket_send = cria_raw_socket("enx000ec61e3fa1");
    int socket_recv = cria_raw_socket("enx000ec61e3fa1");

    unsigned char buffer[TAM_MSG + OFFSET + TAM_EXTRA];
    unsigned char bufferRecv[TAM_MSG + OFFSET + TAM_EXTRA];

    FILE* arq1;
    /* printf("insira o nome do arquivo de saida: ");
    scanf("%s", arqsaida); */
    // FILE* arq2 = fopen("teste.jpg", "w+");

    char c;
    int seq = 0;
    int modo = M_RECEBE;
    int fimMsg = 0;
    int termina = 0;
    int tipoMsg;
    int tipoEnvio;
    for(;;) {
        if (modo == M_RECEBE) {
            // printf("recebe\n");
            int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET);
            //
            //
            // IMPORTANTE: A MENSAGEM RECEBE DUAS VEZES PELA FORMA QUE O LOOPBACK FUNCIONA. TESTES DESSA PARTE VAO SER NECESSARIOS QUANDO TROCAR PRA DUAS MAQUINAS
            //
            //
            /* if (recebe != -1) {
                recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET);
            } */
            if (strlen(bufferRecv) == 0) {
                // printf("chegou vazio\n");
                continue;
            }
            if (recebe == -1) {
                // printf("chegou errado\n");
                tipoMsg = NACK;
                modo = M_ENVIA;
                continue;
            }

            if (obtem_tipo(bufferRecv) == BAIXAR) {
                // printf("chegou certo\n");
                // recebe_dados(socket_send, socket_recv, bufferRecv, &seq, &seqRec, bufferSend, arq1);
                tipoMsg = ACK;
                modo = M_ENVIA;
                tipoEnvio = BAIXAR;
                continue;
            }
            if (obtem_tipo(bufferRecv) == LISTA) {
                // printf("chegou certo\n");
                tipoMsg = ACK;
                modo = M_ENVIA;
                tipoEnvio = LISTA;
                continue;
            }
        }
        else if (modo == M_ENVIA) {
            // printf("agora envia\n");
            int envia;
            // printf("buffer send = %s\n", bufferSend);
            prepara_mensagem(buffer, 0x7f, 0, seq, tipoMsg);
            // seq = (seq + 1) % 32;

            int envio;
            // sleep(1);
            envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
            // printf("%d\n", obtem_sequencia(bufferSend));
            // printf("recebe\n");
            modo = M_RECEBE;

            char arqNome[63];
            strcpy(arqNome, bufferRecv + OFFSET);
            arqNome[strlen(arqNome) - 1] = '\0';

            // necessario pro loopback
            // int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
            // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);

            if (tipoMsg == ACK) {
                if (tipoEnvio == BAIXAR) {
                    printf("Enviando arquivo: %s\n", arqNome);
                    arq1 = fopen(arqNome, "r");
                    if (arq1 == NULL)
                        // printf("problemas\n");
                    // sleep(2);
                    // printf("atual seq: %d\n", seq);

                    seq = 0;

                    envia_dados(socket_send, socket_recv, buffer, &seq, bufferRecv, arq1);
                    fclose(arq1);
                    printf("Terminou de enviar.\n");
                }
                if (tipoEnvio == LISTA) {
                    seq = 0;
                    envia_lista(socket_send, socket_recv, buffer, &seq, bufferRecv);
                }
            }
        }
    }
    // envia_dados(socket_send, socket_recv, buffer, &seq, bufferRecv, arq1);
    // fclose(arq1);
    // fclose(arq2);
}