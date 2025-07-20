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

int int_cmp(const void * pv1, const void * pv2)
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

void int_printer(FILE * file, const void * value)
{
    fprintf(file, "%d", *((const int*) value));
}

// Tests
int clear();
int insert();
int max_capacity();
int random_op();
int reg();
int walk();

void run_test(int * failed_tests, const char * name, int (*test_func)())
{
    printf("Run [%s]: ", name);
    int rc = (*test_func)();
    if (rc)
    {
        printf(" / FAILED\n");
        (*failed_tests)++;
    }
    else
        printf(" / PASSED\n");
}

int main(int argc, const char * argv[])
{
    int failed_tests = 0;
    int tests = 0;

#define RUN_TEST(name) run_test(&failed_tests, #name, name); tests++

    RUN_TEST(clear);
    RUN_TEST(insert);
    RUN_TEST(max_capacity);
    RUN_TEST(random_op);
    RUN_TEST(reg);
    RUN_TEST(walk);

#undef RUN_TEST

    if (failed_tests > 0)
    {
        printf("%d out of %d tests failed\n", failed_tests, tests);
        return -1;
    }
    else
        return 0;
}
