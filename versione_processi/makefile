CC = gcc	# Nome del compilatore

# -Wall		-> mostra i warning principali
# -Wextra	-> mostra warning addizionali (utile)
# -Wpedantic	-> segnala codice non conforme allo standard C
CFLAGS = -Wall -Wextra -Wpedantic

# $(CC)	-> nome del compilatore (gcc)
# $^	-> prende tutte le dipendenze (i file oggetto)
# $(CFLAGS) -> opzioni usate
# $@	-> nome del target
# $<	-> prima dipendenza del target
frogger_versione_processi: main.o menu.o utils.o
	$(CC) $^ $(CFLAGS) -lncurses -o $@

main.o: main.c menu.h utils.h
	$(CC) -c $< -o $@
	
menu.o: menu.c menu.h utils.h
	$(CC) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) -c $< -o $@

clean:
	rm *.o
