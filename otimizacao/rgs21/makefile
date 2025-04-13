# Rafael Gonçalves dos Santos - 20211798

CC = gcc -O3 -march=native
CFLAGS = -g
objects = producao.o lp.o

all: producao

producao: $(objects)
	$(CC) $(objects) -o producao $(CFLAGS)  $(LFLAGS)

producao.o: producao.c
	$(CC) -c producao.c $(CFLAGS) $(LFLAGS)

lp.o: lp.c
	$(CC) -c lp.c $(CFLAGS) $(LFLAGS)

#remove objetos e restos do editor
clean:
	rm -rf $(objects) ./*~ ./.vscode 

purge:
	rm -rf ./producao
