#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cuda_runtime.h>

#define THREADS 256

// Ядро для сравнения и обмена элементов
__global__ void bitonicSortStep(int* dev_values, int j, int k) {
    unsigned int i = threadIdx.x + blockDim.x * blockIdx.x;
    unsigned int ixj = i ^ j;
    
    if (ixj > i) {
        if ((i & k) == 0) {
            // Сортируем по возрастанию
            if (dev_values[i] > dev_values[ixj]) {
                int temp = dev_values[i];
                dev_values[i] = dev_values[ixj];
                dev_values[ixj] = temp;
            }
        } else {
            // Сортируем по убыванию
            if (dev_values[i] < dev_values[ixj]) {
                int temp = dev_values[i];
                dev_values[i] = dev_values[ixj];
                dev_values[ixj] = temp;
            }
        }
    }
}

// Функция для проверки отсортированности массива
int is_sorted(const int* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

// Функция для чтения массива из файла
int* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Читаем размер массива
    if (fscanf(file, "%d", size) != 1) {
        fprintf(stderr, "Ошибка при чтении размера массива\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Выделяем память под массив
    int* array = (int*)malloc(*size * sizeof(int));
    if (!array) {
        perror("Ошибка выделения памяти");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Читаем элементы массива
    for (int i = 0; i < *size; i++) {
        if (fscanf(file, "%d", &array[i]) != 1) {
            fprintf(stderr, "Ошибка при чтении элемента %d\n", i);
            free(array);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    return array;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    int size;
    int* h_values;
    int* d_values;
    cudaEvent_t start, stop;
    float elapsed_time;

    // Чтение массива из файла
    h_values = read_array_from_file(argv[1], &size);

    // Выравниваем размер до степени двойки
    int size_padded = 1;
    while (size_padded < size) {
        size_padded <<= 1;
    }

    // Выделяем память на хосте с выравниванием
    int* h_padded = (int*)malloc(size_padded * sizeof(int));
    if (!h_padded) {
        perror("Ошибка выделения памяти");
        free(h_values);
        return 1;
    }

    // Копируем данные и заполняем оставшиеся элементы максимальным значением
    for (int i = 0; i < size; i++) {
        h_padded[i] = h_values[i];
    }
    for (int i = size; i < size_padded; i++) {
        h_padded[i] = INT_MAX;
    }

    // Выделяем память на устройстве
    cudaMalloc((void**)&d_values, size_padded * sizeof(int));
    cudaMemcpy(d_values, h_padded, size_padded * sizeof(int), cudaMemcpyHostToDevice);

    // Создаем события для замера времени
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);

    // Запускаем Bitonic Sort
    int j, k;
    for (k = 2; k <= size_padded; k <<= 1) {
        for (j = k >> 1; j > 0; j >>= 1) {
            bitonicSortStep<<<(size_padded + THREADS - 1) / THREADS, THREADS>>>(d_values, j, k);
            cudaDeviceSynchronize();
        }
    }

    // Останавливаем таймер
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed_time, start, stop);

    // Копируем результат обратно на хост
    cudaMemcpy(h_padded, d_values, size_padded * sizeof(int), cudaMemcpyDeviceToHost);

    // Проверяем сортировку
    if (is_sorted(h_padded, size)) {
        printf("Массив успешно отсортирован за %.4f секунд\n", elapsed_time / 1000.0f);
    } else {
        printf("Ошибка: массив не отсортирован\n");
    }

    // Освобождаем ресурсы
    free(h_values);
    free(h_padded);
    cudaFree(d_values);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return 0;
}