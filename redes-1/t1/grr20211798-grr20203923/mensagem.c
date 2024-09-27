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

void insere(unsigned char s[], int tam_max, int posicao) {
    int i;
    for (i = tam_max - 1; i > posicao + 1; i--) {
        s[i] = s[i - 1];
    }
    s[i] = 0xff;
    // s[i] = '0';
}


// Necessario, pois o strcpy nao vai depois do zero
void copia_todos(unsigned char a[], unsigned char b[], int tam) {
    for (int i = 0; i < tam; i++) {
        a[i] = b[i];
    }
}

unsigned char crc(unsigned char s[], int tam) {
    // O GRANDE PROBLEMA: O STRCPY NAO COPIA DEPOIS DE UM VALOR \0. TENHO QUE TRATAR DISSO
    unsigned char valor_crc[TAM_MSG + OFFSET + TAM_EXTRA];
    copia_todos(valor_crc, s, tam);
    // printf("%x %x %x, %x\n", s[0], s[1], s[2], s[3]);
    // printf("antes %x %x %x %x\n", valor_crc[0], valor_crc[1], valor_crc[2], valor_crc[3]);
    valor_crc[tam] = 0;
    valor_crc[tam + 1] = 0;
    // printf("agora %x %x %x %x\n", valor_crc[0], valor_crc[1], valor_crc[2], valor_crc[3]);

    for (int i = 0; i < tam; i++) {
        for (int j = 1; j < 9; j++) {
            unsigned char aux = valor_crc[i] << (j - 1);
            if (aux >> 7 == 0)
                continue;
            unsigned char q = valor_crc[i] << j;
            q += valor_crc[i + 1] >> (8 - j);
            /*if (q >> 7 == 0) {
                // printf(" %d\n", valor_crc[i]);
                continue;
            }*/
            unsigned char r = q ^ EQ_CRC;
            // printf(" %d", r);
            valor_crc[i] = r >> j;
            valor_crc[i + 1] = valor_crc[i + 1] << j;
            valor_crc[i + 1] = valor_crc[i + 1] >> j;
            // printf(" %d\n", valor_crc[i+1]);
            valor_crc[i + 1] += (r << (8 - j));
        }
    }
    // printf("%x\n", valor_crc[tam]);
    return valor_crc[tam];
}

void retira(unsigned char s[], int tam_max, int posicao) {
    // printf("%c\n", s[posicao + 1]);
    for (int i = posicao + 1; i < tam_max - 2; i++)
        s[i] = s[i + 1];
}

int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }

    int ifindex = if_nametoindex(nome_interface_rede);

    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }

    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }

    return soquete;
}

// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}
 
int protocolo_e_valido(unsigned char* buffer, int tamanho_buffer) {
    if (tamanho_buffer <= 0) { return 0; }

    unsigned char crc_check = crc(buffer, obtem_tamanho(buffer) + OFFSET);

    if (crc_check != obtem_crc(buffer) && buffer[0] == 0x7f) {
        // printf("%x %x %x, %x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        // printf("%x %x %x\n", crc_check, obtem_crc(buffer), obtem_tamanho(buffer));
        // printf("problemas\n");
        return 0;
    }

    return (buffer[0] == 0x7f);
}
 
// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, unsigned char* buffer, int tamanho_buffer) {
    long long comeco = timestamp();
    struct timeval timeout = { .tv_sec = timeoutMillis/1000, .tv_usec = (timeoutMillis%1000) * 1000 };
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        //printf("tamanho = %d; antes: %s\n", obtem_tamanho(buffer), buffer);
        if (bytes_lidos > 0) {
            for (int i = 0; i < TAM_MSG + OFFSET + TAM_EXTRA - 1; i++) {
                if (buffer[i] == 0x81 || buffer[i] == 0x88) {
                    retira(buffer, TAM_MSG + OFFSET + TAM_EXTRA - 1, i);
                }
            }
        }
        //printf("agora: %s\n", buffer);
        if (protocolo_e_valido(buffer, bytes_lidos)) { return bytes_lidos; }
    } while (timestamp() - comeco <= timeoutMillis);
    return -1;
}

void prepara_mensagem(unsigned char msg[], unsigned char marcador, unsigned char tamanho, unsigned char sequencia, unsigned char tipo) {
    msg[0] = marcador;
    unsigned char aux = tamanho << 2;
    aux += sequencia >> 3;
    msg[1] = aux;

    aux = sequencia << 5;
    aux += tipo;
    msg[2] = aux;

    msg[tamanho + OFFSET] = 0;
    if (tipo == NACK) {
        // printf("%d: %x %x %x %x %x\n", tamanho + OFFSET, msg[0], msg[1], msg[2], msg[3], msg[4]);
    }
    msg[tamanho + OFFSET + 1] = '\0';

    unsigned char crc_valor = crc(msg, tamanho + OFFSET);

    msg[tamanho + OFFSET] = crc_valor;

    // crc_valor = crc(msg, tamanho + OFFSET);

    // printf("comparando %x com %x\n", crc_valor, msg[tamanho + OFFSET]);

    if (tipo == NACK || tipo == ACK) {
        // printf("%d: %x %x %x %x\n", tamanho + OFFSET, msg[0], msg[1], msg[2], msg[3]);
    }

    // printf("CRC = %x\n", obtem_crc(msg));
    for(int i = 0; i < TAM_MSG + OFFSET + TAM_EXTRA - 1; i++) {
        if (msg[i] == 0x81 || msg[i] == 0x88) {
            insere(msg, TAM_MSG + OFFSET + TAM_EXTRA - 1, i);
        }
    }
    //printf("agora: %s\n", msg);
}

unsigned char obtem_tamanho(unsigned char msg[]) {
    return msg[1] >> 2;
}

unsigned char obtem_sequencia(unsigned char msg[]) {
    unsigned char aux = msg[1] << 6;
    aux = aux >> 3;
    aux += msg[2] >> 5;
    return aux;
}

unsigned char obtem_tipo(unsigned char msg[]) {
    unsigned char aux = msg[2] << 3;
    aux = aux >> 3;
    return aux;
}

unsigned char obtem_crc(unsigned char msg[]) {
    return msg[obtem_tamanho(msg) + OFFSET];
}