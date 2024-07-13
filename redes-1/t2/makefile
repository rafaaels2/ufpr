# Rafael Gonçalves dos Santos - 20211798

CFLAGS = -Wall -g
DEBUG_CFLAGS = -DDEBUG
objects = jogo.o 
test_files = jogo.c 
test_executables = $(test_files:.c=)

all: $(test_executables)

%: %.c $(objects)
	gcc $< $(objects) -o $@ $(CFLAGS)

# compilação para debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

clean:
	rm -rf $(objects) $(test_objects) ./*~ ./.vscode

purge:
	rm -rf $(test_executables)