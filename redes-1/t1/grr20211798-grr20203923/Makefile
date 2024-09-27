nome_s=envia
nome_r=recebe
cflags= -std=c99



all: $(nome_s) $(nome_r)

$(nome_s): main-envia.o mensagem.o dados.o
	gcc -o $(nome_s) main-envia.o mensagem.o dados.o $(cflags)

$(nome_r): main-recebe.o mensagem.o dados.o
	gcc -o $(nome_r) main-recebe.o mensagem.o dados.o $(cflags)

main-envia.o: main-envia.c
	gcc -c main-envia.c $(cflags)

main-recebe.o: main-recebe.c
	gcc -c main-recebe.c $(cflags)

mensagem.o: mensagem.c mensagem.h
	gcc -c mensagem.c $(cflags)

dados.o: dados.c dados.h
	gcc -c dados.c $(cflags)

clean:
	rm -f *.o $(nome_s) *.o $(nome_r)

purge: clean
	rm -f $(nome_s) $(nome_r)