# Отчет по Лабораторной Работе №2: Создание Потоков
## Обзор Задачи
Целью данной лабораторной работы было создание консольного процесса, использующего многопоточность для выполнения операций над массивом целых чисел. Программа должна состоять из трех потоков: main, min_max и average.

# Функциональные Требования
## Поток main

Поток main выполняет следующие действия:

Создание и ввод массива: Запрашивает у пользователя размер массива и его элементы.
Создание потоков: Запускает потоки min_max и average.
Ожидание завершения: Ожидает завершения работы обоих дочерних потоков.
Изменение массива: Заменяет минимальный и максимальный элементы массива на вычисленное среднее арифметическое значение.
Вывод результатов: Печатает измененный массив на консоль.
Завершение работы: Корректно завершает выполнение программы, освобождая дескрипторы потоков.

## Поток min_max

Поток min_max отвечает за:

Поиск минимума и максимума: Находит минимальный и максимальный элементы в массиве.
Вывод: Печатает найденные значения на консоль.
Искусственная задержка: Включает задержку в 7 миллисекунд после каждого сравнения элементов (Sleep(7)).

## Поток average

Поток average отвечает за:

Расчет среднего значения: Вычисляет среднее арифметическое всех элементов массива.
Вывод: Печатает среднее значение на консоль.
Искусственная задержка: Включает задержку в 12 миллисекунд после каждой операции суммирования (Sleep(12)).
Реализация
Программа реализована на языке C++ с использованием Windows API для работы с потоками.

# Глобальные Переменные

Для обмена данными между потоками используются следующие глобальные переменные:

array_size: Размер массива.
arr: Указатель на динамически выделенный массив целых чисел.
sum: Сумма элементов массива.
min_element, max_element: Минимальный и максимальный элементы.
min_index, max_index: Индексы минимального и максимального элементов.
Создание и Синхронизация Потоков

Потоки создаются с помощью функции CreateThread().
Для ожидания завершения дочерних потоков в main используется функция WaitForSingleObject() с параметром INFINITE, что гарантирует выполнение операций в main только после того, как min_max и average завершат свои вычисления.
Функция Sleep() используется для имитации задержек в потоках min_max и average, как того требуют условия задачи.
Структура Кода

# Вывод
Данная лабораторная работа успешно продемонстрировала концепцию многопоточности в C++ с использованием Windows API. Были реализованы параллельные вычисления минимального, максимального и среднего значений элементов массива в отдельных потоках, что позволило выполнить требования задачи по распределению задач и синхронизации потоков. Использование глобальных переменных для обмена данными между потоками упростило реализацию, но в более сложных сценариях потребовало бы использования явных механизмов синхронизации для предотвращения состояний гонки.
