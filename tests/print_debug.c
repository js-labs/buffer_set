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

static void value_printer(FILE * file, const void * value)
{
    fprintf(file, "%d", *((const int*)value));
}

int print_debug()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 0, int_cmp);
    if (buffer_set == NULL)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    int values[] = { 10, 15, 20 };
    for (int idx=0; idx<sizeof(values)/sizeof(values[0]); idx++)
    {
        int value = values[idx];
        int inserted;
        void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
        *((int*)ptr) = value;
    }

    buffer_set_print_debug(buffer_set, stdout, &value_printer);
    buffer_set_destroy(buffer_set);

    return 0;
}
