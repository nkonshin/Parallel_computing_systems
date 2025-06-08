#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

// Функция для чтения массива из файла
double* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Подсчет количества чисел в файле
    int count = 0;
    double temp;
    while (fscanf(file, "%lf", &temp) == 1) {
        count++;
    }
    
    // Возврат к началу файла
    rewind(file);
    
    // Выделение памяти под массив
    double* arr = (double*)malloc(count * sizeof(double));
    if (!arr) {
        perror("Ошибка выделения памяти");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    // Чтение чисел в массив
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%lf", &arr[i]) != 1) {
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

// Функция для вывода первых N элементов массива
void print_array_sample(const double* arr, int total_size, int sample_size, const char* label, int rank) {
    if (rank != 0) return; // Выводим только на процессе 0
    
    printf("%s (первые %d из %d):\n", label, sample_size, total_size);
    for (int i = 0; i < sample_size && i < total_size; i++) {
        printf("%7.2lf", arr[i]);
        if ((i + 1) % 5 == 0 || i == sample_size - 1) {
            printf("\n");
        } else {
            printf(" | ");
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    int rank, size;
    double *array1 = NULL, *array2 = NULL;
    double *local_array1 = NULL, *local_array2 = NULL;
    double *local_result_add = NULL, *local_result_sub = NULL;
    double *local_result_mul = NULL, *local_result_div = NULL;
    double *result_add = NULL, *result_sub = NULL, *result_mul = NULL, *result_div = NULL;
    int global_size = 0, local_size = 0;
    double start_time, end_time, compute_time = 0, comm_time = 0;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ ===\n");
        
        // Чтение массивов из файлов (только процесс 0)
        array1 = read_array_from_file("array1.txt", &global_size);
        array2 = read_array_from_file("array2.txt", &local_size); // Используем local_size как временную переменную
        
        // Проверка размеров массивов
        if (global_size != local_size) {
            fprintf(stderr, "Ошибка: массивы имеют разный размер (%d и %d)\n", 
                    global_size, local_size);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("Размер массивов: %d элементов\n", global_size);
        printf("Используется %d процессов\n", size);
    }
    
    // Синхронизация перед началом работы
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Рассылаем размер массивов всем процессам
    MPI_Bcast(&global_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Вычисляем размер локальной части для каждого процесса
    local_size = global_size / size;
    int remainder = global_size % size;
    
    // Массивы для хранения размеров и смещений
    int* recvcounts = (int*)malloc(size * sizeof(int));
    int* displs = (int*)malloc(size * sizeof(int));
    
    // Вычисляем размеры частей и смещения
    int offset = 0;
    for (int i = 0; i < size; i++) {
        recvcounts[i] = global_size / size + (i < remainder ? 1 : 0);
        displs[i] = offset;
        offset += recvcounts[i];
    }
    
    // Обновляем локальный размер для текущего процесса
    local_size = recvcounts[rank];
    
    // Выделяем память под локальные части массивов
    local_array1 = (double*)malloc(local_size * sizeof(double));
    local_array2 = (double*)malloc(local_size * sizeof(double));
    
    // Распределяем данные между процессами
    MPI_Scatterv(array1, recvcounts, displs, MPI_DOUBLE,
                local_array1, local_size, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    MPI_Scatterv(array2, recvcounts, displs, MPI_DOUBLE,
                local_array2, local_size, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // Выделяем память под результаты
    local_result_add = (double*)malloc(local_size * sizeof(double));
    local_result_sub = (double*)malloc(local_size * sizeof(double));
    local_result_mul = (double*)malloc(local_size * sizeof(double));
    local_result_div = (double*)malloc(local_size * sizeof(double));
    
    // Выполняем вычисления над локальными частями
    double compute_start = MPI_Wtime();
    
    for (int i = 0; i < local_size; i++) {
        local_result_add[i] = local_array1[i] + local_array2[i];
        local_result_sub[i] = local_array1[i] - local_array2[i];
        local_result_mul[i] = local_array1[i] * local_array2[i];
        // Проверка деления на ноль
        local_result_div[i] = (local_array2[i] != 0) ? 
                             (local_array1[i] / local_array2[i]) : 0;
    }
    
    compute_time = MPI_Wtime() - compute_start;
    
    // Собираем результаты на процессе 0
    if (rank == 0) {
        result_add = (double*)malloc(global_size * sizeof(double));
        result_sub = (double*)malloc(global_size * sizeof(double));
        result_mul = (double*)malloc(global_size * sizeof(double));
        result_div = (double*)malloc(global_size * sizeof(double));
    }
    
    double comm_start = MPI_Wtime();
    
    // Собираем результаты на процессе 0
    MPI_Gatherv(local_result_add, local_size, MPI_DOUBLE,
               result_add, recvcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    MPI_Gatherv(local_result_sub, local_size, MPI_DOUBLE,
               result_sub, recvcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    MPI_Gatherv(local_result_mul, local_size, MPI_DOUBLE,
               result_mul, recvcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    MPI_Gatherv(local_result_div, local_size, MPI_DOUBLE,
               result_div, recvcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    comm_time = MPI_Wtime() - comm_start;
    
    // Синхронизация и замер времени
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    
    // Вывод результатов на процессе 0
    if (rank == 0) {
        printf("Время выполнения: %.6f секунд\n", end_time - start_time);
        printf("  - Время вычислений: %.6f секунд\n", compute_time);
        printf("  - Время обмена данными: %.6f секунд\n", comm_time);
        
        // Выводим образцы результатов
        int sample_size = 20;
        print_array_sample(result_add, global_size, sample_size, "Сумма", rank);
        print_array_sample(result_sub, global_size, sample_size, "Разность", rank);
        print_array_sample(result_mul, global_size, sample_size, "Произведение", rank);
        print_array_sample(result_div, global_size, sample_size, "Частное", rank);
    }
    
    // Освобождаем память
    if (rank == 0) {
        free(array1);
        free(array2);
        free(result_add);
        free(result_sub);
        free(result_mul);
        free(result_div);
    }
    
    free(local_array1);
    free(local_array2);
    free(local_result_add);
    free(local_result_sub);
    free(local_result_mul);
    free(local_result_div);
    free(recvcounts);
    free(displs);
    
    // Завершаем MPI
    MPI_Finalize();
    
    return 0;
}