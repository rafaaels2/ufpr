# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g -std=c99
objects = ppos.o
test_files = teste1.c teste2.c teste3.c
test_executables = $(test_files:.c=)

all: $(test_executables)

%: %.c $(objects)
	gcc $< $(objects) -o $@ $(CFLAGS)

ppos.o: ppos.c
	gcc -c ppos.c -o $@ $(CFLAGS)

# remove objetos e restos do editor
clean:
	rm -rf $(objects) $(test_objects) ./*~ ./.vscode ./*.txt

purge:
	rm -rf $(test_executables)
