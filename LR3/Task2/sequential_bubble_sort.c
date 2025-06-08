#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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

// Функция для проверки отсортированности массива
bool is_sorted(const int* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return false;
        }
    }
    return true;
}

// Оптимизированная пузырьковая сортировка
void bubble_sort(int* arr, int size) {
    bool swapped;
    for (int i = 0; i < size - 1; i++) {
        swapped = false;
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                // Обмен элементов
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
                swapped = true;
            }
        }
        
        // Если на текущей итерации не было перестановок, массив отсортирован
        if (!swapped) {
            break;
        }
    }
}

// Функция для вывода первых и последних 5 элементов массива
void print_array_sample(const int* arr, int size, const char* label) {
    printf("%s: [", label);
    // Выводим первые 5 элементов
    for (int i = 0; i < 5 && i < size; i++) {
        printf("%d", arr[i]);
        if (i < 4 && i < size - 1) printf(", ");
    }
    
    if (size > 10) {
        printf(", ..., ");
        // Выводим последние 5 элементов
        for (int i = size - 5; i < size; i++) {
            printf("%d", arr[i]);
            if (i < size - 1) printf(", ");
        }
    } else if (size > 5) {
        // Если элементов немного больше 5, выводим оставшиеся
        for (int i = 5; i < size; i++) {
            printf(", %d", arr[i]);
        }
    }
    printf("]\n");
}

int main() {
    const char* filename = "array.txt";
    int size;
    clock_t start, end;
    double cpu_time_used;
    
    printf("=== ПОСЛЕДОВАТЕЛЬНАЯ ПУЗЫРЬКОВАЯ СОРТИРОВКА ===\n");
    
    // Засекаем общее время выполнения
    start = clock();
    
    // Чтение массива из файла
    printf("Чтение массива из файла %s...\n", filename);
    int* arr = read_array_from_file(filename, &size);
    printf("Прочитано %d элементов\n", size);
    
    // Выводим образец несортированного массива
    print_array_sample(arr, size, "Несортированный массив");
    
    // Сортировка
    printf("Сортировка...\n");
    clock_t sort_start = clock();
    bubble_sort(arr, size);
    clock_t sort_end = clock();
    
    // Проверка отсортированности
    printf("Проверка отсортированности... ");
    if (is_sorted(arr, size)) {
        printf("массив отсортирован корректно\n");
    } else {
        printf("ОШИБКА: массив не отсортирован!\n");
    }
    
    // Выводим образец отсортированного массива
    print_array_sample(arr, size, "Отсортированный массив");
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    double sort_time = ((double) (sort_end - sort_start)) / CLOCKS_PER_SEC;
    
    printf("Время сортировки: %.6f секунд\n", sort_time);
    printf("Общее время выполнения: %.6f секунд\n", cpu_time_used);
    
    // Освобождаем память
    free(arr);
    
    return 0;
}