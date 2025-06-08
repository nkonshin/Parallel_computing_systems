#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Функция для чтения матрицы из файла
double** read_matrix_from_file(const char* filename, int* rows, int* cols) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Читаем размеры матрицы
    if (fscanf(file, "%d %d", rows, cols) != 2) {
        fprintf(stderr, "Ошибка при чтении размеров матрицы\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    // Выделяем память под матрицу
    double** matrix = (double**)malloc(*rows * sizeof(double*));
    if (!matrix) {
        perror("Ошибка выделения памяти для матрицы");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < *rows; i++) {
        matrix[i] = (double*)malloc(*cols * sizeof(double));
        if (!matrix[i]) {
            perror("Ошибка выделения памяти для строки матрицы");
            // Освобождаем уже выделенную память в случае ошибки
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        
        // Читаем элементы строки
        for (int j = 0; j < *cols; j++) {
            if (fscanf(file, "%lf", &matrix[i][j]) != 1) {
                fprintf(stderr, "Ошибка при чтении элемента [%d][%d]\n", i, j);
                // Освобождаем память в случае ошибки
                for (int k = 0; k <= i; k++) {
                    free(matrix[k]);
                }
                free(matrix);
                fclose(file);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    fclose(file);
    return matrix;
}

// Функция для освобождения памяти, выделенной под матрицу
void free_matrix(double** matrix, int rows) {
    if (matrix) {
        for (int i = 0; i < rows; i++) {
            free(matrix[i]);
        }
        free(matrix);
    }
}

// Функция для выполнения операций над матрицами
void perform_matrix_operations(double** matrix1, double** matrix2, 
                             double** result_add, double** result_sub,
                             double** result_mul, double** result_div,
                             int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // Сложение
            result_add[i][j] = matrix1[i][j] + matrix2[i][j];
            
            // Вычитание
            result_sub[i][j] = matrix1[i][j] - matrix2[i][j];
            
            // Умножение
            result_mul[i][j] = matrix1[i][j] * matrix2[i][j];
            
            // Деление (с проверкой деления на ноль)
            if (matrix2[i][j] != 0.0) {
                result_div[i][j] = matrix1[i][j] / matrix2[i][j];
            } else {
                result_div[i][j] = NAN; // Not a Number при делении на ноль
            }
        }
    }
}

// Функция для выполнения операций над матрицами и вывода первых 5 результатов
void perform_operations_and_print(double** matrix1, double** matrix2, int rows, int cols) {
    printf("Первые 5 результатов операций:\n");
    printf("Индекс |   Сложение  |  Вычитание  | Умножение  |  Деление   \n");
    printf("-------+-------------+-------------+------------+-------------\n");
    
    int count = 0;
    int printed = 0;
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double a = matrix1[i][j];
            double b = matrix2[i][j];
            
            // Выполняем операции
            double add = a + b;
            double sub = a - b;
            double mul = a * b;
            double div = (b != 0.0) ? a / b : NAN;
            
            // Выводим первые 5 результатов
            if (printed < 5) {
                printf("%6d | %11.2f | %11.2f | %10.2f | ", 
                       count + 1, add, sub, mul);
                
                if (isnan(div)) {
                    printf("%11s", "NaN");
                } else {
                    printf("%11.2f", div);
                }
                printf("\n");
                printed++;
            }
            
            count++;
        }
    }
    printf("\nВсего обработано элементов: %d\n\n", rows * cols);
}

int main() {
    clock_t start, end;
    double cpu_time_used;
    int rows1, cols1, rows2, cols2;
    
    // Замер времени начала выполнения
    start = clock();
    
    // Чтение первой матрицы из файла
    double** matrix1 = read_matrix_from_file("matrix1.txt", &rows1, &cols1);
    
    // Чтение второй матрицы из файла
    double** matrix2 = read_matrix_from_file("matrix2.txt", &rows2, &cols2);
    
    // Проверка, что матрицы одного размера
    if (rows1 != rows2 || cols1 != cols2) {
        fprintf(stderr, "Ошибка: матрицы имеют разные размеры (%dx%d и %dx%d)\n", 
                rows1, cols1, rows2, cols2);
        free_matrix(matrix1, rows1);
        free_matrix(matrix2, rows2);
        return 1;
    }
    
    int rows = rows1;
    int cols = cols1;
    
    // Выделяем память под результаты операций
    double** result_add = (double**)malloc(rows * sizeof(double*));
    double** result_sub = (double**)malloc(rows * sizeof(double*));
    double** result_mul = (double**)malloc(rows * sizeof(double*));
    double** result_div = (double**)malloc(rows * sizeof(double*));
    
    if (!result_add || !result_sub || !result_mul || !result_div) {
        perror("Ошибка выделения памяти для результатов");
        free_matrix(matrix1, rows);
        free_matrix(matrix2, rows);
        if (result_add) free(result_add);
        if (result_sub) free(result_sub);
        if (result_mul) free(result_mul);
        if (result_div) free(result_div);
        return 1;
    }
    
    for (int i = 0; i < rows; i++) {
        result_add[i] = (double*)malloc(cols * sizeof(double));
        result_sub[i] = (double*)malloc(cols * sizeof(double));
        result_mul[i] = (double*)malloc(cols * sizeof(double));
        result_div[i] = (double*)malloc(cols * sizeof(double));
        
        if (!result_add[i] || !result_sub[i] || !result_mul[i] || !result_div[i]) {
            perror("Ошибка выделения памяти для строк результатов");
            // Освобождаем уже выделенную память
            for (int j = 0; j <= i; j++) {
                if (result_add[j]) free(result_add[j]);
                if (result_sub[j]) free(result_sub[j]);
                if (result_mul[j]) free(result_mul[j]);
                if (result_div[j]) free(result_div[j]);
            }
            free(result_add);
            free(result_sub);
            free(result_mul);
            free(result_div);
            free_matrix(matrix1, rows);
            free_matrix(matrix2, rows);
            return 1;
        }
    }
    
    // Выполнение операций над матрицами
    perform_matrix_operations(matrix1, matrix2, result_add, result_sub, 
                            result_mul, result_div, rows, cols);
    
    // Замер времени окончания выполнения
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    // Вывод результатов
    printf("=== ПОСЛЕДОВАТЕЛЬНАЯ ВЕРСИЯ ===\n");
    printf("Размер матриц: %dx%d (всего %d элементов)\n", rows, cols, rows * cols);
    printf("Время выполнения: %.6f секунд\n", cpu_time_used);
    
    // Вычисляем и выводим скорость обработки (операций в секунду)
    // Умножаем на 4, так как выполняем 4 операции над каждым элементом (+, -, *, /)
    double total_operations = (double)(rows * cols) * 4.0;
    printf("Скорость: %.2f операций/сек\n", total_operations / cpu_time_used);
    
    // Выполняем операции и выводим результаты
    perform_operations_and_print(matrix1, matrix2, rows, cols);
    
    // Освобождение памяти
    free_matrix(matrix1, rows);
    free_matrix(matrix2, rows);
    free_matrix(result_add, rows);
    free_matrix(result_sub, rows);
    free_matrix(result_mul, rows);
    free_matrix(result_div, rows);
    
    return 0;
}
