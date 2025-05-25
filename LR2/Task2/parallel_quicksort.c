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

// Параллельная быстрая сортировка
void quick_sort_parallel(int arr[], int low, int high, int threshold) {
    if (low < high) {
        // Если размер подмассива меньше порога, сортируем последовательно
        if (high - low < threshold) {
            // Используем последовательную версию для маленьких подмассивов
            if (low < high) {
                int pi = partition(arr, low, high);
                quick_sort_parallel(arr, low, pi - 1, threshold);
                quick_sort_parallel(arr, pi + 1, high, threshold);
            }
        } else {
            // Параллельная сортировка для больших подмассивов
            int pi = partition(arr, low, high);
            
            #pragma omp task firstprivate(arr, low, pi, threshold)
            {
                quick_sort_parallel(arr, low, pi - 1, threshold);
            }
            
            #pragma omp task firstprivate(arr, high, pi, threshold)
            {
                quick_sort_parallel(arr, pi + 1, high, threshold);
            }
            
            // Ожидаем завершения всех задач
            #pragma omp taskwait
        }
    }
}

// Обертка для параллельной сортировки
void parallel_quicksort(int arr[], int size, int num_threads, int threshold) {
    // Устанавливаем количество потоков
    omp_set_num_threads(num_threads);
    
    // Запускаем параллельный регион с одной задачей
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            quick_sort_parallel(arr, 0, size - 1, threshold);
        }
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

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        printf("Использование: %s <количество_потоков> <порог>\n", argv[0]);
        printf("  порог - минимальный размер подмассива для параллельной обработки\n");
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    int threshold = atoi(argv[2]);
    
    if (num_threads <= 0 || threshold <= 0) {
        printf("Количество потоков и порог должны быть положительными числами\n");
        return 1;
    }
    
    int size;
    double start_time, end_time;
    
    // Замер времени начала выполнения
    start_time = omp_get_wtime();
    
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
    
    // Параллельная сортировка
    parallel_quicksort(array_to_sort, size, num_threads, threshold);
    
    // Проверка корректности сортировки
    if (!is_sorted(array_to_sort, size)) {
        printf("Ошибка: массив не отсортирован корректно!\n");
    }
    
    // Замер времени окончания выполнения
    end_time = omp_get_wtime();
    
    // Вывод результатов
    printf("Параллельная быстрая сортировка\n");
    printf("Количество потоков: %d\n", num_threads);
    printf("Порог параллелизма: %d\n", threshold);
    printf("Размер массива: %d элементов\n", size);
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    
    // Освобождение памяти
    free(array);
    free(array_to_sort);
    
    return 0;
}
