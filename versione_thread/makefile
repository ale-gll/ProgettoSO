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
frogger_versione_thread: main.o menu.o graphics.o utils.o game.o frog.o croc.o 
	$(CC) $^ $(CFLAGS) -lncurses -o $@

main.o: main.c menu.h utils.h game.h
	$(CC) -c $< -o $@
	
menu.o: menu.c menu.h graphics.h
	$(CC) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) -c $< -o $@

graphics.o: graphics.c graphics.h utils.h 
	$(CC) -c $< -o $@

frog.o: frog_thread.c frog_thread.h utils.h 
	$(CC) -c $< -o $@

croc.o: croc_thread.c croc_thread.h utils.h 
	$(CC) -c $< -o $@


clean:
	rm *.o
