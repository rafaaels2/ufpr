# Rafael Gonçalves dos Santos - 20211798
# Andre Gauer Thomal - 20203923

CC = gcc -O3 -march=native
CFLAGS = -g
objects = clausulas.o comb.o
mergesat = cd MergeSAT && make && cd .. && cp MergeSAT/build/release/bin/mergesat .
mergesatclean = cd MergeSAT && make clean && cd ..

all: clausulas
	$(mergesat)

clausulas: $(objects)
	$(CC) $(objects) -o clausulas $(CFLAGS)  $(LFLAGS)

clausulas.o: clausulas.c
	$(CC) -c clausulas.c $(CFLAGS) $(LFLAGS)

comb.o: comb.c
	$(CC) -c comb.c $(CFLAGS) $(LFLAGS)

#remove objetos e restos do editor
clean:
	rm -rf $(objects) ./*~ ./.vscode ./clausulas.txt
	$(mergesatclean)

purge: 
	rm -rf ./clausulas ./mergesat