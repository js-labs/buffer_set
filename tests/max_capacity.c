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

int max_capacity()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 0, int_cmp);
    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    int ret = 0;

    for (int idx=0; idx<0xFFFE; idx++)
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

    int inserted;
    int value = 0xFFFF;
    void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
    if (ptr)
    {
        printf("buffer_set_insert() unexpectedly returned non NULL value");
        ret = -1;
    }

    buffer_set_destroy(buffer_set);

    return ret;
}
