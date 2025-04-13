%{
#include <stdio.h>
#include <stdlib.h>
#include "tabela_simbolos.h" 
#include "compilador.h"

int yylex();

struct tabela_simbolos *tab_simbolos = NULL;
%}

%%

PROGRAMA:
    /* Vazio */ { tabela_init (&tab_simbolos); printf ("Tabela inicializada!\n"); }
    ;

%%

int main() {
    yyparse();

    return 0;
}

int yyerror(const char *s) {
  fprintf (stderr, "Erro na linha: %s\n",s);

  exit (1);
  //return 0;
}