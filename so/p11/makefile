# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g
DEBUG_CFLAGS = -DDEBUG
objects = ppos_core.o queue.o
test_files = pingpong-prodcons.c
test_executables = $(test_files:.c=)

all: $(test_executables)

%: %.c $(objects)
	gcc $< $(objects) -o $@ $(CFLAGS)

ppos_core.o: ppos_core.c
	gcc -c ppos_core.c -o $@ $(CFLAGS)

queue.o: queue.c
	gcc -c queue.c -o $@ $(CFLAGS)

# compilação para debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

# remove objetos e restos do editor
clean:
	rm -rf $(objects) $(test_objects) ./*~ ./.vscode

purge:
	rm -rf $(test_executables)
