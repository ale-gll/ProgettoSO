
#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#define FROG_WIDTH 5
#define FROG_CROC_HEIGHT 2  //rana e coccodrillo hanno la stessa altezza
#define CROC_WIDTH 12


//Tipi di oggetti dinamici presenti in gioco
typedef enum { OBJ_FROG, OBJ_CROC, OBJ_PROJECTILE, OBJ_GRANADE } ObjectType;

//I coccodrilli e i proiettili/granate si muovono in un unica direzione a parte la rana
typedef enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_UNKNOWN } ObjectDirection;


//Info di un oggetto dinamico
typedef struct {
    int y, x;
    int type;
    int direction;  //DIR_UNKNOWN nel caso della rana
} Object;

//Definisce dove inizia e finisce una tana (indica anche se è stata occupata)
typedef struct {
    bool is_occupied;
    int start_x, end_x;
} Burrow;

//Statistiche di gioco
typedef struct {
    int score;      //Punteggio 
    int lives;      //Vite della rana
    int time;       //Tempo in sec
} Stats;


typedef enum { MSG_UPDATE_POS, MSG_FIRE, MSG_SET_FROG } MessageType;

//Struttura di un messaggio inviabile al gestore della grafica
typedef struct {
    pthread_t tid;
    int msg_type;
    Object obj;
    int stream_index;      //Indice in Streams (coccodrilli, proiettili, granate)
} Message;

void set_message(Message *m, pthread_t tid, int msg_type, Object *obj, int *stream_index);


#define BUFFER_CAPACITY 16

typedef struct BufferNode {
    Message msg;
    struct BufferNode *next;
} BufferNode;

typedef struct {
    BufferNode *head;  // prossimo da leggere
    BufferNode *tail;  // ultimo inserito
    int count;
    int capacity;

    pthread_mutex_t mutex;
} SharedBuffer;


// Inizializzazione buffer
void buffer_init(SharedBuffer *buf, int capacity);

// Distruzione buffer
void buffer_destroy(SharedBuffer *buf);

// Inserimento (produttore)
bool produce_try(SharedBuffer *buf, Message *msg);

// Prelievo (consumatore)
bool consume_try(SharedBuffer *buf, Message *msg);


#endif