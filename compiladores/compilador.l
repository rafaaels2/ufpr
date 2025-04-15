%{
#include "tabela_simbolos.h"
#include "compilador.h"
#include "compilador.tab.h"
%}

id [a-zA-Z][a-zA-Z1-9]*
digitos [0-9]+
num [0-9]+(.[0-9]+)?([Ee][+-]?{digitos})?
%%

/* enunciados */
"program"   { return PROGRAM; }
"var"       { return VAR; }
"begin"     { return BEGIN_TOKEN; }
"end"       { return END; }
"function"  { return FUNCTION; }
"procedure" { return PROCEDURE; }

/* tipos */
"integer" { return INTEIRO; }
"real"    { return REAL; }

/* parenteses */
"(" { return ABRE_PARENTESES; }
")" { return FECHA_PARENTESES; }

/* pontuacao */
";" { return PONTO_VIRGULA; }
":" { return DOIS_PONTOS; }
"," { return VIRGULA; }

/* comparativos */
"if"    { return IF; }
"then"  { return THEN; }
"else"  { return ELSE; }
"while" { return WHILE; }
"do"    { return DO; }

/* atribuicao */
":=" { return OPERADOR_ATRIBUICAO; }

/* comparacao */
"="  { yylval.lexema = strdup ("=")  return OPERADOR_RELACIONAL; }
"<>" { yylval.lexema = strdup ("<>") return OPERADOR_RELACIONAL; }
">"  { yylval.lexema = strdup (">")  return OPERADOR_RELACIONAL; }
">=" { yylval.lexema = strdup (">=") return OPERADOR_RELACIONAL; }
"<"  { yylval.lexema = strdup ("<")  return OPERADOR_RELACIONAL; }
"<=" { yylval.lexema = strdup ("<=") return OPERADOR_RELACIONAL; }

/* conjuncao */
"or" { return OR; }

/* matematicos */
"+"   { return MAIS; }
"-"   { return MENOS; }
"*"   { yylval.lexema = strdup ("*")   return OPERADOR_MULTIPLICATIVO; }
"/"   { yylval.lexema = strdup ("/")   return OPERADOR_MULTIPLICATIVO; }
"div" { yylval.lexema = strdup ("div") return OPERADOR_MULTIPLICATIVO; }
"mod" { yylval.lexema = strdup ("mod") return OPERADOR_MULTIPLICATIVO; }
"and" { yylval.lexema = strdup ("and") return OPERADOR_MULTIPLICATIVO; }

{num} {
    yylval.lexema = strdup (yytext);
    return NUM;
}

{id} {
    yylval.lexema = strdup (yytext);
    return ID;
}

\n {}
[ \n] {}

%%