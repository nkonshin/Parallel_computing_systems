import numpy as np
from sum_omp import parallel_array_sum
import time

# Чтение массива (предположим, сохранён как строки чисел в array.txt)
arr = np.loadtxt("array.txt")

start = time.time()
total = parallel_array_sum(arr)
end = time.time()

print(f"Сумма элементов: {total}")
print(f"Execution time: {end - start:.6f} seconds")
