#include <stddef.h>

#include "library.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

double factorial(int n) {
    if (n == 0 || n == 1) return 1.0;

    double result = 1.0;
    for (int i = 2; i <= n; ++i) result *= i;

    return result;
}

EXPORT float e(int x) {
    if (x < 0) return 0.0f;

    float amount = 0.0f;
    for (int n = 0; n <= x; ++n) amount += 1.0f / factorial(n);

    return amount;
}

void quicksort(int *array, long left, long right) {
    int i = left;
    int j = right;
    int pivot = array[(left + right) / 2];

    while (i <= j) {
        while (array[i] < pivot) ++i;
        while (array[j] > pivot) --j;
        if (i <= j) {
            int tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
            ++i;
            --j;
        }
    }

    if (left < j) quicksort(array, left, j);
    if (i < right) quicksort(array, i, right);
}

EXPORT int *sort(int *array, size_t n) {
    if (!array || n == 0) return array;

    if (n > 1) quicksort(array, 0, (long)n - 1);

    return array;
}
