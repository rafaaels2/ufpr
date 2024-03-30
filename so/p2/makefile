# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g -std=c99
DEBUG_CFLAGS = -DDEBUG
objects = ppos.o
test_files = teste1.c teste2.c teste3.c
test_executables = $(test_files:.c=)

all: $(test_executables)

%: %.c $(objects)
	gcc $< $(objects) -o $@ $(CFLAGS)

ppos.o: ppos.c
	gcc -c ppos.c -o $@ $(CFLAGS)

# compilação para debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

# remove objetos e restos do editor
clean:
	rm -rf $(objects) $(test_objects) ./*~ ./.vscode

purge:
	rm -rf $(test_executables)
