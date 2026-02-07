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

int shrink()
{
    buffer_set_t* buffer_set = buffer_set_create(sizeof(int), 0, &int_cmp, NULL, NULL);
    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    for (int idx=0; idx<63; idx++)
    {
        int inserted = 0;
        void * ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        *((int*)ptr) = idx;
    }

    for (int idx=0; idx<50; idx++)
        buffer_set_erase(buffer_set, &idx);

    buffer_set_shrink(buffer_set);

    int rc = 0;
    buffer_set_iterator_t * it = buffer_set_begin(buffer_set);

    for (int idx=50; idx<63; idx++)
    {
        if (it == buffer_set_end(buffer_set))
        {
            fprintf(stderr, "unexpected iterator end\n");
            rc = -1;
            break;
        }

        int * value = buffer_set_get_at(buffer_set, it);
        if (*value != idx)
        {
            fprintf(stderr, "got %d instead of expected %d\n", *value, idx);
            rc = -1;
            break;
        }

        it = buffer_set_iterator_next(buffer_set, it);
    }

    if (it != buffer_set_end(buffer_set))
    {
        fprintf(stderr, "iterator unexpectedly not end\n");
        rc = -1;
    }

    buffer_set_destroy(buffer_set);

    return rc;
}
