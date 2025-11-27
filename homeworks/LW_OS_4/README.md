# Библиотеки 
cc -fPIC -shared impl1.c -o libimpl1.so -lm
cc -fPIC -shared impl2.c -o libimpl2.so

gcc main_static.c impl1.c -o main_static -lm

# Программа №2
cc main_dynamic.c -ldl -o main_dynamic

# Программа с линковкой
./main_static
# ввод:
# 1 5
# 2 5 4 1 3 2 0 // Сначала номер функции, потом размер и токо потом массив
# ...

# Программа с dlopen (две реализации и переключение 0)
./main_dynamic ./libimpl1.so ./libimpl2.so
# пример:
# 1 5
# 0         # переключились на вторую библиотеку
# 1 5
# 2 5 4 1 3 2 0