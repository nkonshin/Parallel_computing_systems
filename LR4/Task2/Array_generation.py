import random

def generate_array(size, filename):
    print(f"Генерация массива из {size} элементов...")
    arr = [random.randint(1, 1000) for _ in range(size)]
    with open(filename, "w") as f:
        f.write(f"{size}\n")  # Записываем размер массива в первую строку
        f.write(" ".join(map(str, arr)))
    print(f"Массив сохранён в файл {filename}")

# Генерация массивов разного размера
sizes = [1000000, 5000000, 10000000]  # 1M, 5M, 10M элементов
for size in sizes:
    generate_array(size, f"array_{size//1000000}mln.txt")