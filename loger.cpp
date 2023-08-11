#include "loger.h"

/**
 * @file loger.cpp
 * @brief Функции для логирования с использованием переменного числа аргументов и вывода текущей даты и времени.
 */

std::mutex mtxLog; ///< Мьютекс для синхронизации доступа к файлу логирования.
std::mutex mtxTime; ///< Мьютекс для синхронизации вывода текущего времени.
std::condition_variable condLog; ///< Условная переменная для оповещения о завершении записи в файл логирования.

/**
 * @brief Функция для логирования с использованием переменного числа аргументов.
 *
 * @param type Тип лога (DEBUG, ERROR, INFO, WARNING).
 * @param er Код ошибки (если есть).
 * @param msg Форматированное сообщение для лога.
 * @param ... Дополнительные аргументы (переменное число параметров).
 */
void loger(LogType type, int er, const char* msg,...){

    std::unique_lock<std::mutex> lock(mtxLog);

    va_list ap; 		            //создаем указатель на неопределенное колличество параметров
    va_start(ap, msg); 	            //связываем с первым необязательным параметром
    flog = fopen(pNameFile,"a+");   //Открывает текстовый файл логирования для чтения и записи (если файл не существует то он будет создан)
    if(flog)                        //проверяем открыт ли файл
    {
    print_time(flog);
    fprintf(flog," %s |", program_name);     //печатаем название программы
    switch (type)
    {
    case DEBUG:
        fprintf(flog, " DEBUG   |");           //печатаем уровень ошибки
        break;
    case ERROR:
        fprintf(flog, " ERROR   |");           //печатаем уровень ошибки

        break;
    case INFO:
        fprintf(flog, " INFO    |");           //печатаем уровень ошибки
        break;
    case WARNING:
        fprintf(flog, " WARNING |");           //печатаем уровень ошибки
        break;
    default:
        break;
    }
    vfprintf(flog, msg, ap);                //печатаем сообщение
    va_end(ap);
    if (er)                                                 //если ошибка имеет номер печатаем
        fprintf(flog, "| %s (%d)", strerror(er), er);
    }
    fprintf(flog," \n");                          //переводим на новую строку
    fclose(flog);                           //закрываем файл
    flog=0;                                 //обнуляем переменную
    lock.unlock();
    condLog.notify_all();
};

/**
 * @brief Функция для вывода текущей даты и времени.
 *
 * @param fd Указатель на файл, в который будет записано время.
 */
void print_time(FILE *fd)
{
    // переменные для хранения компонентов даты и времени
    int hours, minutes, seconds, day, month, year;
    // `time_t` - арифметический тип времени
    time_t now;

    // Получить текущее время
    // `time()` возвращает текущее время системы как значение `time_t`
    time(&now);

    // localtime преобразует значение `time_t` в календарное время и
    // возвращает указатель на структуру `tm` с ее членами
    // заполняется соответствующими значениями
    struct tm *local = localtime(&now);

    hours = local->tm_hour;         // получаем часы с полуночи (0-23)
    minutes = local->tm_min;        // получить минуты, прошедшие после часа (0-59)
    seconds = local->tm_sec;        // получаем секунды, прошедшие через минуту (0-59)

    day = local->tm_mday;            // получить день месяца (от 1 до 31)
    month = local->tm_mon + 1;      // получить месяц года (от 0 до 11)
    year = local->tm_year + 1900;   // получаем год с 1900

    std::unique_lock<std::mutex> lock(mtxTime);

    // печатаем местное время
    fprintf(fd,"Дата %i-%i-%i Время %02d:%02d:%02d |",day,month,year, hours, minutes, seconds);

    lock.unlock();
    condLog.notify_all();
};
