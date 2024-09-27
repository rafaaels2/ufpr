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
#include <dirent.h>

#include "mensagem.h"
#include "dados.h"

void envia_lista(int socket_send, int socket_recv, char buffer[], int *seq, char bufferRecv[]) {
    DIR *d;
    d = opendir(".");
    if (!d) {
        return;
    }
    struct dirent *dir;

    int ack = 0;

    while ((dir = readdir(d)) != NULL) {
        char *nome = dir->d_name;
        nome = nome + strlen(nome) - 3;
        if (strcmp(nome, "mp4") != 0 && strcmp(nome, "avi") != 0)
            continue;
        // printf("oi\n");

        int fimMsg = 0;
        int termina = 0;
        int modo = M_ENVIA;
        if (modo == M_ENVIA) {
            strcpy(buffer + OFFSET, dir->d_name);
            prepara_mensagem(buffer, 0x7f, strlen(dir->d_name), *seq, MOSTRA);
            *seq = (*seq + 1) % 32;

            int envio;
            if (strlen(buffer) != 0) {
                envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
            }
            modo = M_RECEBE;
            ack = 0;
        }
        else {
            while (!ack) {
                int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);

                if (recebe == -1 || strlen(bufferRecv) == 0)
                    continue;
                switch (obtem_tipo(bufferRecv)) {
                    case ACK:
                        // printf("ACK\n");
                        modo = M_ENVIA;
                        ack = 1;
                        break;
                    case NACK:
                        // printf("NACK\n");
                        int envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);

                        // loopback
                        // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
                        // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
                        break;
                    case ERRO:
                        break;
                    default:
                        break;
                }
            }
        }
    }

    closedir(d);

    prepara_mensagem(buffer, 0x7f, 0, *seq, FIM_TX);
    int envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
    ack = 0;
    while (!ack) {
        int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);

        if (recebe == -1 || strlen(bufferRecv) == 0)
            continue;
        switch (obtem_tipo(bufferRecv)) {
            case ACK:
                ack = 1;
                break;
            case NACK:
                int envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
                break;
            case ERRO:
                break;
            default:
                break;
        }
    }

    printf("Terminou de enviar lista\n");

    /*int termina = 0;
    while (!termina) {
        //
    }*/
}

void recebe_lista(int socket_send, int socket_recv, char buffer[], int *seq, int *seqRec, char bufferSend[]) {
    int modo = M_RECEBE;
    int tipo_msg;
    int termina = 0;
    for (;;) {
        if (modo == M_RECEBE) {
            // printf("recebe\n");
            int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
            if (strlen(buffer) == 0)
                continue;

            if (obtem_tipo(buffer) == ACK || obtem_tipo(buffer) == NACK)
                continue;

            if (*seqRec % 32 != obtem_sequencia(buffer)) {
                recebe = -1;
                //printf("ordem errada; ordem atual deve ser %d mas e %d\n", *seqRec % 32, obtem_sequencia(buffer));
            }

            if (recebe != -1)
                *seqRec = (*seqRec + 1) % 32;

            if (obtem_tipo(buffer) == FIM_TX) {
                //printf("acabou\n");
                termina = 1;
            }

            if (recebe != -1 && strlen(buffer) > 0) {
                int tam = obtem_tamanho(buffer);
                buffer[tam + OFFSET] = '\0';
                //printf("Tamanho: %d\n", tam);
                //printf("escrevendo %s\n", buffer + 3);
                // fwrite(buffer + OFFSET, recebe - OFFSET, 1, arqRecebe);
                // fwrite(buffer + OFFSET, tam, 1, arqRecebe);
                printf("%s\n", buffer + OFFSET);
                // printf("Enviando ACK: %d %d %d\n", obtem_sequencia(buffer), *seqRec, recebe);
                tipo_msg = ACK;
            }
            else {
                // printf("%d %d %d\n", obtem_sequencia(buffer), *seqRec, recebe);
                tipo_msg = NACK;
            }
            // sleep(1);
            modo = M_ENVIA;
        }
        else {
            // printf("buffer send = %s\n", bufferSend);
            prepara_mensagem(bufferSend, 0x7f, 0, *seq, tipo_msg);
            *seq = (*seq + 1) % 32;

            int envio;
            envio = send(socket_send, bufferSend, TAM_MSG + OFFSET + TAM_EXTRA, 0);
            // printf("%d\n", obtem_sequencia(bufferSend));
            // printf("recebe\n");
            modo = M_RECEBE;

            if (termina)
                break;
        }
    }
}

void envia_dados(int socket_send, int socket_recv, char buffer[], int *seq, char bufferRecv[], FILE* arqRecebe) {
    int fimMsg = 0;
    int termina = 0;
    int modo = M_ENVIA;
    int cont = 0;
    char janela[32][TAM_MSG + OFFSET + TAM_EXTRA];
    while (!termina) {
        if (modo == M_ENVIA) {
            for (int i = 0; i < TAM_JANELA; i++) {
                size_t leituraArq = fread(buffer + OFFSET, 1, TAM_MSG, arqRecebe);
                prepara_mensagem(buffer, 0x7f, leituraArq, *seq, DADOS);
                *seq = (*seq + 1) % 32;

                // buffer[leituraArq + OFFSET] = '\0';

                if (leituraArq == 0) {
                    *seq = (*seq - 1) % 32;
                    prepara_mensagem(buffer, 0x7f, leituraArq, *seq, FIM_TX);
                    fimMsg = 1;
                }
                /* if (leituraArq < TAM_MSG) {
                    for (int i = leituraArq + OFFSET; i < OFFSET + TAM_MSG; i++)
                        buffer[i] = '\0';
                } */

                int envio;
                if (strlen(buffer) != 0) {
                    copia_todos(janela[obtem_sequencia(buffer)], buffer, TAM_MSG + OFFSET + TAM_EXTRA - 1);
                    printf("enviando %d\n", obtem_sequencia(buffer));
                    envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
                    cont++;

                    // int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
                    // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);

                    // printf("tamanho: %d | sequencia: %d | tipo: %x\n", obtem_tamanho(buffer), obtem_sequencia(buffer), obtem_tipo(buffer));
                }
                // modo = M_RECEBE;
                // sleep(1);
                if (fimMsg)
                    break;
            }
            modo = M_RECEBE;
        }
        else {
            int recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
            //
            //
            // IMPORTANTE: A MENSAGEM RECEBE DUAS VEZES PELA FORMA QUE O LOOPBACK FUNCIONA. TESTES DESSA PARTE VAO SER NECESSARIOS QUANDO TROCAR PRA DUAS MAQUINAS
            //
            //
            /* if (recebe != -1) {
                recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
            } */
            if (recebe == -1 || strlen(bufferRecv) == 0)
                continue;
            switch (obtem_tipo(bufferRecv)) {
                case ACK:
                    // printf("ACK\n");
                    modo = M_ENVIA;
                    if (fimMsg)
                        termina = 1;
                    break;
                case NACK:
                    // printf("NACK de %d com seq %d\n", bufferRecv[OFFSET], obtem_sequencia(bufferRecv));
                    // int envio = send(socket_send, buffer, TAM_MSG + OFFSET + TAM_EXTRA, 0);
                    
                    int c = bufferRecv[OFFSET];
                    for (int i = c; i != *seq; i = (i + 1) % 32) {
                        // printf("reenviando %d\n", i);
                        int envio = send(socket_send, janela[i], TAM_MSG + OFFSET + TAM_EXTRA, 0);
                    }

                    // loopback
                    // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
                    // recebe = recebe_mensagem(socket_recv, 200, bufferRecv, TAM_MSG + OFFSET + TAM_EXTRA);
                    break;
                case ERRO:
                    break;
                default:
                    break;
            }
        }
    }
    // printf("%d\n", cont);
}

void recebe_dados(int socket_send, int socket_recv, char buffer[], int *seq, int *seqRec, char bufferSend[], FILE* arqRecebe) {
    int modo = M_RECEBE;
    int tipo_msg;
    int termina = 0;
    int cont = 0;
    char buffer_ordena[TAM_JANELA][TAM_MSG + OFFSET + TAM_EXTRA];
    for (;;) {
        if (modo == M_RECEBE) {
            int i = 0;
            while(i < TAM_JANELA) {
                // printf("recebe\n");
                int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                if (strlen(buffer) == 0)
                    continue;
                //
                //
                // IMPORTANTE: A MENSAGEM RECEBE DUAS VEZES PELA FORMA QUE O LOOPBACK FUNCIONA. TESTES DESSA PARTE VAO SER NECESSARIOS QUANDO TROCAR PRA DUAS MAQUINAS
                //
                //
                /* if (recebe != -1) {
                    recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
                } */

                // if (recebe != -1)
                    // printf("buffer = %s\n", buffer + 3);
                    // printf("tamanho: %d | sequencia: %d | tipo: %x\n", obtem_tamanho(buffer), obtem_sequencia(buffer), obtem_tipo(buffer));
                
                if (obtem_tipo(buffer) == ACK || obtem_tipo(buffer) == NACK)
                    continue;

                if (!(obtem_sequencia(buffer) == *seqRec % 32 || obtem_sequencia(buffer) == (*seqRec + 1) % 32 || obtem_sequencia(buffer) == (*seqRec + 2) % 32 || obtem_sequencia(buffer) == (*seqRec + 3) % 32 || obtem_sequencia(buffer) == (*seqRec + 4) % 32)) {
                    recebe = -1;
                    // printf("ordem errada; ordem atual deve ser %d mas e %d\n", *seqRec % 32, obtem_sequencia(buffer));
                }

                /* if (recebe != -1)
                    *seqRec = (*seqRec + 1) % 32; */

                if (obtem_tipo(buffer) == FIM_TX) {
                    //printf("acabou\n");
                    termina = 1;
                }

                if (recebe != -1 && strlen(buffer) > 0) {
                    cont++;
                    i++;
                    int ordem = ((obtem_sequencia(buffer)) - *seqRec + 32) % 32;
                    if (((obtem_sequencia(buffer_ordena[ordem])) - *seqRec + 32) % 32 != ordem) {
                        //printf("%d = %d - %d\n", ordem, obtem_sequencia(buffer), *seqRec);
                        // strcpy(buffer_ordena[ordem], buffer);
                        copia_todos(buffer_ordena[ordem], buffer, obtem_tamanho(buffer) + OFFSET);
                    }
                    // int tam = obtem_tamanho(buffer);
                    // printf("Tamanho: %d\n", tam);
                    // printf("escrevendo %s\n", buffer + 3);
                    // fwrite(buffer + OFFSET, recebe - OFFSET, 1, arqRecebe);
                    // fwrite(buffer + OFFSET, tam, 1, arqRecebe);
                    // printf("Enviando ACK: %d %d %d\n", obtem_sequencia(buffer), *seqRec, recebe);
                    tipo_msg = ACK;
                    if (termina == 1)
                        break;
                }
                else {
                    // printf("%d %d %d\n", obtem_sequencia(buffer), *seqRec, recebe);
                    tipo_msg = NACK;
                }
                // sleep(1);
            }
            modo = M_ENVIA;
            *seqRec = (*seqRec + 5) % 32;
            for (int j = 0; j < i; j++) {
                //printf("%s\n", buffer_ordena[j]);
                int tam = obtem_tamanho(buffer_ordena[j]);
                fwrite(buffer_ordena[j] + OFFSET, tam, 1, arqRecebe);
            }
        }
        else {
            // printf("buffer send = %s\n", bufferSend);
            prepara_mensagem(bufferSend, 0x7f, 0, *seq, tipo_msg);
            *seq = (*seq + 1) % 32;

            int envio;
            envio = send(socket_send, bufferSend, TAM_MSG + OFFSET + TAM_EXTRA, 0);
            // printf("%d\n", obtem_sequencia(bufferSend));
            // printf("recebe\n");
            modo = M_RECEBE;

            // necessario pro loopback
            // int recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);
            // recebe = recebe_mensagem(socket_recv, 200, buffer, TAM_MSG + OFFSET + TAM_EXTRA);

            if (termina)
                break;
        }
    }
    // printf("%d\n", cont);
}