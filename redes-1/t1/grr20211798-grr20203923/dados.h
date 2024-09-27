#ifndef DADOS_H
#define DADOS_H

#define TAM_JANELA 5

void envia_lista(int socket_send, int socket_recv, char buffer[], int *seq, char bufferRecv[]);

void recebe_lista(int socket_send, int socket_recv, char buffer[], int *seq, int *seqRec, char bufferSend[]);

void envia_dados(int socket_send, int socket_recv, char buffer[], int *seq, char bufferRecv[], FILE* arqRecebe);

void recebe_dados(int socket_send, int socket_recv, char buffer[], int *seq, int *seqRec, char bufferSend[], FILE* arqRecebe);

#endif