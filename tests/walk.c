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

/**
 * Check if buffer_set_walk() traverses the set in ascending order,
 * starting from the minimum value.
 */

#define COUNT 20

static int walk_callback(const void * value, void * arg)
{
    int last_value = *((int*) arg);
    int current_value = *((const int*)value);
    if ((last_value + 1) == current_value)
    {
        *((int*)arg) = current_value;
        return 0;
    }
    else
    {
        printf("unexpectedly got %d instead of %d", current_value, last_value+1);
        return -1;
    }
}

int walk()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), COUNT, &int_cmp, NULL);
    if (buffer_set == NULL)
    {
        printf("not enough memory");
        return -1;
    }

    int ret = 0;

    for (int idx=1; idx<COUNT; idx++)
    {
        int inserted;
        void * ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        if (!ptr)
        {
            printf("buffer_set_insert() unexpectedly returned NULL for %d", idx);
            ret = -1;
            break;
        }
        *((int*)ptr) = idx;
    }

    if (!ret)
    {
        int last_value = 0;
        ret = buffer_set_walk(buffer_set, walk_callback, &last_value);
    }

    buffer_set_destroy(buffer_set);

    return ret;
}
