#include <stdlib.h>

#include "dynArray.h"

void dynArrayAlloc(dyn_array_t* arr, u32 size) {
    arr->ptr = malloc(size);
    arr->size = size;
}

void dynArrayGrow(dyn_array_t* arr, u32 newsize) {
    arr->ptr = realloc(arr->ptr, newsize);
    arr->size = newsize;
}

void dynArrayFree(dyn_array_t* arr) {
    free(arr->ptr);
}