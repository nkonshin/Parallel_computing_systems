#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

// Функция для вычисления суммы элементов массива
long long calculate_sum(const int* arr, int size) {
    long long sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int main() {
    int size;
    double start_time, end_time;
    
    // Замер времени начала выполнения
    start_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Чтение массива из файла
    int* array = read_array_from_file("array.txt", &size);
    
    // Вычисление суммы
    long long sum = calculate_sum(array, size);
    
    // Замер времени окончания выполнения
    end_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Вывод результатов
    printf("Размер массива: %d элементов\n", size);
    printf("Сумма элементов: %lld\n", sum);
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    
    // Освобождение памяти
    free(array);
    
    return 0;
}
