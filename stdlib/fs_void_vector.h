//
// Created by ChatGPT on 11/10/24.
//

#ifndef FS_VOID_VECTOR_H
#define FS_VOID_VECTOR_H

#define INITIAL_CAPACITY 4
#define MAX_PATH_BUFFER_SIZE 4096
#include <stddef.h>

typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} fs_vector;

// Function to initialize the vector
void vector_init(fs_vector *vector);

// Function to add an element to the vector
void vector_add(fs_vector *vector, void *element);

// Function to free the vector itself
void vector_free(fs_vector *vector);

// Function to free all elements using a custom myfree function
void vector_free_all_elements(fs_vector *vector, void (*myfree)(void *));

// Function to get an element at a specific index
int vector_get(fs_vector *vector, int index, void **out_element);

// Iterator structure
typedef struct {
    fs_vector *vector;
    size_t current;
} VectorIterator;

// Function to initialize the iterator
void vector_iterator_init(VectorIterator *iterator, fs_vector *vector);

// Function to get the next element using the iterator
void *vector_iterator_next(VectorIterator *iterator);

// Function to check if there are more elements in the iterator
int vector_iterator_has_next(VectorIterator *iterator);

void *vector_pop_or_null(fs_vector *vector);


// Definition for fs_string
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} fs_string;

// Function declarations
void fs_string_init(fs_string *str);
void fs_string_reserve(fs_string *str, size_t new_capacity);
void fs_string_append(fs_string *str, const char *suffix);
void fs_string_free(fs_string *str);
char *fs_string_to_cstr(fs_string *str);
fs_string *fs_string_duplicate(const fs_string *original);

#endif //FS_VOID_VECTOR_H
