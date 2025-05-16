import random

# Генерируем массив из 100001 случайного числа от 1 до 100
arr = [random.randint(1, 100) for _ in range(100001)]

# Сохраняем массив в файл
with open("array.txt", "w") as f:
    f.write(" ".join(map(str, arr)))

print("Массив сохранён в файл array.txt")
