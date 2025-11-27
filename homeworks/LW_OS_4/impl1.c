#include <stddef.h>

#include "library.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT float e(int x) {
    if (x <= 0) return 0.0f;

    float base = 1.0f + 1.0f / (float)x;
    float result = 1.0f;

    for (int i = 0; i < x; ++i) {
        result *= base;
    }

    return result;
}

EXPORT int *sort(int *array, size_t n) {
    if (!array || n == 0) return array;

    for (size_t i = 0; i + 1 < n; ++i) {
        for (size_t j = 0; j + 1 < n - i; ++j) {
            if (array[j] > array[j + 1]) {
                int tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
            }
        }
    }

    return array;
}
