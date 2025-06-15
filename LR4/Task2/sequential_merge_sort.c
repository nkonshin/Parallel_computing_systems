#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Функция для слияния двух подмассивов
void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Создаем временные массивы
    int L[n1], R[n2];

    // Копируем данные во временные массивы
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Слияние временных массивов обратно в arr[l..r]
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Копируем оставшиеся элементы L[], если они есть
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Копируем оставшиеся элементы R[], если они есть
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

// Итеративная функция сортировки слиянием
void mergeSort(int arr[], int l, int r) {
    if (l >= r) return;
    
    // Выделяем временный массив один раз
    int* temp = (int*)malloc((r - l + 1) * sizeof(int));
    if (!temp) {
        fprintf(stderr, "Ошибка выделения памяти для временного массива\n");
        exit(EXIT_FAILURE);
    }
    
    int curr_size;  // Текущий размер подмассивов для слияния
    int left_start; // Начальный индекс левого подмассива
    
    // Сортируем подмассивы снизу вверх
    for (curr_size = 1; curr_size <= r; curr_size = 2 * curr_size) {
        // Объединяем подмассивы arr[left_start...left_start+curr_size-1] 
        // и arr[left_start+curr_size...left_start+2*curr_size-1]
        for (left_start = l; left_start < r; left_start += 2 * curr_size) {
            int mid = left_start + curr_size - 1;
            int right_end = (left_start + 2 * curr_size - 1) < r ? 
                          (left_start + 2 * curr_size - 1) : r;
            
            if (mid > r) mid = r;
            
            // Объединяем подмассивы
            int i = left_start;
            int j = mid + 1;
            int k = 0;
            
            while (i <= mid && j <= right_end) {
                if (arr[i] <= arr[j]) {
                    temp[k++] = arr[i++];
                } else {
                    temp[k++] = arr[j++];
                }
            }
            
            while (i <= mid) {
                temp[k++] = arr[i++];
            }
            
            while (j <= right_end) {
                temp[k++] = arr[j++];
            }
            
            // Копируем отсортированный подмассив обратно в arr
            for (i = 0; i < k; i++) {
                arr[left_start + i] = temp[i];
            }
        }
    }
    
    free(temp);
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

// Функция для проверки отсортированности массива
int is_sorted(const int* arr, int size) {
    for (int i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    int size;
    clock_t start, end;
    double cpu_time_used;

    // Чтение массива из файла
    int* arr = read_array_from_file(argv[1], &size);

    printf("Сортировка массива из %d элементов...\n", size);
    
    // Замер времени
    start = clock();
    mergeSort(arr, 0, size - 1);
    end = clock();
    
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;

    // Проверка сортировки
    if (is_sorted(arr, size)) {
        printf("Массив успешно отсортирован за %.4f секунд\n", cpu_time_used);
        
        // Вывод первых 10 элементов
        printf("\nПервые 10 элементов:\n");
        for (int i = 0; i < 10 && i < size; i++) {
            printf("%d ", arr[i]);
        }
        
        // Вывод последних 10 элементов
        printf("\n\nПоследние 10 элементов:\n");
        int start = (size > 10) ? size - 10 : 0;
        for (int i = start; i < size; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    } else {
        printf("Ошибка: массив не отсортирован\n");
    }

    // Освобождение памяти
    free(arr);

    return 0;
}