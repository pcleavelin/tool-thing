#ifndef ED_ARRAY_INCLUDED
#define ED_ARRAY_INCLUDED
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define array(T) struct T##_Array
#define arrayTemplate(T)                                                       \
    array(T) {                                                                 \
        size_t size, capacity;                                                 \
        T *data;                                                               \
    };                                                                         \
    array(T) T##_ArrayConstruct(size_t size) {                                 \
        array(T) this;                                                         \
        this.size = 0;                                                         \
        this.capacity = size;                                                  \
        this.data = malloc(size * sizeof(T));                                  \
        if (!this.data) {                                                      \
            assert("failed to allocate memory for array");                     \
        }                                                                      \
        return this;                                                           \
    };                                                                         \
    void T##_PushArray(array(T) * arr, T value) {                              \
        if (arr->size == arr->capacity) {                                      \
            arr->capacity *= 2;                                                \
            void *new_data = realloc(arr->data, arr->capacity * sizeof(T));    \
            if (new_data == NULL) {                                            \
                fprintf(stderr, "out of memory when reallocating array\n");    \
            }                                                                  \
            arr->data = new_data;                                              \
        }                                                                      \
        if (arr->size + 1 <= arr->capacity) {                                  \
            arr->data[arr->size] = value;                                      \
            arr->size += 1;                                                    \
        }                                                                      \
    };                                                                         \
    void T##_PushArrayMulti(array(T) * arr, T * values, size_t len) {          \
        for (size_t i = 0; i < len; ++i) {                                     \
            T##_PushArray(arr, values[i]);                                     \
        }                                                                      \
    };                                                                         \
    void T##_InsertArrayAt(array(T) * arr, size_t loc, T value) {              \
        if (arr->size == arr->capacity) {                                      \
            arr->capacity *= 2;                                                \
            void *new_data = realloc(arr->data, arr->capacity * sizeof(T));    \
            if (new_data == NULL) {                                            \
                fprintf(stderr, "out of memory when reallocating array\n");    \
            }                                                                  \
            arr->data = new_data;                                              \
        }                                                                      \
        memcpy(&arr->data[loc + 1], &arr->data[loc], arr->size * sizeof(T));   \
        arr->data[loc] = value;                                                \
        arr->size += 1;                                                        \
    };

#define slice(T) struct T##_Slice
#define sliceTemplate(T)                                                       \
    slice(T) {                                                                 \
        size_t len;                                                            \
        T *data;                                                               \
    };                                                                         \
    array(T) T##_FromSlice(slice(T) s) {                                       \
        array(T) arr = T##_ArrayConstruct(s.len);                              \
        memcpy(arr.data, s.data, sizeof(T) * s.len);                           \
        arr.size = s.len;                                                      \
        return arr;                                                            \
    }                                                                          \
    slice(T) T##_SliceConstruct(void *data, size_t len) {                      \
        slice(T) this;                                                         \
        this.len = len;                                                        \
        this.data = data;                                                      \
        return this;                                                           \
    }

#define newArray(T, size) T##_ArrayConstruct(size)
#define newArrayFromSlice(T, s) T##_FromSlice(s)
#define pushArray(T, arr, value) T##_PushArray(arr, (value))
#define pushArrayMulti(T, arr, values, len) T##_PushArrayMulti(arr, values, len)

#define insertArrayAt(T, arr, loc, value) T##_InsertArrayAt(arr, loc, (value))

#define newSlice(T, data, len) T##_SliceConstruct(data, len)

arrayTemplate(uint8_t);
sliceTemplate(uint8_t);
arrayTemplate(uint32_t);
sliceTemplate(uint32_t);

#endif
