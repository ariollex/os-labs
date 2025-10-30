#include <stdio.h>

#define UINT128_MAX  ((__uint128_t)-1)
#define INT128_MAX   ((__int128_t)(UINT128_MAX >> 1))
#define INT128_MIN   (-INT128_MAX - 1)

int GetDecimalValue(char number) {
    if ('0' <= number && number <= '9') return number - '0';
    if ('A' <= number && number <= 'Z') return number - 'A' + 10;
    if ('a' <= number && number <= 'z') return number - 'a' + 10;
    return -1;
}

__int128_t toDecimal(const char* number, char** endptr, unsigned const int base) {
    int sign = 1;
    if (*number == '-') { sign = -1; ++number; }
    __int128_t res = 0;
    __uint128_t umax_pos = (__uint128_t)INT128_MAX;
    __uint128_t umax_neg = ((__uint128_t)INT128_MAX + 1);

    while (*number != '\0') {
        int decimalValue = GetDecimalValue(*number++);
        if (decimalValue < 0 || decimalValue >= base) break;

        __int128_t limit = (sign > 0) ? umax_pos : umax_neg;
        if (res > (limit - decimalValue) / base) {
            if (endptr) *endptr = (char*)number;
            return res;
        }
        res = res * base + decimalValue;
    }
    if(endptr) *endptr = (char*)number;
    return res * sign;
}

void reverse(char* arr, const int length) {
    char temp;
    for(int i = 0; i < length / 2; ++i) {
        temp = arr[i] ;
        arr[i] = arr[length - 1 - i];
        arr[length - 1 - i] = temp;
    }
}

static size_t int128ToString(char *res, const __int128_t number) {
    __int128_t n = number;
    if (number < 0) n = ~n + 1;
    int index = 0;
    res[index++] = '\0';
    while (n != 0) {
        res[index++] = '0' + n % 10;
        n /= 10;
    }
    if (number < 0) res[index++] = '-';
    if (!number) res[index++] = '0';

    reverse(res, index);
    return index - 1;
}
