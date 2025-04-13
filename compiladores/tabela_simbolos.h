#ifndef TABELA_SIMBOLOS_H
#define TABELA_SIMBOLOS_H

typedef enum tipo_e {INTEIRO, REAL, BOOLEAN, CHAR} tipo_e;
typedef enum tipo_simb_e {VARIAVEL, CONSTANTE, PROCEDURE, FUNCAO} tipo_simb_e;

struct simbolo {
    char *nome;
    tipo_e tipo;
    tipo_simb_e tipo_simb;
    int escopo;
    struct lista_params *params;
};

struct lista_params {
    tipo_e tipo;
    struct lista_params *proximo;
};

struct tabela_simbolos {
    struct simbolo *simb;
    struct tabela_simbolos *proximo;
};

struct tabela_simbolos **tabela_init (struct tabela_simbolos **tabela);

struct simbolo *novo_simbolo (char *nome, tipo_e tipo, tipo_simb_e tipo_simb, int escopo);

#endif
