#include <stdio.h>
#include <stdlib.h>
#include "libavl.h"

noh *cria_noh (int chave) {
    noh *novo_noh = malloc (sizeof(noh));
    if (novo_noh == NULL)
        return 0;
    
    novo_noh->chave = chave;
    novo_noh->h = 0;
    novo_noh->pai = NULL;
    novo_noh->esq = NULL;
    novo_noh->dir = NULL;

    return novo_noh;
}

noh *inclui (int chave, noh *no, int *count) {
    if (no == NULL) {
        no = cria_noh(chave);
        return no;
    }

    if (chave < no->chave) {
        no->esq = inclui(chave, no->esq, count);
        no->esq->pai = no;
        *count += 1;
    }
    else {
        no->dir = inclui(chave, no->dir, count);
        no->dir->pai = no;
        *count += 1;
    }

    if(*count > no->h)
        no->h++;
    
    return no;
}

noh *busca (int num, noh *no) {
    if(no == NULL) 
        return NULL;

    if (num < no->chave)
        return busca (num, no->esq);
    else if(num > no->chave)
        return busca (num, no->dir);
    else
        return no;
}

noh *min (noh *no) {
    while(no->esq != NULL)
        no = no->esq;
    return no;
}

void ajusta_no_pai(noh *no, noh *novo) {
    if (no->pai != NULL) {
        if (no->pai->esq == no)
            no->pai->esq = novo;
        else
            no->pai->dir = novo;
        if (novo != NULL)
            novo->pai = no->pai;
    } 
}

void ajusta_altura(noh *no) {
    while(no->pai != NULL) {
        if(no == no->pai->esq) {
            if(no->pai->dir == NULL) {
                no->pai->h -= 1;
            }
            else if(no->pai->dir->h < no->h){
                no->pai->h -= 1;
            }
            else if((no->pai->dir->h == no->h) && ((no->pai->h - no->h) > 1)) {
                no->pai->h -= 1;
            }
        }
        else if(no->pai->esq == NULL) {
            no->pai->h -= 1;
        }
        else if(no->pai->esq->h < no->h){
            no->pai->h -= 1;
        }
        else if((no->pai->esq->h == no->h) && ((no->pai->h - no->h) > 1)) {
            no->pai->h -= 1;
        }
        no = no->pai;
    }
}

noh *exclui (noh *no, noh *raiz) {
    noh *s, *nova_raiz = raiz, *aux = no;

    if (no->esq == NULL) {
        ajusta_altura(no);
        ajusta_no_pai(no, no->dir);
        free (aux);
    } 
    else {
        if (no->dir == NULL) {
            ajusta_altura(no);
            ajusta_no_pai(no, no->esq);
            free (aux);
        }
        else {
            s = min(no->dir);
            ajusta_altura(no);
            ajusta_no_pai(s, s->dir);
            s->esq = no->esq;
            s->dir = no->dir;
            ajusta_no_pai(no, s);
            if (no == raiz) {
                s->h = no->h;
                nova_raiz = s;
            }
            else
                s->h = no->pai->h - 1;
            free (aux); 
        }
    }
    return nova_raiz;
 }

void emordem(noh *n) {
    if(n == NULL)
        return;
    emordem(n->esq);
    printf("%d - %d\n", n->chave, n->h);
    emordem(n->dir);
}


