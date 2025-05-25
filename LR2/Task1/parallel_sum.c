#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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

// Функция для вычисления суммы элементов массива с использованием OpenMP
long long calculate_sum_parallel(const int* arr, int size, int num_threads) {
    long long sum = 0;
    
    // Установка количества потоков
    omp_set_num_threads(num_threads);
    
    // Параллельное вычисление суммы с использованием reduction
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    
    return sum;
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
    
    int size;
    double start_time, end_time;
    
    // Замер времени начала выполнения
    start_time = omp_get_wtime();
    
    // Чтение массива из файла
    int* array = read_array_from_file("array.txt", &size);
    
    // Вычисление суммы с использованием OpenMP
    long long sum = calculate_sum_parallel(array, size, num_threads);
    
    // Замер времени окончания выполнения
    end_time = omp_get_wtime();
    
    // Вывод результатов
    printf("Количество потоков: %d\n", num_threads);
    printf("Размер массива: %d элементов\n", size);
    printf("Сумма элементов: %lld\n", sum);
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    
    // Освобождение памяти
    free(array);
    
    return 0;
}
