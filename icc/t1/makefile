# Gustavo do Prado Silva - 20203942 
# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g -lm
objects = main.o utils.o oper.o sistema.o metodos.o

all: cgSolver

cgSolver: $(objects)
	gcc $(objects) -o cgSolver $(CFLAGS)

main.o: main.c
	gcc -c main.c $(CFLAGS)

utils.o: utils.c
	gcc -c utils.c $(CFLAGS)

oper.o: oper.c
	gcc -c oper.c $(CFLAGS)

sistema.o: sistema.c
	gcc -c sistema.c $(CFLAGS)

metodos.o: metodos.c
	gcc -c metodos.c $(CFLAGS)

#remove objetos e restos do editor
clean:
	rm -rf $(objects) ./*~ ./.vscode ./cgSolver ./*.txt
