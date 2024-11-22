#include <windows.h>
#include <process.h>
#include <iostream>
#include <random>
#include <string>

#define COUNT_LIMIT 1000  // Limita maximă până la care se numără
#define SEMAPHORE_INITIAL 1  // Numărul maxim de thread-uri care pot accesa resursa
#define SEMAPHORE_MAX 1

HANDLE hMutex;  // Mutex pentru acces exclusiv la secțiunea critică
HANDLE hSemaphore;  // Semafor pentru sincronizare între thread-uri
HANDLE hEvent;  // Eveniment pentru sincronizarea între etape

int* sharedMemory = nullptr;  // Pointer la memoria partajată

void SafePrint(const std::string& message) {
    WaitForSingleObject(hMutex, INFINITE);  // Obține acces la consola
    std::cout << message << std::endl;
    ReleaseMutex(hMutex);  // Eliberează accesul la consola
}

void CountingThread(void* param) {
    int count = 0;

    while (count < COUNT_LIMIT) {
        // Accesul exclusiv la secțiunea critică cu WaitForSingleObject și Mutex
        WaitForSingleObject(hMutex, INFINITE);
        count = *sharedMemory;  // Citește numărul curent din memoria partajată

        // Aruncă banul: cât timp cade pe 2, incrementăm valoarea
        std::random_device rd;
        std::uniform_int_distribution<int> dist(1, 2);
        while (dist(rd) == 2 && count < COUNT_LIMIT) {
            count++;
            *sharedMemory = count;  // Scrie în memoria partajată
            SafePrint("Count: " + std::to_string(count));
        }
        ReleaseMutex(hMutex);  // Eliberează mutexul

        // Notificăm celălalt thread să continue (semafor)
        ReleaseSemaphore(hSemaphore, 1, nullptr);

        // Așteaptă semaforul înainte de a relua bucla
        WaitForSingleObject(hSemaphore, INFINITE);
    }
}

int main() {
    // Creare obiect de memorie partajată
    HANDLE hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(int), L"SharedMemoryObject");
    sharedMemory = (int*)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    *sharedMemory = 0;  // Inițializarea la 0

    // Creare mutex pentru acces exclusiv la secțiuni critice
    hMutex = CreateMutex(nullptr, FALSE, nullptr);

    // Creare semafor
    hSemaphore = CreateSemaphore(nullptr, SEMAPHORE_INITIAL, SEMAPHORE_MAX, nullptr);

    // Creare eveniment (setat manual)
    hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    // Lansare două thread-uri
    _beginthread(CountingThread, 0, nullptr);
    _beginthread(CountingThread, 0, nullptr);

    // Așteaptă thread-urile să termine
    WaitForSingleObject(hEvent, INFINITE);  // Așteaptă notificare de finalizare
    SetEvent(hEvent);  // Setează evenimentul de terminare

    // Curățare resurse
    UnmapViewOfFile(sharedMemory);
    CloseHandle(hMapping);
    CloseHandle(hMutex);
    CloseHandle(hSemaphore);
    CloseHandle(hEvent);

    return 0;
}
