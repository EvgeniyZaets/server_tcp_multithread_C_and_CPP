#include "timemanager.h"

/**
 * Устанавливает таймер с заданной функцией и временем в миллисекундах.
 *
 * @param func Функция, которая будет выполнена по истечении времени.
 * @param ms Время в миллисекундах.
 * @return Идентификатор установленного таймера.
 *
 * Эта функция выделяет новый таймер из свободного списка, устанавливает заданную функцию и время и добавляет таймер в активный список.
 * Таймеры в активном списке отсортированы по времени, поэтому новый таймер вставляется в правильное место в списке.
 * Функция возвращает идентификатор установленного таймера.
 *
 * Пример использования:
 * @code
 * unsigned int timerId = timeout([](){
 *     // Код, который нужно выполнить по истечении времени
 * }, 1000); // Устанавливаем таймер на 1 секунду
 * @endcode
 *
 * В этом примере устанавливается таймер на 1 секунду с заданной функцией.
 * Идентификатор таймера сохраняется в переменную timerId для дальнейшего использования.
 */
unsigned int TimerManager::timeout(std::function<void ()> func, int ms)
{
    TimerEvent te;
    te.func = func;
    te.time = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    te.id = next_id++;
    active.push(te);
    timers[te.id] = te;
    return te.id;
}

/**
 * Отменяет таймер с указанным идентификатором.
 *
 * @param id Идентификатор таймера.
 *
 * Эта функция ищет таймер с указанным идентификатором в активном списке и удаляет его.
 * Удаленный таймер добавляется в свободный список для повторного использования.
 * Если таймер с указанным идентификатором не найден, выбрасывается исключение std::runtime_error.
 *
 * Пример использования:
 * @code
 * unsigned int timerId = ...; // Идентификатор устанавливаемого таймера
 * unitimeout(timerId); // Отменяем таймер
 * @endcode
 *
 * В этом примере отменяется таймер с указанным идентификатором.
 * Пользователь должен передать корректный идентификатор таймера для отмены.
 * Если таймер с указанным идентификатором не найден, будет выброшено исключение std::runtime_error.
 */
void TimerManager::unitimeout(unsigned int id)
{
    auto it = timers.find(id);
    if (it == timers.end()) {
        throw std::runtime_error("При выполнении unitimeout указан несуществующий таймер");
    }

    std::priority_queue<TimerEvent, std::vector<TimerEvent>, TimerEventCompare> temp;
    while (!active.empty()) {
        TimerEvent te = active.top();
        active.pop();
        if (te.id != id) {
            temp.push(te);
        }
    }

    active = temp;
    timers.erase(it);
}

/**
 * Выполняет выборку событий ввода-вывода (I/O) с использованием таймеров.
 *
 * @param maxpl Максимальное значение файлового дескриптора.
 * @param re Указатель на набор файловых дескрипторов, доступных для чтения.
 * @param we Указатель на набор файловых дескрипторов, доступных для записи.
 * @param ee Указатель на набор файловых дескрипторов, доступных для исключений.
 * @return Количество готовых файловых дескрипторов, -1 в случае ошибки.
 *
 * Эта функция выполняет выборку событий ввода-вывода (I/O) с использованием таймеров.
 * Она использует функцию select для ожидания готовности файловых дескрипторов к чтению, записи или обнаружению исключений.
 * При наступлении просроченного таймера, функция выполняет соответствующую функцию, связанную с таймером.
 * Функция возвращает количество готовых файловых дескрипторов или -1 в случае ошибки.
 *
 * Пример использования:
 * @code
 * int maxpl = ...; // Максимальное значение файлового дескриптора
 * fd_set re, we, ee;
 * int result = tselect(maxpl, &re, &we, &ee);
 * if (result > 0) {
 *     // Обработка готовых файловых дескрипторов
 * } else if (result == 0) {
 *     // Нет готовых файловых дескрипторов
 * } else {
 *     // Ошибка
 * }
 * @endcode
 *
 * В этом примере функция tselect выполняет выборку событий I/O с использованием таймеров для максимального значения файлового дескриптора maxpl.
 * После выполнения функции, результат сохраняется в переменную result, и затем проверяется для определения наличия готовых файловых дескрипторов или возникновения ошибки.
 * Пользователь может передать наборы файловых дескрипторов re, we и ee для указания интересующих событий.
 */
int TimerManager::tselect(int maxpl, fd_set *re, fd_set *we, fd_set *ee)
{
    fd_set rmask;
    fd_set wmask;
    fd_set emask;
    timeval now;
    timeval tv;
    timeval* tvp = nullptr;
    int n;
    if (re) rmask = *re;
    if (we) wmask = *we;
    if (ee) emask = *ee;
    for (;;) {
        auto now_time = std::chrono::steady_clock::now();
        auto now_sec = std::chrono::time_point_cast<std::chrono::seconds>(now_time);
        auto now_usec = std::chrono::duration_cast<std::chrono::microseconds>(now_time - now_sec);
        now.tv_sec = now_sec.time_since_epoch().count();
        now.tv_usec = now_usec.count();
        while (!active.empty() && active.top().time <= now_time) {
            TimerEvent te = active.top();
            active.pop();
            te.func();
            timers.erase(te.id);
        }
        if (!active.empty()) {
            auto diff = active.top().time - now_time;
            auto diff_sec = std::chrono::duration_cast<std::chrono::seconds>(diff);
            auto diff_usec = std::chrono::duration_cast<std::chrono::microseconds>(diff - diff_sec);
            tv.tv_sec = diff_sec.count();
            tv.tv_usec = diff_usec.count();
            tvp = &tv;
        }
        else if (re == nullptr && we == nullptr && ee == nullptr) {
            return 0;
        }
        else {
            tvp = nullptr;
        }
        n = select(maxpl, re, we, ee, tvp);
        if (n < 0) {
            return -1;
        }
        if (n > 0) {
            return n;
        }
        if (re) *re = rmask;
        if (we) *we = wmask;
        if (ee) *ee = emask;
    }
}
