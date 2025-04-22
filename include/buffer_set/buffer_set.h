/*
 * This file is part of BUFFER_SET library.
 * Copyright (C) 2020 Sergey Zubarev, info@js-labs.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 */

#if !defined(BUFFER_SET_H)
#define BUFFER_SET_H

#include <stdint.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct buffer_set_s buffer_set_t;

buffer_set_t * buffer_set_create(
    size_t value_size,
    uint16_t capacity,
    int (*compar)(const void * v1, const void * v2)
);

/**
 * Inserts a value into the set.
 *
 * @return
 * A pointer to the value in the set, or NULL if
 * an error occurred (e.g., memory allocation failure).
 *
 * @note
 * The function uses the provided value only for searching within the buffer set. 
 * If the value does not exist in the set, a new node will be allocated from the buffer,
 * but it is the caller's responsibility to properly initialize the value.
 *
 * For example, if the buffer set is intended to store integers, the insertion 
 * should be performed as follows:
 *
 *    int value = 42;
 *    int inserted;
 *    void *ptr = buffer_set_insert(buffer_set, &value, &inserted);
 *    if (inserted) {
 *        *((int *)ptr) = value;
 *    }
 *
 * The address of the value inside the buffer is always aligned to the size of a pointer.
 */
void * buffer_set_insert(
    buffer_set_t * buffer_set,
    const void * value,
    int * inserted
);

uint16_t buffer_set_get_size(
    buffer_set_t * buffer_set
);

void * buffer_set_get(
    buffer_set_t * buffer_set,
    const void * value
);

/**
 * Erase the value from the set.
 *
 * @return
 * A pointer to the value in the set that was erased, or NULL if the value was not found.
 * The erased value can be accessed until a subsequent insertion operation reuses the node.
 */
const void * buffer_set_erase(
    buffer_set_t * buffer_set,
    const void * value
);

/**
 * The buffer_set_walk() function iterates through the set
 * and invokes the action function for each value.
 * The action function is called with the value and the arg parameter,
 * which is provided as the third argument to buffer_set_walk().
 * If the action function returns a value other than 0, buffer_set_walk()
 * stops the traversal and returns the value returned by action.
 */
int buffer_set_walk(
    const buffer_set_t * buffer_set,
    int (*action)(const void * value, void * arg),
    void * arg
);

void buffer_set_print_debug(
    const buffer_set_t * buffer_set,
    FILE * file,
    void (*value_printer)(FILE * file, const void * value)
);

void buffer_set_destroy(buffer_set_t * buffer_set);

#if defined(__cplusplus)
}
#endif

#endif /* BUFFER_SET_H */
