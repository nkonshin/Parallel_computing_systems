#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

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

// Функция для слияния двух отсортированных массивов
void merge_arrays(int* arr1, int size1, int* arr2, int size2, int* result) {
    int i = 0, j = 0, k = 0;
    
    while (i < size1 && j < size2) {
        if (arr1[i] <= arr2[j]) {
            result[k++] = arr1[i++];
        } else {
            result[k++] = arr2[j++];
        }
    }
    
    while (i < size1) {
        result[k++] = arr1[i++];
    }
    
    while (j < size2) {
        result[k++] = arr2[j++];
    }
}

// Функция для вывода первых и последних 5 элементов массива
void print_array_sample(const int* arr, int size, const char* label, int rank) {
    if (rank != 0) return; // Выводим только на процессе 0
    
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

int main(int argc, char* argv[]) {
    int rank, proc_size;
    int* global_array = NULL;
    int global_size = 0;
    double start_time, end_time, local_sort_time = 0, merge_time = 0;
    int* local_array = NULL;
    int* recvcounts = NULL;
    int* displs = NULL;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_size);
    
    // Инициализация массивов, чтобы избежать освобождения неинициализированных указателей
    if (rank == 0) {
        printf("=== ПАРАЛЛЕЛЬНАЯ ПУЗЫРЬКОВАЯ СОРТИРОВКА ===\n");
        printf("Используется %d процессов\n", proc_size);
        
        const char* filename = "array.txt";
        printf("Чтение массива из файла %s...\n", filename);
        global_array = read_array_from_file(filename, &global_size);
        if (!global_array) {
            fprintf(stderr, "Ошибка при чтении массива\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Прочитано %d элементов\n", global_size);
        print_array_sample(global_array, global_size, "Несортированный массив", rank);
    }
    
    // Синхронизация перед началом сортировки
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Рассылаем размер массива всем процессам
    MPI_Bcast(&global_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Проверка на случай, если размер массива меньше числа процессов
    if (global_size == 0) {
        if (rank == 0) {
            fprintf(stderr, "Ошибка: массив пуст\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    // Вычисляем размер части массива для каждого процесса
    int local_size = global_size / proc_size;
    int remainder = global_size % proc_size;
    
    // Выделяем память для массивов размеров и смещений
    recvcounts = (int*)malloc(proc_size * sizeof(int));
    displs = (int*)malloc(proc_size * sizeof(int));
    
    if (!recvcounts || !displs) {
        fprintf(stderr, "Ошибка выделения памяти для recvcounts/displs\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Вычисляем размеры частей и смещения
    int offset = 0;
    for (int i = 0; i < proc_size; i++) {
        recvcounts[i] = local_size + (i < remainder ? 1 : 0);
        displs[i] = offset;
        offset += recvcounts[i];
    }
    
    // Размер локальной части для текущего процесса
    local_size = recvcounts[rank];
    local_array = (int*)malloc(local_size * sizeof(int));
    if (!local_array) {
        fprintf(stderr, "Ошибка выделения памяти для local_array\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Распределяем данные между процессами
    MPI_Scatterv(global_array, recvcounts, displs, MPI_INT,
                local_array, local_size, MPI_INT,
                0, MPI_COMM_WORLD);
    
    // Сортируем локальную часть
    double local_start = MPI_Wtime();
    bubble_sort(local_array, local_size);
    local_sort_time = MPI_Wtime() - local_start;
    
    // Собираем отсортированные части на процессе 0
    if (rank == 0) {
        // Копируем первую часть в результирующий массив
        int* temp_buffer = (int*)malloc(global_size * sizeof(int));
        if (!temp_buffer) {
            fprintf(stderr, "Ошибка выделения памяти для temp_buffer\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        memcpy(global_array, local_array, local_size * sizeof(int));
        int merged_size = local_size;
        
        double merge_start = MPI_Wtime();
        for (int i = 1; i < proc_size; i++) {
            int recv_size = recvcounts[i];
            int* recv_buf = temp_buffer + merged_size;
            
            MPI_Recv(recv_buf, recv_size, MPI_INT, i, 0, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Объединяем с уже отсортированной частью
            merge_arrays(global_array, merged_size, 
                        recv_buf, recv_size,
                        temp_buffer);
            
            // Копируем результат обратно в global_array
            memcpy(global_array, temp_buffer, 
                  (merged_size + recv_size) * sizeof(int));
            
            merged_size += recv_size;
        }
        merge_time = MPI_Wtime() - merge_start;
        
        free(temp_buffer);
    } else {
        // Отправляем отсортированную часть на процесс 0
        MPI_Send(local_array, local_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    
    // Синхронизация после завершения сортировки
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    
    // Выводим результаты
    if (rank == 0) {
        // Проверка отсортированности
        printf("Проверка отсортированности... ");
        fflush(stdout);
        
        if (is_sorted(global_array, global_size)) {
            printf("массив отсортирован корректно\n");
        } else {
            printf("ОШИБКА: массив не отсортирован!\n");
        }
        
        // Выводим образец отсортированного массива
        print_array_sample(global_array, global_size, "Отсортированный массив", rank);
        
        // Выводим время выполнения
        printf("Общее время выполнения: %.6f секунд\n", end_time - start_time);
        printf("Время сортировки (локальные части): %.6f секунд\n", local_sort_time);
        printf("Время слияния: %.6f секунд\n", merge_time);
        
    }
    
    // Освобождаем память
    if (local_array) free(local_array);
    if (recvcounts) free(recvcounts);
    if (displs) free(displs);
    if (rank == 0 && global_array) {
        free(global_array);
    }
    
    // Завершаем MPI
    MPI_Finalize();
    
    return 0;
}