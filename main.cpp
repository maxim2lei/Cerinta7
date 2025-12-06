#include <cstdio>
#include <windows.h>
#include <process.h>

CRITICAL_SECTION g_cs; //mutex global
CONDITION_VARIABLE g_cvWhite; //condvar pt WHITE
CONDITION_VARIABLE g_cvBlack; //condvar pt BLACK

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
    EnterCriticalSection(&g_cs); //lock
    waitingWhite++;

    while (!canEnterWhite()) {
        SleepConditionVariableCS(&g_cvWhite,&g_cs,INFINITE);
        //elibereaza g_cs cat timp asteapta, dupa trezire il recupereaza
    }

    waitingWhite--;
    activeWhite++;

    LeaveCriticalSection(&g_cs); //unlock
}
void enterBlack() {
    EnterCriticalSection(&g_cs); //lock
    waitingBlack++;

    while (!canEnterBlack()) {
        SleepConditionVariableCS(&g_cvBlack,&g_cs,INFINITE);
    }

    waitingBlack--;
    activeBlack++;

    LeaveCriticalSection(&g_cs); //unlock
}

void leaveWhite() {
    EnterCriticalSection(&g_cs);
    activeWhite--;

    if (activeWhite == 0) {
        if (waitingBlack > 0) {
            turn = BLACK;
            WakeAllConditionVariable(&g_cvBlack);
        } else if (waitingWhite > 0) {
            turn = WHITE;
            WakeAllConditionVariable(&g_cvWhite);
        } else {
            turn = NONE;
        }
    }
    LeaveCriticalSection(&g_cs);
}
void leaveBlack() {
    EnterCriticalSection(&g_cs);
    activeBlack--;

    if (activeBlack == 0) {
        if (waitingWhite > 0) {
            turn = WHITE;
            WakeAllConditionVariable(&g_cvWhite);
        } else if (waitingBlack > 0) {
            turn = BLACK;
            WakeAllConditionVariable(&g_cvBlack);
        } else {
            turn = NONE;
        }
    }
    LeaveCriticalSection(&g_cs);
}

void useResource(Color color, int id, int iter) {
    const char* colName = (color == WHITE ? "WHITE" : "BLACK");
    printf("[T%d-%s] folosesc resursa (iter %d)\n", id, colName, iter);
    Sleep(200 + (rand() % 300)); // simulare lucru
}

unsigned __stdcall threadFunc(void* arg) {
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

        Sleep(100 + (rand() % 400)); // pauza ca sa se amestece cererile
    }

    return 0;
}

int main() {
    InitializeCriticalSection(&g_cs);
    InitializeConditionVariable(&g_cvWhite);
    InitializeConditionVariable(&g_cvBlack);

    constexpr int NUM_WHITE = 5;
    constexpr int NUM_BLACK = 5;
    constexpr int TOTAL_THREADS = NUM_WHITE + NUM_BLACK;

    HANDLE handles[TOTAL_THREADS];
    ThreadArgs args[TOTAL_THREADS];

    int idx = 0;

    for (int i = 0; i < NUM_WHITE; ++i,++idx) {
        args[idx].id = idx;
        args[idx].color = WHITE;
        handles[idx] = (HANDLE)_beginthreadex(nullptr,0,&threadFunc,&args[idx],0,nullptr);
    }
    for (int i = 0; i < NUM_BLACK; ++i,++idx) {
        args[idx].id = idx;
        args[idx].color = BLACK;
        handles[idx] = (HANDLE)_beginthreadex(nullptr,0,&threadFunc,&args[idx],0,nullptr);
    }

    WaitForMultipleObjects(TOTAL_THREADS,handles,TRUE,INFINITE);

    for (int i = 0; i < TOTAL_THREADS; ++i) {
        CloseHandle(handles[i]);
    }

    DeleteCriticalSection(&g_cs);

    printf("Toate firele au terminat.\n");
    return 0;
}



