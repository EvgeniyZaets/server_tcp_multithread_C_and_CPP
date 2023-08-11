#pragma once
#ifndef SERVERX_H
#define SERVERX_H
#include "libtcp.h"
#include "loger.h"
#include "timemanager.h"
#include "safecout.h"

/**
 * @file
 * @brief Значения и константы для системы передачи сообщений.
 *
 * T0: Частота отправки сообщений в OUT.
 * T1: Время ожидания первого подтверждения ACK (в мс).
 * TSTAT: Интервал вывода статистики (в мс).
 * T2: Интервал между пульсами по второму соединению (в мс).
 * MRSZ: Максимальное число неподтвержденных сообщений для переключения на другую сеть.
 * BUF: Максимальный размер буфера сообщения.
 * ACKSZ: Размер пакета подтверждения ACK.
 * COOKIESZ: Размер области номера пакета.
 * ACK: Символ подтверждения ACK.
 */
#define T0          20
#define T1          1000
#define TSTAT       5000
#define T2          5000
#define MRSZ        128
#define BUF         3700
#define ACKSZ       (sizeof(u_int32_t)+1)
#define COOKIESZ    sizeof(u_int32_t)
#define ACK         0x6

/**
 * @brief Перечисление для результатов операций.
 */
enum result{
    Break,      ///< Прервать операцию.
    Continue,   ///< Продолжить операцию.
    Error,      ///< Ошибка.
    True,       ///< Истина.
    False,      ///< Ложь.
    None        ///< Нет результата.
};

/**
 * @brief Структура пакета сообщения.
 *
 * Эта структура представляет собой пакет сообщения, содержащий длину признака и данных, признак сообщения и само сообщение.
 * Поле `len` используется для указания длины признака и данных, и удаленный хост может использовать это поле для разбиения на отдельные записи.
 * Поле `cookie` является 32-битным порядковым номером сообщения.
 * Поле `buf` представляет собой буфер для хранения сообщения.
 */
typedef struct
{
    u_int32_t len;      ///< Длина признака и данных - удаленный хост может использовать это поле для разбиения на отдельные записи.
    u_int32_t cookie;   ///< Признак сообщения - это 32-битный порядковый номер сообщения.
    char buf[BUF];      ///< Сообщение.
} packet_t;

/**
 * @brief Структура посылаемого сообщения.
 *
 * Эта структура содержит указатель на сохраненное сообщение (пакет) и идентификатор таймера.
 * Поле `pkt` является указателем на сохраненное сообщение типа `packet_t`.
 * Поле `id` представляет собой идентификатор таймера, который используется для ретрансмиссии.
 * Поле `start` представляет собой структуру `timeval` для контроля времени.
 */
typedef struct
{
    packet_t pkt;               ///< Указатель на сохраненное сообщение.
    unsigned int id;            ///< Идентификатор таймера - т.е. таймера ретрансмиссии.
    struct timeval start;       ///< Время для контроля.
} msgrec_t;

/**
 * @class XOUT
 * @brief Класс для отправки данных в сокет и приема подтверждений.
 */
class XOUT {
protected:
    SOCKET fdxout;                  ///< Сокет.
    msgrec_t mr[MRSZ];              ///< Массив содержащий неподтвержденные сообщения OUT.
    u_int32_t cn_ack_out;           ///< Получено ack в OUT.
    unsigned int mid;               ///< Переменная для признака сообщения для OUT.
    struct timeval end;             ///< Переменная - конец передачи сообщения OUT.
    int cnt;                        ///< Остаток сообщения размера для OUT.
    unsigned int msgid;             ///< Отправлено всего сообщений клиенту OUT.
    char ack[ACKSZ];                ///< Сообщение ACK для OUT.
    long sr_time;                   ///< Время от отправки сообщения до получения акта общее среднее.
    long time_out;                  ///< Время от отправки сообщения до получения акта общее, microsec.
    long time_out_max;              ///< Время от отправки сообщения до получения акта общее максимальное, microsec.
    TimerManager TimeXout;          ///< Менеджер таймеров для tselect timeout

    msgrec_t *getfreerec(void);                          ///< Поиск свободной записи.
    msgrec_t *findmsgrec(u_int32_t);                     ///< Поиск записи с заданным признаком сообщения.
    void freemsgrec( msgrec_t* );                        ///< Освобождение сообщения.
    void drop(msgrec_t* );                               ///< Отбрасывание записи.
    void setTimeOut(long time){time_out += time;}                                ///< Инициализация поля новым значением времени.
    void setTimeOutMax(long time){time_out_max = std::max(time,time_out_max);}   ///< Инициализация полем максимума новым значением времени.

public:
    XOUT(SOCKET fd):fdxout(fd){
        for (msgrec_t* data_out = mr; data_out<mr+MRSZ;data_out++)
            data_out->pkt.len=-1;
    };
    ~XOUT(){std::cout << "XOUT delete " << std::endl;};
    result SendMsg(char msg[],int size);   ///< Отправка сообщений.
    result RecvAck();                      ///< Считывание подтверждений.
};

/**
 * @class XIN
 * @brief Класс для приема данных и отправки подтверждений.
 */
class XIN {
protected:
    u_int32_t cn_in;                    ///< Получено всего сообщений от клиента IN.
    u_int32_t cn_ack_in;                ///< Отправлено ack в IN.
    u_int32_t Lost;                     ///< Количество неподтвержденных ack в IN.
    SOCKET fdxin;                       ///< Сокет.
    int size_in;                        ///< Размер массива по умолчанию равен BUF.
public:
    XIN(SOCKET fd):fdxin(fd),size_in(BUF){};
    ~XIN(){std::cout << "XIN delete " << std::endl;};
    char msgin[BUF];                     ///< Массив последнего сообщения.
    result Eсho();                       ///< Чтение сообщений и отправка подтверждений.
};

/**
 * @class ServerXOI
 * @brief Класс сервера с асинхронным вводом XIN и выводом XOUT.
 */
class ServerXOI : public XOUT, XIN {
protected:
    SOCKET server_out;                  ///< Сокет сервера OUT.
    SOCKET server_in;                   ///< Сокет сервера IN.
    SOCKET client_out;                  ///< Сокет клиента OUT.
    SOCKET client_in;                   ///< Сокет клиента IN.
    const char* port_out;               ///< Порт OUT.
    const char* port_in;                ///< Порт IN.
    const char* server_name;            ///< Имя сервера.
    const char* ip;
    bool sock_out_flag = false;         ///< Флаг подключения порта OUT.
    bool sock_in_flag = false;          ///< Флаг подключения порта IN.
public:
    ServerXOI(const char* ip, const char* out, const char* in, const char* name,SafeCout* s)
        :XOUT(-1),XIN(-1),port_out(out),port_in(in),server_name(name),ip(ip),scout(s) {Start();};

    ~ServerXOI(){std::cout << "ServerXOI delete " << std::endl;Stop();};

    SafeCout* scout;                    ///< Безопасный вывод сообщений.
    void Start();                       ///< Старт сервера.
    void Stop(){
        close(server_out);
        close(server_in);
        *scout << "закрыли сокеты сервера: " << server_name << "\n";
    };                                   ///< Стоп сервера.
    void init_cnt();                    ///< Обнуление счетчиков.
    void ClientHandler();               ///< Функция обработки клиента - отправка и прием сообщений.
    void statistic();                   ///< Вывод статистики.
    void AcceptToServer(int server, bool* sock_flag, int* client); ///< Подключение клиента к сокету.

};

#endif // SERVERX_H
