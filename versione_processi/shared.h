/*
    Questo file contiene Strutture dati condivise, costanti, enum, ...
*/

#ifndef SHARED_H
#define SHARED_H

//Dimensioni delle sprite
#define FROG_LENGTH 5
#define FROG_CROC_HEIGHT 2  //rana e coccodrillo hanno la stessa altezza
#define CROC_WIDTH 10


//Sprite dei personaggi
char *frog_sprite[] = {
    "[^v^]",
    ";[ ];"
};

//Sprite del coccodrillo volto a sx
char *croc_sprite_sx[] = {
    " /ç'~~~~~|",
    "[^_<___>_|"
};

//Sprite del coccodrillo volto a dx
char *croc_sprite_dx[] = {
    "|~~~~~'ç\\ ",
    "|_<___>_^]"
};


//Tipi di oggetti dinamici presenti in gioco
typedef enum {
    OBJ_FROG, OBJ_CROC, OBJ_PROJECTILE, OBJ_GRANADE
} ObjectType;

typedef enum {
    
} MessageType;

//Oggetto dinamico
typedef struct 
{
    int x;
    int y;
    ObjectType type;
} Object;



#endif