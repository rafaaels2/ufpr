parametrosCompilacao=-Wall -g -lm
nomePrograma=myht

all: $(nomePrograma)

$(nomePrograma): main.o hash.o
	gcc -o $(nomePrograma) main.o hash.o $(parametrosCompilacao) 

main.o: main.c
	gcc -c main.c $(parametrosCompilacao) 

hash.o: hash.h hash.c
	gcc -c hash.c $(parametrosCompilacao) 

clean:
	-rm -f *~ *.o

purge:
	-rm -f $(nomePrograma)
