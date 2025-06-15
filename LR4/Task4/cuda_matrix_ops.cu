#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <cuda_runtime.h>

#define THREADS_PER_BLOCK 16  // 16x16 = 256 threads per block

// Ядро для выполнения операций над матрицами
__global__ void matrix_operations_kernel(
    const double* mat1, const double* mat2,
    double* add, double* sub, 
    double* mul, double* div,
    int rows, int cols) {
    
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < rows && col < cols) {
        int idx = row * cols + col;
        double a = mat1[idx];
        double b = mat2[idx];
        
        // Выполняем операции
        add[idx] = a + b;
        sub[idx] = a - b;
        mul[idx] = a * b;
        div[idx] = (b != 0.0) ? a / b : NAN;
    }
}

// Функция для чтения матрицы из файла (аналогичная последовательной версии)
double* read_matrix_from_file(const char* filename, int* rows, int* cols) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d %d", rows, cols) != 2) {
        fprintf(stderr, "Ошибка при чтении размеров матрицы\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    double* matrix = (double*)malloc((*rows) * (*cols) * sizeof(double));
    if (!matrix) {
        perror("Ошибка выделения памяти для матрицы");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < *rows; i++) {
        for (int j = 0; j < *cols; j++) {
            if (fscanf(file, "%lf", &matrix[i * (*cols) + j]) != 1) {
                fprintf(stderr, "Ошибка при чтении элемента [%d][%d]\n", i, j);
                free(matrix);
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    fclose(file);
    return matrix;
}

// Функция для вывода результатов
void print_results(const double* mat1, const double* mat2, int rows, int cols) {
    printf("Первые 5 результатов операций:\n");
    printf("Индекс |   Сложение  |  Вычитание  | Умножение  |  Дение   \n");
    printf("-------+-------------+-------------+------------+------------\n");
    
    int count = 0;
    int printed = 0;
    
    for (int i = 0; i < rows && printed < 5; i++) {
        for (int j = 0; j < cols && printed < 5; j++) {
            int idx = i * cols + j;
            double a = mat1[idx];
            double b = mat2[idx];
            
            printf("%6d | %11.2f | %11.2f | %10.2f | ", 
                   count + 1, a + b, a - b, a * b);
            
            if (b != 0.0) {
                printf("%.2f", a / b);
            } else {
                printf("     NaN");
            }
            printf("\n");
            
            count++;
            printed++;
        }
    }
    printf("\nВсего обработано элементов: %d\n\n", rows * cols);
}

int main() {
    cudaEvent_t start, stop;
    float elapsed_time;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    
    // Замер времени начала выполнения
    cudaEventRecord(start, 0);
    
    // Чтение матриц
    int rows1, cols1, rows2, cols2;
    double *h_mat1 = read_matrix_from_file("matrix1.txt", &rows1, &cols1);
    double *h_mat2 = read_matrix_from_file("matrix2.txt", &rows2, &cols2);
    
    if (rows1 != rows2 || cols1 != cols2) {
        fprintf(stderr, "Ошибка: матрицы имеют разные размеры (%dx%d и %dx%d)\n", 
                rows1, cols1, rows2, cols2);
        free(h_mat1);
        free(h_mat2);
        return 1;
    }
    
    int rows = rows1;
    int cols = cols1;
    size_t size = rows * cols * sizeof(double);
    
    // Выделяем память на устройстве
    double *d_mat1, *d_mat2, *d_add, *d_sub, *d_mul, *d_div;
    cudaMalloc((void**)&d_mat1, size);
    cudaMalloc((void**)&d_mat2, size);
    cudaMalloc((void**)&d_add, size);
    cudaMalloc((void**)&d_sub, size);
    cudaMalloc((void**)&d_mul, size);
    cudaMalloc((void**)&d_div, size);
    
    // Копируем данные на устройство
    cudaMemcpy(d_mat1, h_mat1, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_mat2, h_mat2, size, cudaMemcpyHostToDevice);
    
    // Настраиваем размеры блоков и сетки
    dim3 blockSize(THREADS_PER_BLOCK, THREADS_PER_BLOCK);
    dim3 gridSize((cols + blockSize.x - 1) / blockSize.x, 
                  (rows + blockSize.y - 1) / blockSize.y);
    
    // Запускаем ядро
    matrix_operations_kernel<<<gridSize, blockSize>>>(
        d_mat1, d_mat2, d_add, d_sub, d_mul, d_div, rows, cols);
    
    // Выделяем память для результатов на хосте
    double *h_add = (double*)malloc(size);
    double *h_sub = (double*)malloc(size);
    double *h_mul = (double*)malloc(size);
    double *h_div = (double*)malloc(size);
    
    // Копируем результаты обратно на хост
    cudaMemcpy(h_add, d_add, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_sub, d_sub, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_mul, d_mul, size, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_div, d_div, size, cudaMemcpyDeviceToHost);
    
    // Замер времени окончания выполнения
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed_time, start, stop);
    
    // Вывод результатов
    printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ (CUDA) ===\n");
    printf("Размер матриц: %dx%d (всего %d элементов)\n", rows, cols, rows * cols);
    printf("Время выполнения: %.6f секунд\n", elapsed_time / 1000.0f);
    
    // Вычисляем и выводим скорость обработки
    double total_operations = (double)(rows * cols) * 4.0;
    printf("Скорость: %.2f операций/сек\n", 
           total_operations / (elapsed_time / 1000.0f));
    
    // Выводим первые 5 результатов
    print_results(h_mat1, h_mat2, rows, cols);
    
    // Освобождаем ресурсы
    free(h_mat1);
    free(h_mat2);
    free(h_add);
    free(h_sub);
    free(h_mul);
    free(h_div);
    cudaFree(d_mat1);
    cudaFree(d_mat2);
    cudaFree(d_add);
    cudaFree(d_sub);
    cudaFree(d_mul);
    cudaFree(d_div);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    
    return 0;
}