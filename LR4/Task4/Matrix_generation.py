import random
import math

rows, cols = 2000, 2000

# Создаем двумерный массив
matrix = [[random.randint(1, 1000) for _ in range(cols)] for _ in range(rows)]

# Сохраняем матрицу в файл
with open("matrix2.txt", "w") as f:
    # Сначала записываем размеры матрицы
    f.write(f"{rows} {cols}\n")
    
    # Затем записываем саму матрицу
    for row in matrix:
        f.write(" ".join(map(str, row)) + "\n")

print(f"Матрица {rows}x{cols} (всего {rows*cols} элементов) сохранена в файл matrix2.txt")
