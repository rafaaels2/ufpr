# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g -std=c99
objects = testafila.o queue.o

all: queue

queue: $(objects)
	gcc $(objects) -o queue $(CFLAGS)

testafila.o: testafila.c
	gcc -c testafila.c $(CFLAGS)

queue.o: queue.c
	gcc -c queue.c $(CFLAGS)

# remove objetos e restos do editor
clean:
	rm -rf $(objects) ./*~ ./.vscode ./queue ./*.txt
