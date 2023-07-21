#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define THREAD_POOL_SIZE 5

//создаем массив потоков
pthread_t threads[THREAD_POOL_SIZE];

// Структура для хранения задачи
typedef struct {
    void *(*function)(void *);
    void *argument;
} thread_task_t;

// Мьютекс и условная переменная для синхронизации потоков
pthread_mutex_t lock;
pthread_cond_t notify;

// Очередь задач
thread_task_t task_queue[THREAD_POOL_SIZE];
int task_queue_size = 0;
int task_queue_front = 0;
int task_queue_rear = 0;

// Инициализация мьютекса и условной переменной
void thread_pool_init() {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&notify, NULL);
}

// Добавление задачи в очередь
void thread_pool_add_task(void *(*function)(void *), void *argument) {
    //блокируем добавление в очередь
    pthread_mutex_lock(&lock);

    // Если очередь полна, поток ожидает сигнала на условной переменной
    while (task_queue_size == THREAD_POOL_SIZE) {
        pthread_cond_wait(&notify, &lock);
    }

    // Добавление задачи в очередь
    task_queue[task_queue_rear].function = function;
    task_queue[task_queue_rear].argument = argument;
    task_queue_rear = (task_queue_rear + 1) % THREAD_POOL_SIZE;
    ++task_queue_size;

    // Отправка сигнала на условной переменной
    pthread_cond_signal(&notify);
    //разблокируем добавление в очередь
    pthread_mutex_unlock(&lock);
}

// Получение следующей задачи из очереди
thread_task_t thread_pool_get_task() {
    thread_task_t task = task_queue[task_queue_front];
    task_queue_front = (task_queue_front + 1) % THREAD_POOL_SIZE;
    --task_queue_size;
    return task;
}

// Функция потока
void *thread_function(void *arg) {
    while (1) {
        // Блокировка мьютекса
        pthread_mutex_lock(&lock);

        // Если очередь пуста, поток ожидает сигнала на условной переменной
        while (task_queue_size == 0) {
            pthread_cond_wait(&notify, &lock);
        }
    
        // Получение следующей задачи из очереди
        thread_task_t task = thread_pool_get_task();

        // Отправка сигнала на условной переменной
        pthread_cond_signal(&notify);
        pthread_mutex_unlock(&lock);

        // Выполнение задачи
        (*(task.function))(task.argument);
    }
}

//функция обработки клиента
void *client_handler(void *arg) {
    //сокет клиента
    int client_socket_fd = *(int *)arg;

    //буфер для инфомации
    char buffer[1024];
    //количество байт
    int read_size;

    //цикл пока сервер получает данные от клиента не выходим
    while ((read_size = recv(client_socket_fd, buffer, sizeof(buffer), 0)) > 0) {
        //добавляем в конце полученного буфера конец строки
        buffer[read_size] = '\0';
        //выводим буфер
        printf("Received message from client: %s\n", buffer);
        
        //если буфер вернул ноль выходим из цикла while
        if (strcmp(buffer, "0") == 0) {
            break;
        }

        //буфер для возврата клиенту
        char response[1024];
        snprintf(response, sizeof(response), "You sent me: %s", buffer);

        //пишем клиенту
        if (send(client_socket_fd, response, strlen(response), 0) < 0) {
            perror("Error sending response to client");
            break;
        }
    }

    //клиент вернул ошибку
    if (read_size == -1) {
        perror("Error receiving message from client");
    }
    printf("Client disconnected\n");

    //закрываем сокет
    close(client_socket_fd);

    return NULL;
}

//функция работы сервера
void *server_function(int) {
    //инициализируем сокет сервера
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    //проверям его на ошибки 
    if (server_socket_fd == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    //создаем структуру адреса
    struct sockaddr_in server_address;
    //обнуляем поля структуры
    memset(&server_address, 0, sizeof(server_address));
    //заполняем структуру адреса
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(9000);

    //биндим сокет
    if (bind(server_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    //слушаем сокет
    if (listen(server_socket_fd, 5) == -1) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", ntohs(server_address.sin_port));

    while (1) {
        //присоединяем клиента к серверу
        int client_socket_fd = accept(server_socket_fd, NULL, NULL);

        //проверям на обишки
        if (client_socket_fd == -1) {
            perror("Error accepting client connection");
            continue;
        }
        //клиент подсоединился
        printf("Client connected\n");

        //создаем указатель на сокет клиента
        int *arg = (int*)malloc(sizeof(*arg));
        *arg = client_socket_fd;
        
        //передаем в отдельный поток сокет клиента для обработки
        thread_pool_add_task(client_handler, arg);
    }
}

int main() {
	thread_pool_init();

	for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
		pthread_create(&threads[i], NULL, thread_function, NULL);
	}

    //запускаем функцию сервера
    thread_pool_add_task(server_function, NULL);
    
    for (int i = 0; i < THREAD_POOL_SIZE; ++i) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            
        }
    }

    //pthread_join(threads[0], NULL);

	return 0;
}
