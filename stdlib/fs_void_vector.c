//
// Created by ChatGPT on 11/10/24.
//

#include "fs_void_vector.h"
#include "fs_malloc.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void vector_init(fs_vector *vector) {
    // printf("Initializing vector\n");
    vector->data = malloc(sizeof(void *) * INITIAL_CAPACITY);
    if (!vector->data) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    vector->size = 0;
    vector->capacity = INITIAL_CAPACITY;
}

void vector_add(fs_vector *vector, void *element) {
    // printf("vector add %lu element: %p\n", vector->size, element);
    if (vector->size == vector->capacity) {
        vector->capacity *= 2;
        vector->data = fs_xrealloc(vector->data, sizeof(void *) * vector->capacity);
    }
    vector->data[vector->size++] = element;
}

void *vector_pop_or_null(fs_vector *vector) {
    void *element = NULL;
    if (vector->size > 0) {
        element = vector->data[vector->size - 1];
        vector->size--;
    }
    return element;
}

void vector_free(fs_vector *vector) {
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void vector_free_all_elements(fs_vector *vector, void (*myfree)(void *)) {
    for (size_t i = 0; i < vector->size; i++) {
        myfree(vector->data[i]);
    }
    vector->size = 0;
}

int vector_get(fs_vector *vector, int index, void **out_element) {
    if (index < 0 || (size_t) index >= vector->size) {
        return -1;
    }
    *out_element = vector->data[index];
    return 0;
}

void vector_iterator_init(VectorIterator *iterator, fs_vector *vector) {
    iterator->vector = vector;
    iterator->current = 0;
}

void *vector_iterator_next(VectorIterator *iterator) {
    if (iterator->current >= iterator->vector->size) { return NULL; }
    return iterator->vector->data[iterator->current++];
}

int vector_iterator_has_next(VectorIterator *iterator) { return iterator->current < iterator->vector->size; }

void fs_string_init(fs_string *str) {
    str->capacity = MAX_PATH_BUFFER_SIZE;
    str->data = fs_xmalloc(str->capacity);
    str->length = 0;
}

void fs_string_reserve(fs_string *str, size_t new_capacity) {
    if (str->capacity >= new_capacity) {
        return;
    }
    str->data = fs_xrealloc(str->data, new_capacity);
    str->capacity = new_capacity;
}

void fs_string_append(fs_string *str, const char *suffix) {
    size_t suffix_len = strlen(suffix);
    int need_realloc = 0;
    while (str->length + suffix_len > str->capacity) {
        str->capacity = str->capacity * 2;
        need_realloc = 1;
    }
    if (need_realloc) { str->data = fs_xrealloc(str->data, str->capacity); }
    strcpy(str->data + str->length, suffix);
    str->length += suffix_len;
}

void fs_string_free(fs_string *str) {
    free(str->data);
    str->data = NULL;
    str->length = 0;
    str->capacity = 0;
    free(str);
}

char *fs_string_to_cstr(fs_string *str) {
    char *new_str = fs_xmalloc(str->length + 1);
    memcpy(new_str, str->data, str->length);
    new_str[str->length] = '\0';
    return new_str;
}

fs_string *fs_string_duplicate(const fs_string *original) {
    fs_string *copy = (fs_string *) fs_xmalloc(sizeof(fs_string));
    fs_string_init(copy);
    fs_string_append(copy, original->data);
    copy->length = original->length;
    return copy;
}
