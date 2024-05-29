#include "message.h"
#include "functions.h"

Message *createMessage(void) {
    srand(time(NULL));

    // Генерация случайного размера сообщения (от 1 до 254 байт, выровненного до 4 байт)
    size_t size;
    do {
        size = rand() % 255; // 0 - 254
    } while (size == 0); // Повторять, пока не получим ненулевой размер
    size = ((size + 3) / 4) * 4; // Выровнить размер до ближайшего кратного 4

    // Выделение памяти под структуру сообщения
    Message *message = (Message *)malloc(sizeof(Message));

    // Заполнение сообщения данными и вычисление хэша
    message->size = size;
    message->hash = 0;
    for (size_t i = 0; i < message->size; i++) {
        int upperCase = rand() % 2; // Случайно выбираем регистр буквы (верхний или нижний)
        if (upperCase) {
            message->data[i] = 'A' + rand() % 26; // Генерация случайной заглавной буквы
        } else {
            message->data[i] = 'a' + rand() % 26; // Генерация случайной строчной буквы
        }
        message->hash = (message->hash * 17) + message->data[i]; // Вычисление хэша методом аккумулятивного хеширования
    }

    message->type = 0; // Установка типа сообщения (здесь 0)

    return message; // Возвращаем указатель на созданное сообщение
}

void printMessage(Message *message) {
    printf("_______________MESSAGE_______________\n");
    printf("Type: %u\n", message->type); // Вывод типа сообщения
    printf("Hash: %u\n", message->hash); // Вывод хэша сообщения
    printf("Size: %u\n", message->size); // Вывод размера сообщения
    printf("Data:\n");
    
    // Вывод данных сообщения с переносами строки
    for (size_t i = 0; i < message->size; i++) {
        printf("%c", message->data[i]); // Вывод очередного символа данных
        if(i % 37 == 0 && i != 0) { // Разбивка вывода каждые 37 символов
            printf("\n");
        }
    }
    
    printf("\n_____________________________________\n");
}