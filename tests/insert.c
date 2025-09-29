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

int insert()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 5, &int_cmp, NULL);
    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    int ret = 0;
    for (int value=1; value<1000; value++)
    {
        int inserted = 0;
        void * ptr1 = buffer_set_insert(buffer_set, &value, &inserted);
        if (inserted == 0)
        {
            printf("unexpected 'inserted' result: %d, it should be non-zero", inserted);
            ret = -1;
        }
        else
        {
            *((int*)ptr1) = value;
            inserted = 1;
            void * ptr2 = buffer_set_insert(buffer_set, &value, &inserted);
            if (inserted == 0)
            {
                if (ptr1 != ptr2)
                {
                    printf("different value address on update");
                    ret = -1;
                }
            }
            else
            {
                printf("unexpected 'inserted' result: %d, it should be zero", inserted);
                ret = -1;
            }
        }
    }

    buffer_set_iterator_t * it = buffer_set_begin(buffer_set);
    buffer_set_iterator_t * it_end = buffer_set_end(buffer_set);
    while (it != it_end)
    {
        const int value = *((const int*) buffer_set_get_at(buffer_set, it));
        it = buffer_set_iterator_next(buffer_set, it);
    }

    buffer_set_destroy(buffer_set);

    return ret;
}
