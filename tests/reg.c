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
#include "test.h"

static const struct operation_s operations[] =
{
    { operation_type_insert,  6 },
    { operation_type_insert, 17 },
    { operation_type_insert, 10 },
    { operation_type_erase,   6 }
};

int reg()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 16, &int_cmp, NULL);
    if (!buffer_set)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    int inserted;
    void * ptr;

    const size_t count = (sizeof(operations) / sizeof(struct operation_s));
    for (size_t idx=0; idx<count; idx++)
    {
        const struct operation_s * operation = &operations[idx];
        switch (operation->type)
        {
            case operation_type_insert:
                ptr = buffer_set_insert(buffer_set, &operation->value, &inserted);
                *((int*)ptr) = operation->value;
                //printf("*** inserted %d\n", operation->value);
                break;
            case operation_type_erase:
                buffer_set_erase(buffer_set, &operation->value);
                //printf("*** erased %d\n", operation->value);
                break;
        }
        //buffer_set_print_debug(buffer_set, stdout, int_printer);
        //printf("\n");
    }

    buffer_set_destroy(buffer_set);
    return 0;
}
