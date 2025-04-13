%{
#include "tabela_simbolos.h"
#include "compilador.h"
#include "compilador.tab.h"
%}

%%

[ \t\n]
. { return yytext[0]; } 

%%