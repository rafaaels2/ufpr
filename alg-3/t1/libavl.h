#ifndef _LIBavl_t_H
#define _LIBavl_t_H

struct tno {
    int chave;  
    int h;    
    struct tno *esq, *dir, *pai;
};
typedef struct tno noh;

// criar e inicializar o noh com as informacoes necessarias
noh *cria_noh (int chave);

// inclui o noh na folha BST
noh *inclui (int chave, noh *no, int *count);

//busca pelo no com chave == num
noh *busca (int num, noh *no);

//retorna o no com menor chave
noh *min (noh *no);

//void transplant (noh *raiz, noh *rem, noh *suc);
void ajusta_no_pai(noh *no, noh *novo);

noh *exclui (noh *no, noh *raiz);

//busca e remove o no com a chave desejada
noh *remocao(int chave, noh *no);

//imprime os elementos da arvore em ordem
void emordem(noh *n);
#endif
