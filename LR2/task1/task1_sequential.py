import random
import timeit

# Чтение массива из файла
with open("array.txt", "r") as f:
    arr = list(map(int, f.read().split()))

def sequential_array_sum(arr):
    total = 0
    for i in arr:
        total += i
    return total

# запуск 5 раз, возвращает среднее время одного запуска
avg_time = timeit.timeit(
    stmt='sequential_array_sum(arr)',
    globals=globals(),
    number=5
) / 5

print(f"Сумма элементов = {sequential_array_sum(arr)} \nСреднее время выполнения (5 запусков): {avg_time:.6f} сек")