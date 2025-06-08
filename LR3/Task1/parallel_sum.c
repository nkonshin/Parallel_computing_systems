#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Функция для чтения массива из файла (только корневым процессом)
int* read_array_from_file(const char* filename, int* size, int rank) {
    FILE* file = NULL;
    int* arr = NULL;
    
    if (rank == 0) {  // Только корневой процесс читает файл
        file = fopen(filename, "r");
        if (!file) {
            perror("Ошибка при открытии файла");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Подсчет количества чисел в файле
        int count = 0;
        int temp;
        while (fscanf(file, "%d", &temp) == 1) {
            count++;
        }
        
        rewind(file);  // Возврат к началу файла
        
        // Выделение памяти под массив
        arr = (int*)malloc(count * sizeof(int));
        if (!arr) {
            perror("Ошибка выделения памяти");
            fclose(file);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Чтение чисел в массив
        for (int i = 0; i < count; i++) {
            if (fscanf(file, "%d", &arr[i]) != 1) {
                fprintf(stderr, "Ошибка при чтении элемента %d\n", i);
                free(arr);
                fclose(file);
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
        }
        
        fclose(file);
        *size = count;
    }
    
    return arr;
}

// Функция для вычисления частичной суммы
long long calculate_partial_sum(const int* arr, int start, int end) {
    long long sum = 0;
    for (int i = start; i < end; i++) {
        sum += arr[i];
    }
    return sum;
}

int main(int argc, char** argv) {
    int rank, size;
    int* global_array = NULL;
    int global_size = 0;
    double start_time, end_time;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Корневой процесс читает массив из файла
    global_array = read_array_from_file("array.txt", &global_size, rank);
    
    // Синхронизация перед началом замера времени
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Рассылаем размер массива всем процессам
    MPI_Bcast(&global_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Вычисляем размер части массива для каждого процесса
    int chunk_size = global_size / size;
    int remainder = global_size % size;
    
    // Массивы для хранения размеров и смещений для каждого процесса
    int* send_counts = NULL;
    int* displacements = NULL;
    
    if (rank == 0) {
        send_counts = (int*)malloc(size * sizeof(int));
        displacements = (int*)malloc(size * sizeof(int));
        
        // Вычисляем размеры и смещения для каждого процесса
        int sum = 0;
        for (int i = 0; i < size; i++) {
            send_counts[i] = chunk_size + (i < remainder ? 1 : 0);
            displacements[i] = sum;
            sum += send_counts[i];
        }
    }
    
    // Отправляем каждому процессу его размер части массива
    int local_size;
    if (rank == 0) {
        local_size = send_counts[0];
    } else {
        local_size = chunk_size + (rank < remainder ? 1 : 0);
    }
    
    // Выделяем память под локальную часть массива
    int* local_array = (int*)malloc(local_size * sizeof(int));
    
    // Рассылаем части массива процессам
    MPI_Scatterv(global_array, send_counts, displacements, MPI_INT,
                local_array, local_size, MPI_INT,
                0, MPI_COMM_WORLD);
    
    // Вычисляем локальную сумму
    long long local_sum = calculate_partial_sum(local_array, 0, local_size);
    
    // Собираем частичные суммы на корневом процессе
    long long total_sum;
    MPI_Reduce(&local_sum, &total_sum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    
    // Синхронизация перед замером времени
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    
    // Выводим результаты на корневом процессе
    if (rank == 0) {
        printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ (MPI) ===\n");
        printf("Количество процессов: %d\n", size);
        printf("Размер массива: %d элементов\n", global_size);
        printf("Сумма элементов: %lld\n", total_sum);
        printf("Время выполнения: %.6f секунд\n", end_time - start_time);
        
        // Освобождаем память
        free(send_counts);
        free(displacements);
    }
    
    // Освобождаем память
    if (rank == 0) {
        free(global_array);
    }
    free(local_array);
    
    // Завершаем работу с MPI
    MPI_Finalize();
    return 0;
}