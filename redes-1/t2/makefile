# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g
DEBUG_CFLAGS = -DDEBUG
objects = carta.o socket.o
test_files = jogo.c 
test_executables = $(test_files:.c=)

all: $(test_executables)

%: %.c $(objects)
	gcc $< $(objects) -o $@ $(CFLAGS)

carta.o: carta.c
	gcc -c carta.c -o $@ $(CFLAGS)

socket.o: socket.c
	gcc -c socket.c -o $@ $(CFLAGS)

# compilação para debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

clean:
	rm -rf $(objects) $(test_objects) ./*~ ./.vscode

purge:
	rm -rf $(test_executables)