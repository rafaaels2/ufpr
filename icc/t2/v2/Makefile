# Gustavo do Prado Silva - 20203942 
# Rafael Gonçalves dos Santos - 20211798

CC = gcc -DLIKWID_PERFMON -I${LIKWID_HOME}/include -O3 -march=native -mavx
CFLAGS = -g -lm
objects = main.o utils.o oper.o sistema.o metodos.o
LFLAGS = -L${LIKWID_HOME}/lib -llikwid

all: cgSolver

cgSolver: $(objects)
	$(CC) $(objects) -o cgSolver $(CFLAGS)  $(LFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS) $(LFLAGS)

utils.o: utils.c
	$(CC) -c utils.c $(CFLAGS) $(LFLAGS)

oper.o: oper.c
	$(CC) -c oper.c $(CFLAGS) $(LFLAGS)

sistema.o: sistema.c
	$(CC) -c sistema.c $(CFLAGS) $(LFLAGS)

metodos.o: metodos.c
	$(CC) -c metodos.c $(CFLAGS) $(LFLAGS)

#remove objetos e restos do editor
clean:
	rm -rf $(objects) ./*~ ./.vscode ./cgSolver ./*.txt
