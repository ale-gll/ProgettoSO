#include <curses.h> 
#include <unistd.h>
#include "utils.h"
#include "menu.h"
#include "game.h"

int main() {
    initscr();
    noecho();
    curs_set(false);
    start_color();

    //Disegno la finestra di gioco
    int height = 23, width = 70;
    int start_y = (LINES-height)/2 , start_x = (COLS-width)/2;
    WINDOW *win = newwin(height, width, start_y, start_x);
    box(win, 0, 0);
    wrefresh(win);

    init_game_colors(); //Inizializzo i colori

    while(1){
        int choice = start_menu(win);

        if(choice == NEW_GAME) {
            wclear(win);
            wrefresh(win);
            game_loop(win, start_y, start_x);
            break;
        };

        if(choice == HOW_TO_PLAY) {
            wclear(win);
            wrefresh(win);
            print_how_to_play(win, "res/how_to_play.txt");
            while(wgetch(win) != '\n');
            wclear(win);
            box(win, 0, 0);
        }
        
        if(choice == EXIT_GAME) break;  //Esci dal gioco
    }

    close_window(win);
    endwin();
    
    //Ripulisco completamente e riposiziono il cursore in alto
    printf("\033[2J\033[H");    //\033 è l'escaper character ESC
                                //[H è il cursor positioning command
                                //\033[2J cancella lo schermo
    return 0;
}