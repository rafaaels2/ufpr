#ifndef MENSAGEM_H
#define MENSAGEM_H

// Tipos de mensagem
#define ACK 0b00000
#define NACK 0b00001
#define LISTA 0b01010
#define BAIXAR 0b01011
#define MOSTRA 0b10000
#define DESCRITOR 0b10001
#define DADOS 0b10010
#define FIM_TX 0b11110
#define ERRO 0b11111

// Estados
#define M_ENVIA 0
#define M_RECEBE 1
#define ENVIA_DADOS 0
#define RECEBE_DADOS 1
#define ENVIA_LISTA 2
#define RECEBE_LISTA 3
#define ESPERA 4

// Gerencial
#define TAM_MSG 63
#define TAM_MIN 14
#define OFFSET 3
#define TAM_EXTRA 70

#define EQ_CRC 0xd5

void copia_todos(unsigned char a[], unsigned char b[], int tam);

int cria_raw_socket(char* nome_interface_rede);

long long timestamp();

int protocolo_e_valido(unsigned char* buffer, int tamanho_buffer);

int recebe_mensagem(int soquete, int timeoutMillis, unsigned char* buffer, int tamanho_buffer);

void prepara_mensagem(unsigned char msg[], unsigned char marcador, unsigned char tamanho, unsigned char sequencia, unsigned char tipo);

unsigned char obtem_tamanho(unsigned char msg[]);

unsigned char obtem_sequencia(unsigned char msg[]);

unsigned char obtem_tipo(unsigned char msg[]);

unsigned char obtem_crc(unsigned char msg[]);

#endif