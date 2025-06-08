#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <string.h>

// Структура для хранения информации о матрице
typedef struct {
    double** data;
    int rows;
    int cols;
} Matrix;

// Функция для создания матрицы
double** create_matrix(int rows, int cols) {
    double** matrix = (double**)malloc(rows * sizeof(double*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (double*)malloc(cols * sizeof(double));
    }
    return matrix;
}

// Функция для освобождения памяти матрицы
void free_matrix(double** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Функция для чтения матрицы из файла
Matrix read_matrix_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
    }

    // Читаем размеры матрицы
    int rows, cols;
    if (fscanf(file, "%d %d", &rows, &cols) != 2) {
        fprintf(stderr, "Ошибка при чтении размеров матрицы из файла\n");
        exit(EXIT_FAILURE);
    }

    // Создаем матрицу
    Matrix matrix;
    matrix.rows = rows;
    matrix.cols = cols;
    matrix.data = create_matrix(rows, cols);

    // Читаем данные
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(file, "%lf", &matrix.data[i][j]) != 1) {
                fprintf(stderr, "Ошибка при чтении элемента [%d][%d]\n", i, j);
                exit(EXIT_FAILURE);
            }
        }
    }

    fclose(file);
    return matrix;
}

// Функция для выполнения операций над матрицами
void perform_operations(Matrix mat1, Matrix mat2, Matrix* add, Matrix* sub, 
                       Matrix* mul, Matrix* div) {
    for (int i = 0; i < mat1.rows; i++) {
        for (int j = 0; j < mat1.cols; j++) {
            add->data[i][j] = mat1.data[i][j] + mat2.data[i][j];
            sub->data[i][j] = mat1.data[i][j] - mat2.data[i][j];
            mul->data[i][j] = mat1.data[i][j] * mat2.data[i][j];
            div->data[i][j] = (mat2.data[i][j] != 0) ? 
                             (mat1.data[i][j] / mat2.data[i][j]) : 0.0;
        }
    }
}

// Функция для вывода результатов
void print_results(Matrix add, Matrix sub, Matrix mul, Matrix div, int count) {
    printf("Первые %d результатов операций:\n", count);
    printf("Индекс |   Сложение  |  Вычитание  | Умножение  |  Деление   \n");
    printf("-------+-------------+-------------+------------+-------------\n");
    
    int elements_printed = 0;
    for (int i = 0; i < add.rows && elements_printed < count; i++) {
        for (int j = 0; j < add.cols && elements_printed < count; j++) {
            printf("%6d | %11.2f | %11.2f | %10.2f | %10.2f\n",
                   elements_printed + 1,
                   add.data[i][j], sub.data[i][j], 
                   mul.data[i][j], div.data[i][j]);
            elements_printed++;
        }
    }
    printf("\nВсего обработано элементов: %d\n", add.rows * add.cols);
}

int main(int argc, char* argv[]) {
    int rank, size;
    double start_time, end_time, compute_time = 0, comm_time = 0;
    Matrix matrix1, matrix2;
    Matrix local_matrix1, local_matrix2;
    Matrix local_add, local_sub, local_mul, local_div;
    Matrix *result_add = NULL, *result_sub = NULL, *result_mul = NULL, *result_div = NULL;
    int *sendcounts = NULL, *displs = NULL;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (rank == 0) {
        printf("=== ПАРАЛЛЕЛЬНАЯ ВЕРСИЯ ===\n");
        
        // Чтение матриц из файлов (только процесс 0)
        matrix1 = read_matrix_from_file("matrix1.txt");
        matrix2 = read_matrix_from_file("matrix2.txt");
        
        // Проверка размеров матриц
        if (matrix1.rows != matrix2.rows || matrix1.cols != matrix2.cols) {
            fprintf(stderr, "Ошибка: матрицы имеют разные размеры\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        printf("Размер матриц: %dx%d (всего %d элементов)\n", 
               matrix1.rows, matrix1.cols, matrix1.rows * matrix1.cols);
        printf("Используется %d процессов\n", size);
    }
    
    // Синхронизация перед началом работы
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Рассылаем размеры матриц всем процессам
    int rows, cols;
    if (rank == 0) {
        rows = matrix1.rows;
        cols = matrix1.cols;
    }
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Вычисляем количество строк на процесс
    int rows_per_proc = rows / size;
    int remainder = rows % size;
    
    // Массивы для хранения размеров и смещений
    sendcounts = (int*)malloc(size * sizeof(int));
    displs = (int*)malloc(size * sizeof(int));
    
    // Вычисляем размеры частей и смещения
    int offset = 0;
    for (int i = 0; i < size; i++) {
        sendcounts[i] = (i < remainder) ? (rows_per_proc + 1) * cols : rows_per_proc * cols;
        displs[i] = offset;
        offset += sendcounts[i];
    }
    
    // Вычисляем локальный размер для текущего процесса
    int local_rows = (rank < remainder) ? (rows_per_proc + 1) : rows_per_proc;
    
    // Выделяем память под локальные части матриц
    local_matrix1.rows = local_rows;
    local_matrix1.cols = cols;
    local_matrix1.data = create_matrix(local_rows, cols);
    
    local_matrix2.rows = local_rows;
    local_matrix2.cols = cols;
    local_matrix2.data = create_matrix(local_rows, cols);
    
    // Создаем дескриптор типа для строки матрицы
    MPI_Datatype row_type;
    MPI_Type_contiguous(cols, MPI_DOUBLE, &row_type);
    MPI_Type_commit(&row_type);
    
    // Распределяем данные между процессами
    if (rank == 0) {
        // Процесс 0 копирует свою часть
        for (int i = 0; i < local_rows; i++) {
            memcpy(local_matrix1.data[i], matrix1.data[i], cols * sizeof(double));
            memcpy(local_matrix2.data[i], matrix2.data[i], cols * sizeof(double));
        }
        
        // Отправляем остальным процессам их части
        for (int dest = 1; dest < size; dest++) {
            int dest_rows = (dest < remainder) ? (rows_per_proc + 1) : rows_per_proc;
            for (int i = 0; i < dest_rows; i++) {
                MPI_Send(&matrix1.data[displs[dest]/cols + i][0], 1, row_type, 
                        dest, 0, MPI_COMM_WORLD);
                MPI_Send(&matrix2.data[displs[dest]/cols + i][0], 1, row_type, 
                        dest, 1, MPI_COMM_WORLD);
            }
        }
    } else {
        // Получаем данные от процесса 0
        for (int i = 0; i < local_rows; i++) {
            MPI_Recv(local_matrix1.data[i], cols, MPI_DOUBLE, 0, 0, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(local_matrix2.data[i], cols, MPI_DOUBLE, 0, 1, 
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    
    // Выделяем память под результаты
    local_add.rows = local_rows;
    local_add.cols = cols;
    local_add.data = create_matrix(local_rows, cols);
    
    local_sub.rows = local_rows;
    local_sub.cols = cols;
    local_sub.data = create_matrix(local_rows, cols);
    
    local_mul.rows = local_rows;
    local_mul.cols = cols;
    local_mul.data = create_matrix(local_rows, cols);
    
    local_div.rows = local_rows;
    local_div.cols = cols;
    local_div.data = create_matrix(local_rows, cols);
    
    // Выполняем вычисления над локальными частями
    double compute_start = MPI_Wtime();
    perform_operations(local_matrix1, local_matrix2, 
                      &local_add, &local_sub, &local_mul, &local_div);
    compute_time = MPI_Wtime() - compute_start;
    
    // Если процесс 0, выделяем память для полных результатов
    if (rank == 0) {
        result_add = (Matrix*)malloc(sizeof(Matrix));
        result_sub = (Matrix*)malloc(sizeof(Matrix));
        result_mul = (Matrix*)malloc(sizeof(Matrix));
        result_div = (Matrix*)malloc(sizeof(Matrix));
        
        result_add->rows = rows;
        result_add->cols = cols;
        result_add->data = create_matrix(rows, cols);
        
        result_sub->rows = rows;
        result_sub->cols = cols;
        result_sub->data = create_matrix(rows, cols);
        
        result_mul->rows = rows;
        result_mul->cols = cols;
        result_mul->data = create_matrix(rows, cols);
        
        result_div->rows = rows;
        result_div->cols = cols;
        result_div->data = create_matrix(rows, cols);
    }
    
    // Собираем результаты на процессе 0
    double comm_start = MPI_Wtime();
    
    // Создаем временные буферы для сбора данных
    double* recv_buf = NULL;
    if (rank == 0) {
        recv_buf = (double*)malloc(rows * cols * sizeof(double));
    }
    
    // Собираем результаты сложения
    MPI_Gatherv(&(local_add.data[0][0]), local_rows * cols, MPI_DOUBLE,
               recv_buf, sendcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result_add->data[i][j] = recv_buf[i * cols + j];
            }
        }
    }
    
    // Собираем результаты вычитания
    MPI_Gatherv(&(local_sub.data[0][0]), local_rows * cols, MPI_DOUBLE,
               recv_buf, sendcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result_sub->data[i][j] = recv_buf[i * cols + j];
            }
        }
    }
    
    // Собираем результаты умножения
    MPI_Gatherv(&(local_mul.data[0][0]), local_rows * cols, MPI_DOUBLE,
               recv_buf, sendcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result_mul->data[i][j] = recv_buf[i * cols + j];
            }
        }
    }
    
    // Собираем результаты деления
    MPI_Gatherv(&(local_div.data[0][0]), local_rows * cols, MPI_DOUBLE,
               recv_buf, sendcounts, displs, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result_div->data[i][j] = recv_buf[i * cols + j];
            }
        }
    }
    
    comm_time = MPI_Wtime() - comm_start;
    
    // Синхронизация и замер времени
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    
    // Вывод результатов на процессе 0
    if (rank == 0) {
        printf("Время выполнения: %.6f секунд\n", end_time - start_time);
        printf("  - Время вычислений: %.6f секунд\n", compute_time);
        printf("  - Время обмена данными: %.6f секунд\n", comm_time);
        
        // Вычисляем и выводим скорость обработки (операций в секунду)
        double total_operations = (double)(rows * cols) * 4.0;
        printf("Скорость: %.2f операций/сек\n", 
               total_operations / (end_time - start_time));
        
        // Выводим результаты
        print_results(*result_add, *result_sub, *result_mul, *result_div, 5);
    }
    
    // Освобождаем память
    free_matrix(local_matrix1.data, local_rows);
    free_matrix(local_matrix2.data, local_rows);
    free_matrix(local_add.data, local_rows);
    free_matrix(local_sub.data, local_rows);
    free_matrix(local_mul.data, local_rows);
    free_matrix(local_div.data, local_rows);
    
    if (rank == 0) {
        free_matrix(matrix1.data, matrix1.rows);
        free_matrix(matrix2.data, matrix2.rows);
        free_matrix(result_add->data, result_add->rows);
        free_matrix(result_sub->data, result_sub->rows);
        free_matrix(result_mul->data, result_mul->rows);
        free_matrix(result_div->data, result_div->rows);
        free(result_add);
        free(result_sub);
        free(result_mul);
        free(result_div);
        free(recv_buf);
    }
    
    free(sendcounts);
    free(displs);
    MPI_Type_free(&row_type);
    
    // Завершаем MPI
    MPI_Finalize();
    
    return 0;
}