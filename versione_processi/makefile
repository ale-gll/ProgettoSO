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
frogger_versione_processi: main.o menu.o utils.o frog_process.o  croc_process.o  projectile_process.o game.o graphics.o  
	$(CC) $^ $(CFLAGS) -lncurses -o $@

main.o: main.c menu.h utils.h game.h
	$(CC) -c $< -o $@
	
menu.o: menu.c menu.h graphics.h
	$(CC) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) -c $< -o $@

frog_process.o: frog_process.c frog_process.h utils.h
	$(CC) -c $< -o $@

game.o: game.c game.h graphics.h frog_process.h  croc_process.h  projectile_process.h
	$(CC) -c $< -o $@

graphics.o: graphics.c graphics.h  
	$(CC) -c $< -o $@

croc_process.o: croc_process.c croc_process.h utils.h
	$(CC) -c $< -o $@

projectile_process: projectile_process.c projectile_process.h utils.h
	$(CC) -c $< -o $@

clean:
	rm *.o
