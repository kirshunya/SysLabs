#include "functions.h"

void printQueueInfo(Queue* queue) {
    printf(YELLOW);
    printf("Queue status:\n");
    printf("Queue current size: %d\n", queue->currentSize);
    printf("Queue max capacity: %d\n", queue->maxCapacity);
    printf("Queue count removed messages: %d\n", queue->removedMessages);
    printf("Queue count added messages: %d\n", queue->addedMessages);
    printf(WHITE);
}

void menu(int sharedMemoryID, Queue* queue) {
    int symbol;
    printf("Choose option:\n"
           "1 - create producer\n"
           "2 - delete producer\n"
           "3 - create consumer\n"
           "4 - delete consumer\n"
           "5 - display queue info\n"
           "q - quit\n");
    while (1) {
        symbol = getchar();
        getchar();
        switch (symbol) {
            case '1':
                createProducer();
                break;
            case '2':
                deleteProducer();
                break;
            case '3':
                createConsumer();
                break;
            case '4':
                deleteConsumer();
                break;
            case '5':
                printQueueInfo(queue);
                break;
            case 'q':
                deleteAllProducers();
                deleteAllConsumers();
                munmap(queue, SHM_SIZE);
                close(sharedMemoryID);
                shm_unlink(SHARED_MEMORY_NAME);
                return;
            default:
                break;
        }
    }
}

/**
 * @brief Обработчик сигнала SIGUSR2 для завершения процесса производителя.
 *
 * @param signal Номер сигнала.
 * @param info Информация о сигнале.
 * @param ptr Дополнительные данные.
 */
void handleSIGUSR2(int signal, siginfo_t *info, void *ptr) {
    runProducer = 0; // Установка флага завершения процесса производителя
}

/**
 * @brief Обработчик сигнала SIGUSR1 для завершения процесса потребителя.
 *
 * @param signal Номер сигнала.
 * @param info Информация о сигнале.
 * @param ptr Дополнительные данные.
 */
void handleSIGUSR1(int signal, siginfo_t *info, void *ptr) {
    runConsumer = 0; // Установка флага завершения процесса потребителя
}

/**
 * @brief Инициализация обработчиков сигналов SIGUSR1 и SIGUSR2.
 */
void initializeHandler(void) {
    struct sigaction act;

    // Установка обработчика для SIGUSR1
    act.sa_sigaction = &handleSIGUSR1;
    act.sa_flags = SA_SIGINFO | SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGUSR1, &act, NULL);

    // Установка обработчика для SIGUSR2
    act.sa_sigaction = &handleSIGUSR2;
    sigaction(SIGUSR2, &act, NULL);
}

void cleanResources(void) {
    shm_unlink(SHARED_MEMORY_NAME); // Удаление разделяемой памяти
    sem_unlink(SEM_FREE_SPACE); // Удаление семафора для свободного места
    sem_unlink(SEM_FILLED_SPACE); // Удаление семафора для заполненного места
    sem_unlink(MUTEX); // Удаление семафора для мьютекса
}

void closeAllSemaphores(sem_t *freeSpaceSemaphore,
                     sem_t *filledSpaceSemaphore,
                     sem_t *mutex) {
    sem_close(freeSpaceSemaphore); // Закрытие семафора свободного места
    sem_close(filledSpaceSemaphore); // Закрытие семафора заполненного места
    sem_close(mutex); // Закрытие семафора мьютекса
}

void initializeAllSemaphores(sem_t **freeSpaceSemaphore,
                          sem_t **filledSpaceSemaphore,
                          sem_t **mutex) {
    // Инициализация семафора свободного места
    *freeSpaceSemaphore =
            sem_open(SEM_FREE_SPACE, O_CREAT | O_EXCL, 0666, QUEUE_CAPACITY);
    if (*freeSpaceSemaphore == SEM_FAILED) {
        perror("sem_open_init_free");
        exit(EXIT_FAILURE);
    }

    // Инициализация семафора заполненного места
    *filledSpaceSemaphore = sem_open(SEM_FILLED_SPACE, O_CREAT | O_EXCL, 0666, 0);
    if (*filledSpaceSemaphore == SEM_FAILED) {
        perror("sem_open_init_filled");
        exit(EXIT_FAILURE);
    }

    // Инициализация семафора мьютекса
    *mutex = sem_open(MUTEX, O_CREAT | O_EXCL, 0666, 1);
    if (*mutex == SEM_FAILED) {
        perror("sem_open_init_mutex");
        exit(EXIT_FAILURE);
    }
}

void initializeSharedMemory(int *sharedMemoryId, Queue **queue, char setSizeFlag) {
    // Создание или открытие разделяемой памяти
    *sharedMemoryId = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (*sharedMemoryId == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Установка размера разделяемой памяти, если задан флаг
    if (setSizeFlag) {
        ftruncate(*sharedMemoryId, SHM_SIZE);
    }

    // Отображение разделяемой памяти в адресное пространство процесса
    *queue = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                  MAP_SHARED, *sharedMemoryId, 0);
    if (*queue == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
}

void openSemaphore(const char name[], sem_t** semaphore, int value) {
    // Открытие семафора с указанным именем и начальным значением
    *semaphore = sem_open(name, value);
    if (*semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
}
