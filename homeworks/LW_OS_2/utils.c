#include <stdio.h>

int GetDecimalValue(char number) {
    if ('0' <= number && number <= '9') return number - '0';
    if ('A' <= number && number <= 'Z') return number - 'A' + 10;
    if ('a' <= number && number <= 'z') return number - 'a' + 10;
    return -1;
}

__uint128_t toDecimal(const char* number, char** endptr, unsigned const int base) {
    __uint128_t res = 0;
    while (*number != '\0') {
        int decimalValue = GetDecimalValue(*number++);
        if (decimalValue < 0 || decimalValue >= base) break;
        res = res * base + decimalValue;
    }
    if(endptr) *endptr = (char*)number;
    return res;
}

void reverse(char* arr, const int length) {
    char temp;
    for(int i = 0; i < length / 2; ++i) {
        temp = arr[i] ;
        arr[i] = arr[length - 1 - i];
        arr[length - 1 - i] = temp;
    }
}

static size_t int128ToString(char *res, const __uint128_t number) {
    __uint128_t n = number;
    int index = 0;
    res[index++] = '\0';
    while (n != 0) {
        res[index++] = '0' + n % 10;
        n /= 10;
    }
    if (!number) res[index++] = '0';
    reverse(res, index);
    return index - 1;
}
