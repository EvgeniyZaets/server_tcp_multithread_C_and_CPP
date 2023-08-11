#pragma once
#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H
#include <stdexcept>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <queue>
#include <sys/select.h>
#include <sys/time.h>

/** 
 * Класс TimerManager управляет таймерами и событиями. 
 * 
 * Класс TimerManager предоставляет функциональность для установки таймеров и выполнения заданных функций по истечении установленного времени. 
 * Он использует структуру TimerEvent для представления каждого таймера, содержащую информацию о времени, функции и идентификаторе таймера. 
 * Класс TimerManager имеет приватные поля active и free_list для хранения активных и свободных таймеров соответственно. 
 * Также он имеет приватное поле next_id для генерации уникальных идентификаторов таймеров. 
 * 
 * Функции класса TimerManager: 
 * - unsigned int timeout(std::function<void()> func, int ms): Устанавливает таймер с заданной функцией и временем в миллисекундах. 
 * - void unitimeout(unsigned int id): Отменяет таймер с указанным идентификатором. 
 * - int tselect(int maxpl, fd_set *re, fd_set *we, fd_set *ee): Выполняет выборку событий ввода-вывода (I/O) с использованием таймеров. 
 * 
 * Пример использования: 
 * @code 
 * TimerManager timerManager; 
 * unsigned int timerId = timerManager.timeout([](){ 
 *     // Код, который нужно выполнить по истечении времени 
 * }, 1000); // Устанавливаем таймер на 1 секунду 
 * 
 * // Отменяем таймер 
 * timerManager.unitimeout(timerId); 
 * 
 * // Выполняем выборку событий I/O с использованием таймеров 
 * int maxpl = ...; // Максимальное значение файлового дескриптора 
 * fd_set re, we, ee; 
 * int result = timerManager.tselect(maxpl, &re, &we, &ee); 
 * @endcode 
 * 
 * В этом примере создается объект класса TimerManager и устанавливается таймер на 1 секунду с заданной функцией. 
 * Затем таймер отменяется с использованием идентификатора таймера. 
 * Наконец, выполняется выборка событий I/O с использованием таймеров. 
 */ 

class TimerManager {
private:
    typedef struct TimerEvent {
        std::chrono::steady_clock::time_point time;
        std::function<void()> func;
        unsigned int id;
    } TimerEvent;

    struct TimerEventCompare {
        bool operator()(const TimerEvent& te1, const TimerEvent& te2) {
            return te1.time > te2.time;
        }
    };

    std::priority_queue<TimerEvent, std::vector<TimerEvent>, TimerEventCompare> active;
    std::unordered_map<unsigned int, TimerEvent> timers;
    unsigned int next_id;

public:
    TimerManager() : next_id(1) {}
    ~TimerManager() {}

    /** 
     * Устанавливает таймер с заданной функцией и временем в миллисекундах. 
     * 
     * @param func Функция, которая будет выполнена по истечении времени. 
     * @param ms Время в миллисекундах, через которое должна быть выполнена функция. 
     * @return Идентификатор установленного таймера. 
     */
    unsigned int timeout(std::function<void()> func, int ms);

    /** 
     * Отменяет таймер с указанным идентификатором. 
     * 
     * @param id Идентификатор таймера, который нужно отменить. 
     * @return Отсутствует. 
     */ 
    void unitimeout(unsigned int id);

    /** 
     * Выполняет выборку событий ввода-вывода (I/O) с использованием таймеров. 
     * 
     * @param maxpl Максимальное значение файлового дескриптора. 
     * @param re Маска набора дескрипторов на чтение. 
     * @param we Маска набора дескрипторов на запись. 
     * @param ee Маска набора дескрипторов на ошибку. 
     * @return Результат выполнения функции tselect(). 
     */ 
    int tselect(int maxpl, fd_set* re, fd_set* we, fd_set* ee);
};


#endif // TIMEMANAGER_H
