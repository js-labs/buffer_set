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

#if defined(_WIN32)
#include <Windows.h>

static int gettimeofday(struct timeval * tv, struct timezone * tz)
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    ULARGE_INTEGER uli = { .LowPart = ft.dwLowDateTime, .HighPart = ft.dwHighDateTime };
    uli.QuadPart /= 10;
    tv->tv_usec = (uli.QuadPart % 1000000);
    tv->tv_sec = (uli.QuadPart / 1000000) - 11644473600L; // adjust 1601 -> 1970
    return 0;
}

#else
#include <sys/time.h>
#include <search.h>

static int stdlib_cmp(const void* pv1, const void* pv2)
{
    const uintptr_t v1 = (uintptr_t)pv1;
    const uintptr_t v2 = (uintptr_t)pv2;
    if (v1 < v2)
        return -1;
    else if (v2 < v1)
        return 1;
    else
        return 0;
}

static unsigned int test_stdlib()
{
    void* root = NULL;

    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);

    for (uintptr_t idx = 0; idx < COUNT; idx++)
        tsearch((const void*)idx, &root, stdlib_cmp);

    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);

    return elapsed_time(tv_start, tv_end);
}

#endif

static int buffer_set_cmp(const void * pv1, const void * pv2)
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

static unsigned int test_buffer_set()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), COUNT, buffer_set_cmp);
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

    buffer_set_destroy(buffer_set);

    return elapsed_time(tv_start, tv_end);
}

int main(int argc, const char * argv[])
{
    printf("buffer_set: inserted values [0...%u] @ %u usec\n", COUNT-1, test_buffer_set());
#if !defined(_WIN32)
    printf("stdlib: inserted values [0...%u] @ %u usec\n", COUNT-1, test_stdlib());
#endif
    return 0;
}
