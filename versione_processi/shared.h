/*
    Questo file contiene principalmente Strutture dati condivise
*/

#ifndef SHARED_H
#define SHARED_H

//Dimensioni delle sprite
#define FROG_WIDTH 5
#define FROG_CROC_HEIGHT 2  //rana e coccodrillo hanno la stessa altezza
#define CROC_WIDTH 10


//Tipi di oggetti dinamici presenti in gioco
typedef enum { OBJ_FROG, OBJ_CROC, OBJ_PROJECTILE, OBJ_GRANADE } ObjectType;

//I coccodrilli e i proiettili/granate si muovono in un unica direzione a parte la rana
typedef enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_UNKNOWN } ObjectDirection;

typedef enum { 
    MSG_UPDATE_POS,         //Aggiorna posizione oggetto
    MSG_FIRE,               //Spara un proiettile
    MSG_TOGGLE_ON_CROC,       //La rana si è spostata su/da un coccodrillo
    MSG_SET_FROG,           //Imposta i parametri della rana
    MSG_KILL                //Uccide il processo
} MessageType;

//Info di un oggetto dinamico
typedef struct {
    int pid;
    int y, x;
    int type;
    int direction;  //DIR_UNKNOWN nel caso della rana
} Object;

//Struttura di un messaggio inviabile al gestore della grafica
typedef struct {
    int msg_type;
    Object obj;
    int stream_index;   //Indice in Streams
    int stream_objs_index;  //Indice nell'array di Objects di uno Stream
} Message;

void set_message(Message *m, int msg_type, Object obj, int *stream_index, int *stream_objs_index);

#endif