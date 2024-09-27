#ifndef _LIhash_t_H
#define _LIBhash_t_H

// estrutura do numero existente na tabela hash
typedef struct {
    int num;        // numero exitente na posicao da tabela
    int ocupado;    // verifica se exite um valor nessa posicao da tabela
} tabela;

int hash_T1 (int num);

int hash_T2 (int num);

int busca_inclusiva (tabela T1[], tabela T2[], int num);

int busca_reclusiva (tabela T[], int num);

void inclui (tabela T[], int num, int posicao); 

void remover (tabela T[], int posicao);

#endif
