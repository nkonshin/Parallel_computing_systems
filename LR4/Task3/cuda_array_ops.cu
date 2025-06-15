#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cuda_runtime.h>

#define THREADS_PER_BLOCK 256

// Ядра для операций с массивами
__global__ void add_arrays(const float* a, const float* b, float* result, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        result[idx] = a[idx] + b[idx];
    }
}

__global__ void sub_arrays(const float* a, const float* b, float* result, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        result[idx] = a[idx] - b[idx];
    }
}

__global__ void mul_arrays(const float* a, const float* b, float* result, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        result[idx] = a[idx] * b[idx];
    }
}

__global__ void div_arrays(const float* a, const float* b, float* result, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        if (b[idx] != 0.0f) {
            result[idx] = a[idx] / b[idx];
        } else {
            result[idx] = 0.0f; // Обработка деления на ноль
        }
    }
}

// Функция для чтения массива из файла
float* read_array_from_file(const char* filename, int* size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Подсчет количества чисел в файле
    int count = 0;
    float temp;
    while (fscanf(file, "%f", &temp) == 1) {
        count++;
    }
    
    // Возврат к началу файла
    rewind(file);
    
    // Выделение памяти под массив
    float* arr = (float*)malloc(count * sizeof(float));
    if (!arr) {
        perror("Ошибка выделения памяти");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Чтение данных
    for (int i = 0; i < count; i++) {
        if (fscanf(file, "%f", &arr[i]) != 1) {
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

// Функция для вывода первых 20 элементов массива
void print_first_20(const char* name, const float* arr, int size) {
    printf("\n%s (первые 20 из %d):\n", name, size);
    for (int i = 0; i < 20 && i < size; i++) {
        printf("%7.2f", arr[i]);
        if ((i + 1) % 5 == 0) printf("\n");
        else printf(" | ");
    }
    if (size > 20) printf("\n... и еще %d элементов\n", size - 20);
}

int main() {
    // Выделяем события для замера времени
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    float elapsed_time;

    // Чтение массивов из файлов
    int size1, size2;
    float *h_a = read_array_from_file("array1.txt", &size1);
    float *h_b = read_array_from_file("array2.txt", &size2);
    
    // Проверка на одинаковый размер массивов
    if (size1 != size2) {
        printf("Ошибка: массивы разного размера (%d и %d)\n", size1, size2);
        free(h_a);
        free(h_b);
        return 1;
    }
    
    int size = size1;
    printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ (CUDA) ===\n");
    printf("Размер массивов: %d элементов\n", size);

    // Выделяем память на устройстве
    float *d_a, *d_b, *d_sum, *d_sub, *d_mul, *d_div;
    float *h_sum = (float*)malloc(size * sizeof(float));
    float *h_sub = (float*)malloc(size * sizeof(float));
    float *h_mul = (float*)malloc(size * sizeof(float));
    float *h_div = (float*)malloc(size * sizeof(float));

    cudaMalloc((void**)&d_a, size * sizeof(float));
    cudaMalloc((void**)&d_b, size * sizeof(float));
    cudaMalloc((void**)&d_sum, size * sizeof(float));
    cudaMalloc((void**)&d_sub, size * sizeof(float));
    cudaMalloc((void**)&d_mul, size * sizeof(float));
    cudaMalloc((void**)&d_div, size * sizeof(float));

    // Копируем данные на устройство
    cudaMemcpy(d_a, h_a, size * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, size * sizeof(float), cudaMemcpyHostToDevice);

    // Настраиваем размеры блоков и сетки
    int threadsPerBlock = THREADS_PER_BLOCK;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;

    // Запускаем таймер
    cudaEventRecord(start, 0);

    // Выполняем операции на GPU
    add_arrays<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_sum, size);
    sub_arrays<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_sub, size);
    mul_arrays<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_mul, size);
    div_arrays<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_div, size);

    // Синхронизируем и останавливаем таймер
    cudaDeviceSynchronize();
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed_time, start, stop);

    // Копируем результаты обратно на хост
    cudaMemcpy(h_sum, d_sum, size * sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_sub, d_sub, size * sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_mul, d_mul, size * sizeof(float), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_div, d_div, size * sizeof(float), cudaMemcpyDeviceToHost);

    // Выводим результаты
    printf("Время выполнения: %.6f секунд\n", elapsed_time / 1000.0f);
    print_first_20("Сумма", h_sum, size);
    print_first_20("Разность", h_sub, size);
    print_first_20("Произведение", h_mul, size);
    print_first_20("Частное", h_div, size);

    // Освобождаем ресурсы
    free(h_a);
    free(h_b);
    free(h_sum);
    free(h_sub);
    free(h_mul);
    free(h_div);
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_sum);
    cudaFree(d_sub);
    cudaFree(d_mul);
    cudaFree(d_div);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    return 0;
}