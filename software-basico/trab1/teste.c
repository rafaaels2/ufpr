#include <stdio.h>
#include "teste.h"

int main (long int argc, char** argv) {
    long int a;

    a = alocaMem (10);

    printf ("# resultado: %ld", a);

    return 0;
}