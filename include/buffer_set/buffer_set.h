/*
 * Copyright (C) 2025 Sergey Zubarev, info@js-labs.org
 */

#if !defined(BUFFER_SET_H)
#define BUFFER_SET_H

#include <stdint.h>
#include <stdio.h>

typedef struct buffer_set_s buffer_set_t;

buffer_set_t * buffer_set_create(
    size_t value_size,
    uint16_t capacity,
    int (*compar)(const void * v1, const void * v2)
);

void * buffer_set_insert(
    buffer_set_t * buffer_set,
    const void * value,
    int * inserted
);

size_t buffer_set_get_size(
    buffer_set_t * buffer_set
);

void * buffer_set_get(
    buffer_set_t * buffer_set,
    const void * value
);

const void * buffer_set_erase(
    buffer_set_t * buffer_set,
    const void * value
);

int buffer_set_for_each(
    const buffer_set_t * buffer_set,
    int (*func)(const void * value, void * arg),
    void * arg
);

void buffer_set_print_debug(
    const buffer_set_t * buffer_set,
    FILE * file,
    void (*value_printer)(FILE * file, const void * value)
);

void buffer_set_destroy(buffer_set_t * buffer_set);

#endif /* BUFFER_SET_H */

