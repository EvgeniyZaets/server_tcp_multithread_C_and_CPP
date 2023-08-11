#include "safecout.h"
#include <cstdarg>

void createTable(const std::vector<std::vector<std::string>>& data, SafeCout& safeCout) {
    std::vector<size_t> columnWidths;

    // Находим максимальную длину каждого столбца
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (i >= columnWidths.size()) {
                columnWidths.push_back(row[i].length());
            } else if (row[i].length() > columnWidths[i]) {
                columnWidths[i] = row[i].length();
            }
        }
    }

    // Выводим верхнюю границу таблицы
    safeCout << "+";
    for (const auto& width : columnWidths) {
        safeCout << std::string(width + 2, '-') << "+";
    }
    safeCout << std::string(columnWidths.back() + 2, '-') << "+" << std::endl;

    // Выводим заголовки таблицы
    for (size_t i = 0; i < data[0].size(); ++i) {
        safeCout << "| " << std::left << std::setw(columnWidths[i]) << data[0][i] << " ";
    }
    safeCout << "|" << std::endl;

    // Выводим разделитель столбцов
    safeCout << "+";
    for (const auto& width : columnWidths) {
        safeCout << std::string(width + 2, '-') << "+";
    }
    safeCout << std::string(columnWidths.back() + 2, '-') << "+" << std::endl;

    // Выводим данные в таблице
    for (size_t i = 1; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            safeCout << "| " << std::left << std::setw(columnWidths[j]) << data[i][j] << " ";
        }
        safeCout << "|" << std::endl;
    }

    // Выводим нижнюю границу таблицы
    safeCout << "+";
    for (const auto& width : columnWidths) {
        safeCout << std::string(width + 2, '-') << "+";
    }
    safeCout << std::string(columnWidths.back() + 2, '-') << "+" << std::endl;
}

void safePrintf(const char* format, ...) {
    std::lock_guard<std::mutex> lock(mtxPrintF); // блокируем мьютекс
    va_list args;
    va_start(args, format);
    std::vprintf(format, args); // вызываем printf для вывода с переменным числом аргументов
    va_end(args);
}