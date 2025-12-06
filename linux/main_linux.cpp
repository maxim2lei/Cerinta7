#include <pthread.h>
#include <unistd.h>   // usleep
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cvWhite = PTHREAD_COND_INITIALIZER;
pthread_cond_t g_cvBlack = PTHREAD_COND_INITIALIZER;

int activeWhite  = 0;
int activeBlack  = 0;
int waitingWhite = 0;
int waitingBlack = 0;

enum Color { WHITE, BLACK, NONE };
Color turn = NONE;

struct ThreadArgs {
    int id;
    Color color;
};

bool canEnterWhite() {
    if (activeBlack > 0) return false;
    if (waitingBlack == 0) return true;
    if (turn == BLACK) return false;
    return true;
}
bool canEnterBlack() {
    if (activeWhite > 0) return false;
    if (waitingWhite == 0) return true;
    if (turn == WHITE) return false;
    return true;
}

void enterWhite() {
    pthread_mutex_lock(&g_mutex); //lock
    waitingWhite++;

    while (!canEnterWhite()) {
        pthread_cond_wait(&g_cvWhite,&g_mutex);
        //elibereaza g_cs cat timp asteapta, dupa trezire il recupereaza
    }

    waitingWhite--;
    activeWhite++;

    pthread_mutex_unlock(&g_mutex); //unlock
}
void enterBlack() {
    pthread_mutex_lock(&g_mutex); //lock
    waitingBlack++;

    while (!canEnterBlack()) {
        pthread_cond_wait(&g_cvBlack,&g_mutex);
    }

    waitingBlack--;
    activeBlack++;

    pthread_mutex_unlock(&g_mutex); //unlock
}

void leaveWhite() {
    pthread_mutex_lock(&g_mutex);
    activeWhite--;

    if (activeWhite == 0) {
        if (waitingBlack > 0) {
            turn = BLACK;
            pthread_cond_broadcast(&g_cvBlack);
        } else if (waitingWhite > 0) {
            turn = WHITE;
            pthread_cond_broadcast(&g_cvWhite);
        } else {
            turn = NONE;
        }
    }
    pthread_mutex_unlock(&g_mutex);
}
void leaveBlack() {
    pthread_mutex_lock(&g_mutex);
    activeBlack--;

    if (activeBlack == 0) {
        if (waitingWhite > 0) {
            turn = WHITE;
            pthread_cond_broadcast(&g_cvWhite);
        } else if (waitingBlack > 0) {
            turn = BLACK;
            pthread_cond_broadcast(&g_cvBlack);
        } else {
            turn = NONE;
        }
    }
    pthread_mutex_unlock(&g_mutex);
}

void useResource(Color color, int id, int iter) {
    const char* colName = (color == WHITE ? "WHITE" : "BLACK");
    printf("[T%d-%s] folosesc resursa (iter %d)\n", id, colName, iter);
    usleep(200000 + (rand() % 300000));  // simulare lucru
}

void* threadFunc(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    Color color = args->color;
    int   id    = args->id;

    const char* colName = (color == WHITE ? "WHITE" : "BLACK");

    for (int i = 0; i < 5; ++i) {
        printf("[T%d-%s] cere resursa (iter %d)\n", id, colName, i);

        if (color == WHITE) {
            enterWhite();
        } else {
            enterBlack();
        }

        printf("[T%d-%s] A INTRAT in resursa (iter %d)\n", id, colName, i);
        useResource(color, id, i);
        printf("[T%d-%s] IESE din resursa (iter %d)\n", id, colName, i);

        if (color == WHITE) {
            leaveWhite();
        } else {
            leaveBlack();
        }

        usleep(100000 + (rand() % 400000)); // pauza ca sa se amestece cererile
    }

    return 0;
}

int main() {
    constexpr int NUM_WHITE = 5;
    constexpr int NUM_BLACK = 5;
    constexpr int TOTAL_THREADS = NUM_WHITE + NUM_BLACK;

    pthread_t threads[TOTAL_THREADS];
    ThreadArgs args[TOTAL_THREADS];

    int idx = 0;

    for (int i = 0; i < NUM_WHITE; ++i,++idx) {
        args[idx].id = idx;
        args[idx].color = WHITE;
        pthread_create(&threads[idx], nullptr, &threadFunc, &args[idx]);
    }
    for (int i = 0; i < NUM_BLACK; ++i,++idx) {
        args[idx].id = idx;
        args[idx].color = BLACK;
        pthread_create(&threads[idx], nullptr, &threadFunc, &args[idx]);
    }

    for (int i = 0; i < TOTAL_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
    }

    printf("Toate firele au terminat.\n");
    return 0;
}



