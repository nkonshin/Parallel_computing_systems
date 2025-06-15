import random

# Генерируем массив из 5000000 случайного числа от 1 до 1000
arr = [random.randint(1, 1000) for _ in range(5000000)]

# Сохраняем массив в файл
with open("array.txt", "w") as f:
    f.write(" ".join(map(str, arr)))

print("Массив сохранён в файл array.txt")
