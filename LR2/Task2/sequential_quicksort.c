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

// Функция для обмена элементов массива
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Функция разделения массива для быстрой сортировки
int partition(int arr[], int low, int high) {
    int pivot = arr[high];  // Выбираем последний элемент как опорный
    int i = (low - 1);     // Индекс меньшего элемента

    for (int j = low; j <= high - 1; j++) {
        // Если текущий элемент меньше или равен опорному
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

// Последовательная быстрая сортировка
void quick_sort_sequential(int arr[], int low, int high) {
    if (low < high) {
        // pi - индекс разделения
        int pi = partition(arr, low, high);

        // Рекурсивно сортируем элементы до и после раздела
        quick_sort_sequential(arr, low, pi - 1);
        quick_sort_sequential(arr, pi + 1, high);
    }
}

// Функция для проверки отсортированности массива
int is_sorted(const int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;  // Не отсортирован
        }
    }
    return 1;  // Отсортирован
}

int main() {
    int size;
    double start_time, end_time;
    
    // Замер времени начала выполнения
    start_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Чтение массива из файла
    int* array = read_array_from_file("array.txt", &size);
    
    // Создаем копию массива для сортировки
    int* array_to_sort = (int*)malloc(size * sizeof(int));
    if (!array_to_sort) {
        perror("Ошибка выделения памяти");
        free(array);
        return 1;
    }
    
    // Копируем массив
    for (int i = 0; i < size; i++) {
        array_to_sort[i] = array[i];
    }
    
    // Сортировка
    quick_sort_sequential(array_to_sort, 0, size - 1);
    
    // Проверка корректности сортировки
    if (!is_sorted(array_to_sort, size)) {
        printf("Ошибка: массив не отсортирован корректно!\n");
    }
    
    // Замер времени окончания выполнения
    end_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Вывод результатов
    printf("Последовательная быстрая сортировка\n");
    printf("Размер массива: %d элементов\n", size);
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    
    // Освобождение памяти
    free(array);
    free(array_to_sort);
    
    return 0;
}
