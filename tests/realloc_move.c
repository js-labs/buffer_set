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

#include <buffer_set/buffer_set.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

struct value_s
{
    int key;
    void * ptr;
};

static int value_cmp(const void * v1, const void * v2, void * thunk)
{
    const struct value_s * value1 = v1;
    const struct value_s * value2 = v2;
    if (value1->key < value2->key)
        return -1;
    else if (value2->key < value1->key)
        return 1;
    else
        return 0;
}

static void value_move(void * dst, void * src, void * thunk)
{
    struct value_s * dst_value = dst;
    struct value_s * src_value = src;
    dst_value->key = src_value->key;
    dst_value->ptr = dst;
    *((int*)thunk) = 1;
}

int realloc_move()
{
    int move_called = 0;
    buffer_set_t * buffer_set = buffer_set_create(
        sizeof(struct value_s),
        0,
        &value_cmp,
        &value_move,
        &move_called
    );

    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    // first buffer capacity is 16
    for (int idx=0; idx<20; idx++)
    {
        int inserted = 0;
        struct value_s * value = buffer_set_insert(buffer_set, &idx, &inserted);
        value->key = idx;
        value->ptr = value;
    }

    buffer_set_iterator_t * it = buffer_set_begin(buffer_set);
    buffer_set_iterator_t * it_end = buffer_set_end(buffer_set);
    int rc = 0;

    while (it != it_end)
    {
        struct value_s * value = buffer_set_get_at(buffer_set, it);
        if (value->ptr != value)
            rc = -1;
        it = buffer_set_iterator_next(buffer_set, it);
    }

    if (!move_called)
        rc = -1;

    buffer_set_destroy(buffer_set);

    return rc;
}
