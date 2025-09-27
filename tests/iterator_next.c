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

#include "buffer_set/buffer_set.h"
#include <stdlib.h>
#include <string.h>
#include "test.h"

#define COUNT 20

int iterator_next()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), COUNT, int_cmp);
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

    //buffer_set_print_debug(buffer_set, stdout, &int_printer);
    //printf("\n");

    if (!ret)
    {
        int expected_value = 1;
        buffer_set_iterator_t * it = buffer_set_begin(buffer_set);
        buffer_set_iterator_t * it_end = buffer_set_end(buffer_set);
        while (it != it_end)
        {
            const int value = * (const int*) buffer_set_get_at(buffer_set, it);
            if (value != expected_value)
            {
                printf("got %d instead of %d\n", value, expected_value);
                ret = -1;
                break;
            }
            expected_value++;
            it = buffer_set_iterator_next(buffer_set, it);
        }
    }

    buffer_set_destroy(buffer_set);

    return ret;
}
