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

int clear()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 16, &int_cmp, NULL, NULL);
    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    for (int idx=5; idx<10; idx++)
    {
        int inserted = 0;
        void * ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        *((int*)ptr) = idx;
    }

    buffer_set_clear(buffer_set);

    for (int idx=1; idx<16; idx++)
    {
        int inserted = 0;
        void* ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        *((int*)ptr) = idx;
    }

    buffer_set_destroy(buffer_set);

    return 0;
}
