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

int main(int argc, char *argv[]) {
    // lo: LOOPBACK(Maquina envia pra si mesma)
    int socket_recv = cria_raw_socket("enx000ec61e4a99");
    int socket_send = cria_raw_socket("enx000ec61e4a99");

    unsigned char buffer[TAM_MSG + OFFSET + TAM_EXTRA];
    unsigned char bufferSend[TAM_MSG + OFFSET + TAM_EXTRA];

    FILE* arq1;

    char c;

    int modo = M_ENVIA;
    int tipo_msg;
    int seq = 0;
    int seqRec = 0;
    for(;;) {
        char entrada = 0;
        char nomeArq[60];
        printf("Escreva o que quer fazer:\n1-Listar videos\n2-Baixar videos\nQualquer outro digito-Fechar o programa\n");
        scanf("%c", &entrada);

        //Pegar o newline
        fgets(nomeArq, 60, stdin);
        if (entrada == '1') {
            // printf("Ainda a ser adicionado\n");
            for (;;) {
                if (modo == M_ENVIA) {
                    strcpy(bufferSend + OFFSET, nomeArq);
                    prepara_mensagem(bufferSend, 0x7f, strlen(nomeArq), seq, LISTA);
                    seq = (seq + 1) % 32;

                    int envio;
                    envio = send(socket_send, bufferSend, TAM_MSG + OFFSET + TAM_EXTRA, 0);

                    // necessario pro loopback
                    // int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                    // recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                    modo = M_RECEBE;
                }
                else {
                    int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET);
                    //
                    //
                    // IMPORTANTE: A MENSAGEM RECEBE DUAS VEZES PELA FORMA QUE O LOOPBACK FUNCIONA. TESTES DESSA PARTE VAO SER NECESSARIOS QUANDO TROCAR PRA DUAS MAQUINAS
                    //
                    //
                    /* if (recebe != -1) {
                        recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET);
                    } */
                    if (recebe == -1 || strlen(buffer) == 0)
                        continue;
                    if (obtem_tipo(buffer) == ACK) {
                        printf("Obtendo lista...\n");
                        // rewind(arq1);
                        // seqRec = (obtem_sequencia(buffer) + 1) % 32;

                        seqRec = 0;
                        // recebe_dados(socket_send, socket_recv, buffer, &seq, &seqRec, bufferSend, arq1);
                        recebe_lista(socket_send, socket_recv, buffer, &seq, &seqRec, bufferSend);

                        modo = M_ENVIA;
                        break;
                    }
                    else if (obtem_tipo(buffer) == NACK) {
                        // seqRec = (obtem_sequencia(buffer) + 1) % 32;
                        modo = M_ENVIA;
                    }
                }
            }
        }
        else if (entrada == '2') {
            printf("Insira o nome do video a ser instalado:\n");
            fgets(nomeArq, 60, stdin);
            nomeArq[strlen(nomeArq) - 1] = '\0';

            for (;;) {
                if (modo == M_ENVIA) {
                    strcpy(bufferSend + OFFSET, nomeArq);
                    prepara_mensagem(bufferSend, 0x7f, strlen(nomeArq), seq, BAIXAR);
                    seq = (seq + 1) % 32;

                    int envio;
                    envio = send(socket_send, bufferSend, TAM_MSG + OFFSET + TAM_EXTRA, 0);

                    // necessario pro loopback
                    // int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                    // recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                    modo = M_RECEBE;
                }
                else {
                    int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET);
                    //
                    //
                    // IMPORTANTE: A MENSAGEM RECEBE DUAS VEZES PELA FORMA QUE O LOOPBACK FUNCIONA. TESTES DESSA PARTE VAO SER NECESSARIOS QUANDO TROCAR PRA DUAS MAQUINAS
                    //
                    //
                    /* if (recebe != -1) {
                        recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET);
                    } */
                    if (recebe == -1 || strlen(buffer) == 0)
                        continue;
                    if (obtem_tipo(buffer) == ACK) {
                        printf("Baixando video...\n");
                        // rewind(arq1);
                        // seqRec = (obtem_sequencia(buffer) + 1) % 32;

                        char extensao = nomeArq[strlen(nomeArq) - 1];
                        if (extensao == '4')
                            arq1 = fopen("video.mp4", "w+");

                        else if (extensao == 'i')
                            arq1 = fopen("video.avi", "w+");

                        else {
                            perror ("extensao invalida");
                            exit (0);
                        } 


                        seqRec = 0;
                        recebe_dados(socket_send, socket_recv, buffer, &seq, &seqRec, bufferSend, arq1);
                        fclose(arq1);

                        modo = M_ENVIA;
                        break;
                    }
                    else if (obtem_tipo(buffer) == NACK) {
                        // seqRec = (obtem_sequencia(buffer) + 1) % 32;
                        modo = M_ENVIA;
                    }
                }
            }
        }
        else {
            printf("Fechando o programa\n");
            break;
        }
        int termina = 0;
    }


    // recebe_dados(socket_send, socket_recv, buffer, &seq, &seqRec, bufferSend, arq1);
    // fclose(arq1);
    // fclose(arq2);
}