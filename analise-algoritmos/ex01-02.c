#include <stdio.h>
#include <math.h>

int algoritmo(int n) {
    if (n <= 1) 
        return n;

    return 5 * algoritmo(n-1) - 6 * algoritmo(n-2);
}

int teorema(int n) {
    return pow(3, n) - pow(2, n);
}

int main () {
    int n = 7;

    printf ("Algoritmo: %d\n", algoritmo (n));
    printf ("Teorema: %d\n", teorema (n));

    return 0;
}

