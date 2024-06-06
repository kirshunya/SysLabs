#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>


typedef struct index_s {
    double time_mark;
    uint64_t recno;
} Record;

int blocks;
int threads;
size_t blockSize;
char *filename;

pthread_barrier_t barrier;
pthread_mutex_t mutex;
Record *fileMemoryAddress;
int *blocksStatusMap;

void print() { // функция print без аргументов и возвращаемого значения
    for (int i = 0; i < (int)(blockSize * blocks); i++) {
        printf("%ld\t%lf\t", fileMemoryAddress[i].recno, fileMemoryAddress[i].time_mark);
        if (!((i + 1) % (4))) {
            puts("");
        }
    }
    puts("\n");
}

void printRecordsFromFile(const char* fileName) {
    FILE* file = fopen(fileName, "rb");
    fseek(file, 0, SEEK_END);
    unsigned long amountOfRecords = ftell(file) / sizeof(Record);
    fseek(file, 0, SEEK_SET);

    Record temp_record;

    for (int i = 0; i < (int)amountOfRecords; i++) {
        fread(&temp_record, sizeof(Record), 1, file);
        printf("%lu\t%lf;\n", temp_record.recno, temp_record.time_mark);
        if (!((i + 1) % 8)) {
            puts("");
        }
        if (!((i + 1) % 256)) {
            puts("");
        }
    }
    puts("");
}

int isSubOfTwo(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

int check() {
    if (!isSubOfTwo(blocks) || blocks > 256) {
        return -2;
    } if (threads > 64 || threads < 4 || blocks < threads) { // если threads больше 64, меньше 4 или blocks меньше threads
        return -4;
    }
    return 1;
}

int compareBlocks(const void* a, const void* b) { // функция compareBlocks с аргументами a и b типа const void*
    Record num1 = *((Record*)a); // приведение типа a к типу Record и присваивание num1
    Record num2 = *((Record*)b); // приведение типа b к типу Record и присваивание num2
    if (num1.time_mark < num2.time_mark) // если time_mark num1 меньше time_mark num2
        return -1; // возвращает -1
    else if (num1.time_mark > num2.time_mark) // если time_mark num1 больше time_mark num2
        return 1; // возвращает 1
    else
        return 0; // возвращает 0
}

int selectNextBlock(int iteration) { // функция selectNextBlock с аргументом iteration типа int
    int number = -1; // переменная number типа int

    pthread_mutex_lock(&mutex); // блокировка мьютекса

    for (int i = 0; i < blocks / iteration; i++) { // цикл от 0 до blocks / iteration
        if (!blocksStatusMap[i]) { // если blocksStatusMap[i] равно 0
            blocksStatusMap[i] = 1; // присваивание blocksStatusMap[i] значение 1
            number = i; // присваивание number значение i
            break; // выход из цикла
        }
    }

    pthread_mutex_unlock(&mutex); // разблокировка мьютекса

    return number; // возвращает number
}

void *sort(void *arg) { // функция sort с аргументом arg типа void*
    int argument = *(int*)arg; // приведение типа arg к типу int и присваивание argument
    int number = argument; // присваивание number значение argument
    int iteration = 1; // переменная iteration типа int

    pthread_barrier_wait(&barrier); // ожидание на барьере

    while (number >= 0) { // пока number больше или равно 0
        Record* block = (Record*)malloc(blockSize * sizeof(Record)); // выделение памяти для блока
        for (int i = 0; i < blockSize; i++) { // цикл от 0 до blockSize
            block[i] = fileMemoryAddress[blockSize * number + i]; // заполнение блока
        }

        // сортировка блока
        qsort(block, blockSize, sizeof(Record), compareBlocks);

        for (int i = 0; i < blockSize; i++) { // цикл от 0 до blockSize
            fileMemoryAddress[blockSize * number + i] = block[i]; // запись обратно в память
        }

        number = selectNextBlock(1); // получение номера ещё не сортированного блока
        if (number == -1) { // если number равно -1
            break; // выход из цикла
        }
        free(block); // освобождение памяти блока
    }

    size_t newBlockSize = blockSize; // переменная newBlockSize типа size_t
    pthread_barrier_wait(&barrier); // ожидание на барьере
    for (int i = 0; i < blocks; i++) { // цикл от 0 до blocks
        blocksStatusMap[i] = 0; // присваивание blocksStatusMap[i] значение 0
    }

    while (iteration != blocks) { // пока iteration не равно blocks
        number = argument; // присваивание number значение argument
        iteration *= 2; // увеличение iteration в два раза
        newBlockSize *= 2; // увеличение newBlockSize в два раза

        pthread_barrier_wait(&barrier); // ожидание на барьере

        while (number >= 0) { // пока number больше или равно 0
            number = selectNextBlock(iteration);
            if (number == -1) { // если number равно -1
                break; // выход из цикла
            }
            Record* block = (Record*)calloc(newBlockSize, sizeof(Record)); // выделение памяти для блока
            for (int i = 0; i < newBlockSize; i++) { // цикл от 0 до newBlockSize
                block[i] = fileMemoryAddress[i + newBlockSize * number]; // заполнение блока
            }

            int i = 0; // переменная i типа int
            int j = newBlockSize / 2; // переменная j типа int
            int k = 0; // переменная k типа int
            for (; (i < newBlockSize / 2) && (j < newBlockSize);) { // цикл пока i меньше newBlockSize / 2 и j меньше newBlockSize
                if (block[i].time_mark > block[j].time_mark) { // если time_mark block[i] больше time_mark block[j]
                    fileMemoryAddress[k + newBlockSize * number] = block[j]; // запись в память
                    j++;
                } else {
                    fileMemoryAddress[k + newBlockSize * number] = block[i]; // запись в память
                    i++;
                }
                k++;
            }
            // запись в память оставшейся части старого блока с большими значениями
            while (j < newBlockSize) {
                fileMemoryAddress[k + newBlockSize * number] = block[j++];
                k++;
            }
            while (i < newBlockSize / 2) {
                fileMemoryAddress[k + newBlockSize * number] = block[i++];
                k++;
            }

            free(block); // освобождение памяти блока
        }

        pthread_barrier_wait(&barrier); // ожидание на барьере

        for (int i = 0; i < blocks / iteration; i++) { // цикл от 0 до blocks / iteration
            blocksStatusMap[i] = 0; // присваивание blocksStatusMap[i] значение 0
        }
    }

    pthread_barrier_wait(&barrier); // ожидание на барьере

    pthread_exit(NULL); // завершение потока
}

int main(int argc, char *argv[]) { // главная функция

    if (argc != 4) { // если количество аргументов командной строки не равно 4
        puts("./имя_программы <количество блоков> <количество потоков> <имя файла>"); // вывод сообщения об использовании программы
        return 1; // возвращает 1
    }
    blocks   = atoi(argv[1]); // преобразование строки в число и присваивание blocks
    threads  = atoi(argv[2]); // преобразование строки в число и присваивание threads
    filename = strdup(argv[3]); // копирование строки и присваивание filename

    printRecordsFromFile(filename);

    int status = check(); // вызов функции check и присваивание status
    if (status == -2) { // если status равно -2
        puts("blocks"); // вывод сообщения "blocks"
        puts("\033[31mКоличество блоков не должно быть больше 256\033[0m"); // вывод сообщения о некорректном количестве блоков
        free(filename); // освобождение памяти filename
        return 0; // возвращает 0
    } if (status == -4) { // если status равно -4
        puts("threads"); // вывод сообщения "threads"
        puts("\033[31mКоличество потоков должно лежать в пределах от 4 до 64 и быть не больше, чем количество блоков\033[0m"); // вывод сообщения о некорректном количестве потоков
        free(filename); // освобождение памяти filename
        return 0; // возвращает 0
    }

    pthread_mutex_init(&mutex, NULL); // инициализация мьютекса
    pthread_barrier_init(&barrier, NULL, threads); // инициализация барьера

    int file = open(filename, O_RDWR); // открытие файла в режиме чтения и записи
    if (file < 0) { // если file меньше 0
        fprintf(stderr, "\033[31m");
        perror("Ошибка открытия файла"); // вывод сообщения об ошибке при открытии файла
        fprintf(stderr, "\033[0m");
        free(filename); // освобождение памяти filename
        return EXIT_FAILURE; // возвращает EXIT_FAILURE
    }
    size_t fileSize = lseek(file, 0, SEEK_END); // определение размера файла
    lseek(file, 0, SEEK_SET); // перемещение указателя файла в начало

    fileMemoryAddress = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0); // отображение файла в память
    if (fileMemoryAddress == MAP_FAILED) { // если отображение файла в память не удалось
        fprintf(stderr, "\033[31m");
        perror("Ошибка отображения файла в память"); // вывод сообщения об ошибке при открытии файла
        fprintf(stderr, "\033[0m");
        free(filename); // освобождение памяти filename
        return EXIT_FAILURE; // возвращает EXIT_FAILURE
    }

    blockSize = fileSize / blocks / sizeof(Record); // вычисление размера блока

    blocksStatusMap = (int *)calloc(blocks, sizeof(int)); // выделение памяти для blocksStatusMap
    for (int i = 0; i < threads; i++) { // цикл от 0 до threads
        blocksStatusMap[i] = 1; // присваивание blocksStatusMap[i] значение 1
    }
    for (int i = threads; i < blocks; i++) { // цикл от threads до blocks
        blocksStatusMap[i] = 0; // присваивание blocksStatusMap[i] значение 0
    }

    pthread_t *threadsArray = calloc(threads, sizeof(pthread_t)); // выделение памяти для массива потоков
    int *threadsIds = calloc(threads, sizeof(int)); // выделение памяти для идентификаторов потоков
    for (int i = 0; i < threads; i++) { // цикл от 0 до threads
        threadsIds[i] = i; // присваивание threadsIds[i] значение i
        (void) pthread_create(&threadsArray[i], NULL, sort, &threadsIds[i]); // создание потока
    }

    for (int i = 0; i < threads; i++) { // цикл от 0 до threads
        pthread_join(threadsArray[i], NULL); // ожидание завершения потока
    }

    if (msync(fileMemoryAddress, fileSize, MS_SYNC) == -1) { // синхронизация памяти с файлом
        fprintf(stderr, "\033[31m");
        perror("Ошибка  синхронизации памяти с файлом"); // вывод сообщения об ошибке при открытии файла
        fprintf(stderr, "\033[0m");
        munmap(fileMemoryAddress, fileSize); // отображение файла из памяти
        free(filename); // освобождение памяти filename
        free(threadsArray); // освобождение памяти threadsArray
        free(threadsIds); // освобождение памяти threadsIds
        close(file); // закрытие файла
        return EXIT_FAILURE; // возвращает EXIT_FAILURE
    }

    print(); // вызов функции print

    free(threadsIds); // освобождение памяти threadsIds
    free(threadsArray); // освобождение памяти threadsArray
    munmap(fileMemoryAddress, fileSize); // отображение файла из памяти
    close(file); // закрытие файла
    free(filename); // освобождение памяти filename

    return EXIT_SUCCESS; // возвращает EXIT_SUCCESS
}