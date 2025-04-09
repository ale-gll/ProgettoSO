#include <curses.h> 
#include <unistd.h>
#include "utils.h"
#include "menu.h"

int main() {
    initscr();
    curs_set(false);
    start_color();

    //Disegno la finestra di gioco
    int height = 25, width = 60;
    WINDOW *game_win = newwin(height, width, (LINES-height)/2, (COLS-width)/2);
    box(game_win, 0, 0);
    wrefresh(game_win);

    init_game_colors(); //Inizializzo i colori

    while(1){
        int choice = start_menu(game_win);
        
        if(choice == NEW_GAME); //Chiama funzione che gestisce il loop di gioco

        if(choice == HOW_TO_PLAY); //Chiama procedura di stampa info gioco
        
        if(choice == EXIT_GAME) break;  //Esci dal gioco
    }


    delwin(game_win);
    endwin();
    return 0;
}