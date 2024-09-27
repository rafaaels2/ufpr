//Gustavo do Prado Silva - 20203942 
//Rafael Gonçalves dos Santos - 20211798

#ifndef __UTILS_H__
#define __UTILS_H__

#define ERRINPUT -1
#define ERROUTPUT -2
#define ERRALLOC -3
#define ERRNUMER -4

#define ABS(num)  ((num) < 0.0 ? -(num) : (num))

double timestamp(void);

double generateRandomA( unsigned int i, unsigned int j, unsigned int k );

double generateRandomB( unsigned int k );

#endif