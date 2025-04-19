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
#include <sys/time.h>
#include <stdlib.h>

static int int_cmp(const void * pv1, const void * pv2)
{
    const int v1 = *((const int*) pv1);
    const int v2 = *((const int*) pv2);
    if (v1 < v2)
        return -1;
    else if (v2 < v1)
        return 1;
    else
        return 0;
}

static unsigned int elapsed_time(struct timeval start, struct timeval end)
{
    return ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
}

#define COUNT (10*1000)

int main(int argc, const char * argv[])
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), COUNT, int_cmp);
    if (buffer_set == NULL)
    {
        printf("not enough memory");
        return -1;
    }

    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);

    for (int idx=0; idx<COUNT; idx++)
    {
        int inserted;
        void * ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        *((int*)ptr) = idx;
    }

    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);

    printf("inserted values [0...%u] @ %u usec\n", COUNT-1, elapsed_time(tv_start, tv_end));

    buffer_set_destroy(buffer_set);

    return 0;
}
