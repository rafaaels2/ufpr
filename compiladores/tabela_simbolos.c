#include <stdio.h>

#include "tabela_simbolos.h"
#include "compilador.h"

struct tabela_simbolos **tabela_init (struct tabela_simbolos **tabela) {
    *tabela = NULL;

    return tabela;
}

struct simbolo *novo_simbolo (char *nome, tipo_e tipo, tipo_simb_e tipo_simb, int escopo) {
    struct simbolo *novo = malloc (sizeof (struct simbolo));
    if (!novo)
        return NULL;

    novo -> nome = nome;
    novo -> tipo = tipo;
    novo -> tipo_simb = tipo_simb;
    novo -> escopo = escopo;
    novo -> params = NULL;

    return novo;
}


