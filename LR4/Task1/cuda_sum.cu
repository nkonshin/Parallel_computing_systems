#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

// Ядро CUDA для вычисления частичных сумм
__global__ void sumKernel(const int* array, long long* partialSums, int size) {
    extern __shared__ long long sharedSums[];
    
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Каждый поток обрабатывает свой элемент
    sharedSums[tid] = (i < size) ? array[i] : 0;
    __syncthreads();
    
    // Свертка в shared memory
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s && (i + s) < size) {
            sharedSums[tid] += sharedSums[tid + s];
        }
        __syncthreads();
    }
    
    // Первый поток блока записывает частичную сумму
    if (tid == 0) {
        partialSums[blockIdx.x] = sharedSums[0];
    }
}

// Функция для чтения массива из файла (аналогичная последовательной версии)
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

int main() {
    int size;
    double start_time, end_time;
    
    // Замер времени начала выполнения
    start_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Чтение массива из файла
    int* h_array = read_array_from_file("array.txt", &size);
    
    // Выделяем память на устройстве
    int* d_array;
    long long* d_partialSums;
    long long* h_partialSums;
    
    // Количество потоков на блок (должно быть степенью двойки)
    const int threadsPerBlock = 256;
    // Количество блоков
    const int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    // Размер shared memory для каждого блока
    const size_t sharedMemSize = threadsPerBlock * sizeof(long long);
    
    // Выделяем память под частичные суммы на хосте
    h_partialSums = (long long*)malloc(blocksPerGrid * sizeof(long long));
    
    // Выделяем память на устройстве
    cudaMalloc((void**)&d_array, size * sizeof(int));
    cudaMalloc((void**)&d_partialSums, blocksPerGrid * sizeof(long long));
    
    // Копируем данные на устройство
    cudaMemcpy(d_array, h_array, size * sizeof(int), cudaMemcpyHostToDevice);
    
    // Запускаем ядро
    sumKernel<<<blocksPerGrid, threadsPerBlock, sharedMemSize>>>(d_array, d_partialSums, size);
    
    // Копируем частичные суммы обратно на хост
    cudaMemcpy(h_partialSums, d_partialSums, blocksPerGrid * sizeof(long long), cudaMemcpyDeviceToHost);
    
    // Суммируем частичные суммы на хосте
    long long totalSum = 0;
    for (int i = 0; i < blocksPerGrid; i++) {
        totalSum += h_partialSums[i];
    }
    
    // Замер времени окончания выполнения
    end_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Вывод результатов
    printf("Размер массива: %d элементов\n", size);
    printf("Сумма элементов: %lld\n", totalSum);
    printf("Время выполнения: %.6f секунд\n", end_time - start_time);
    printf("Конфигурация: %d блоков по %d потоков\n", blocksPerGrid, threadsPerBlock);
    
    // Освобождаем память
    free(h_array);
    free(h_partialSums);
    cudaFree(d_array);
    cudaFree(d_partialSums);
    
    return 0;
}