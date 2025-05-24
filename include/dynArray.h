#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include "numtypes.h"

typedef struct {
    u32 size;
    u32 count;
    void* ptr;
} dyn_array_t;

void dynArrayAlloc(dyn_array_t* arr, u32 size);
void dynArrayGrow(dyn_array_t* arr, u32 newsize);
void dynArrayFree(dyn_array_t* arr);

#endif