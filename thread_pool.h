#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <queue>
#include <condition_variable>
#include <time.h>
#include <functional>


/**
 * Класс ThreadPool представляет пул потоков для выполнения задач.
 *
 * Пул потоков позволяет эффективно распределять задачи между несколькими потоками.
 * Класс ThreadPool обеспечивает управление потоками, добавление задач в очередь и их выполнение.
 */
class ThreadPool{
public:
    /**
      * Конструктор класса ThreadPool.
      *
      * @param size Количество потоков в пуле.
      */
    ThreadPool(size_t);

    /**
      * Добавляет задачу в очередь пула потоков.
      *
      * @tparam F Тип функции задачи.
      * @tparam Args Типы аргументов функции задачи.
      * @param f Функция задачи.
      * @param args Аргументы функции задачи.
      */

    /**
     * Добавляет задачу в очередь пула потоков.
     *
     * @tparam F Тип функции задачи.
     * @tparam Args Типы аргументов функции задачи.
     * @param f Функция задачи.
     * @param args Аргументы функции задачи.
     * @throws std::runtime_error Если добавление в очередь остановлено.
     *
     * Данная функция принимает функцию задачи `f` и ее аргументы `args`.
     * Она создает упакованную задачу `task` с помощью `std::packaged_task`,
     * используя `std::bind` для связывания функции `f` с аргументами `args`.
     * Затем создается лямбда-функция `func`, которая вызывает `task`.
     * Задача `func` помещается в очередь `tasks` с помощью `emplace`.
     * Если флаг `stop` установлен в `true`, выбрасывается исключение `std::runtime_error`.
     * В конце, вызывается `cond.notify_one()`, чтобы уведомить один из потоков пула о наличии новой задачи.
     */
    template <class F, class... Args>
    auto add_task(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto res = task->get_future();
        auto func = [task](){(*task)();};
        {
            std::unique_lock<std::mutex> lock(mtx);
            if(stop)
                throw std::runtime_error("добавление в очередь остановлено");
            tasks.emplace(std::make_shared<std::function<void()>>(func));
        }
        cond.notify_one();
        return res;
    }

    /**
      * Деструктор класса ThreadPool.
      *
      * Останавливает выполнение всех потоков пула и освобождает ресурсы.
      */
    ~ThreadPool();
private:
    std::vector<std::thread> workers; /**< Вектор потоков пула. */
    std::queue<std::shared_ptr<std::function<void()>>> tasks; /**< Очередь задач. */
    std::mutex mtx; /**< Мьютекс для синхронизации доступа к очереди задач. */
    std::condition_variable cond; /**< Условная переменная для сигнализации о наличии новой задачи. */
    std::atomic<bool> stop; /**< Флаг для остановки выполнения задач. */
};

#endif // THREAD_POOL_H
