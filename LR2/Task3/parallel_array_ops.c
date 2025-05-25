#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

// Функция для чтения массива из файла
int* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Подсчет количества чисел в файле
    int count = 0;
    int temp;
    while (fscanf(file, "%d", &temp) == 1) {
        count++;
    }
    
    // Возврат к началу файла
    rewind(file);
    
    // Выделение памяти под массив
    int* arr = (int*)malloc(count * sizeof(int));
    if (!arr) {
        perror("Ошибка выделения памяти");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    // Чтение чисел в массив
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%d", &arr[i]) != 1) {
            fprintf(stderr, "Ошибка при чтении элемента %d\n", i);
            free(arr);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    
    fclose(file);
    *size = count;
    return arr;
}

// Функция для выполнения операций над массивами с использованием OpenMP
void perform_operations_parallel(const int* arr1, const int* arr2, 
                               double* result_add, double* result_sub, 
                               double* result_mul, double* result_div, 
                               int size, int num_threads) {
    // Устанавливаем количество потоков
    omp_set_num_threads(num_threads);
    
    // Параллельный цикл для выполнения операций
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        // Сложение
        result_add[i] = arr1[i] + arr2[i];
        
        // Вычитание
        result_sub[i] = arr1[i] - arr2[i];
        
        // Умножение
        result_mul[i] = arr1[i] * arr2[i];
        
        // Деление (с проверкой деления на ноль)
        if (arr2[i] != 0) {
            result_div[i] = (double)arr1[i] / arr2[i];
        } else {
            result_div[i] = NAN; // Not a Number при делении на ноль
        }
    }
}

// Функция для проверки результатов (выводим первые 20 элементов в компактном формате)
void check_results(const double* result, int size, const char* operation) {
    printf("%s (первые 20 из %d):\n", operation, size);
    int elements_to_show = (size < 20) ? size : 20;
    for (int i = 0; i < elements_to_show; i++) {
        printf("%7.2f", result[i]);
        if ((i + 1) % 5 == 0 || i == elements_to_show - 1) {
            printf("\n");
        } else {
            printf(" | ");
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 2) {
        printf("Использование: %s <количество_потоков>\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        printf("Количество потоков должно быть положительным числом\n");
        return 1;
    }
    
    double start_time, end_time;
    int size1, size2;
    
    // Замер времени начала выполнения
    start_time = omp_get_wtime();
    
    // Чтение первого массива из файла
    int* array1 = read_array_from_file("array1.txt", &size1);
    
    // Чтение второго массива из файла
    int* array2 = read_array_from_file("array2.txt", &size2);
    
    // Проверка, что массивы одного размера
    if (size1 != size2) {
        fprintf(stderr, "Ошибка: массивы имеют разный размер (%d и %d)\n", size1, size2);
        free(array1);
        free(array2);
        return 1;
    }
    
    int size = size1; // Оба массива одного размера
    
    // Выделяем память под результаты операций
    double* result_add = (double*)malloc(size * sizeof(double));
    double* result_sub = (double*)malloc(size * sizeof(double));
    double* result_mul = (double*)malloc(size * sizeof(double));
    double* result_div = (double*)malloc(size * sizeof(double));
    
    if (!result_add || !result_sub || !result_mul || !result_div) {
        perror("Ошибка выделения памяти для результатов");
        free(array1);
        free(array2);
        free(result_add);
        free(result_sub);
        free(result_mul);
        free(result_div);
        return 1;
    }
    
    // Выполнение операций над массивами с использованием OpenMP
    perform_operations_parallel(array1, array2, result_add, result_sub, 
                              result_mul, result_div, size, num_threads);
    
    // Замер времени окончания выполнения
    end_time = omp_get_wtime();
    
    // Вывод результатов
    double exec_time = end_time - start_time;
    printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ ===\n");
    printf("Количество потоков: %d\n", num_threads);
    printf("Размер массивов: %d элементов\n", size);
    printf("Время выполнения: %.6f секунд\n\n", exec_time);
    
    // Проверка результатов (выводим первые 20 элементов)
    check_results(result_add, size, "Сумма");
    check_results(result_sub, size, "Разность");
    check_results(result_mul, size, "Произведение");
    check_results(result_div, size, "Частное");
    printf("\n");
    
    // Добавляем информацию о скорости работы
    printf("Скорость: %.2f операций/сек\n", size / exec_time);
    
    // Освобождение памяти
    free(array1);
    free(array2);
    free(result_add);
    free(result_sub);
    free(result_mul);
    free(result_div);
    
    return 0;
}
