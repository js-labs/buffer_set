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
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 0, int_cmp);
    if (buffer_set == NULL)
    {
        printf("not enough memory");
        return -1;
    }

    int value = 42;
    int inserted = 0;
    void * ptr1 = buffer_set_insert(buffer_set, &value, &inserted);
    *((int*)ptr1) = value;
    int ret = 0;

    if (inserted == 0)
    {
        printf("unexpected 'inserted' result: %d, it should be non-zero", inserted);
        ret = -1;
    }
    else
    {
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

    buffer_set_destroy(buffer_set);

    return ret;
}
