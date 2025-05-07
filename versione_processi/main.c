#include <curses.h> 
#include <unistd.h>
#include "utils.h"
#include "menu.h"
#include "game.h"

int main() {
    initscr();
    curs_set(false);
    start_color();

    //Disegno la finestra di gioco
    int height = 23, width = 70;
    int start_y = (LINES-height)/2 , start_x = (COLS-width)/2;
    WINDOW *win = newwin(height, width, start_y-1, start_x);
    box(win, 0, 0);
    wrefresh(win);

    init_game_colors(); //Inizializzo i colori

    while(1){
        int choice = start_menu(win);

        if(choice == NEW_GAME) {
            wclear(win);
            game_loop(win, start_y, start_x);
            break;
        };

        if(choice == HOW_TO_PLAY); //Chiama procedura di stampa info gioco
        
        if(choice == EXIT_GAME) break;  //Esci dal gioco
    }

    delwin(win);
    endwin();
    return 0;
}