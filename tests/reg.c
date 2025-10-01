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

// Some simple scenarios helping to debug the implementation

/*
                       50
                     /    \
           25                      75
         /    \                  /    \
     15          35          65          85
    /  \        /  \        /  \        /  \
  12    17    32    37    62    67    82    87
*/

/*
     50            75
    /  \    ==>   /
  25    75       25
*/
static const int test_01_gs[] = { 25, 75, -1 };
static const struct operation_s test_01[] =
{
    { operation_type_insert, 50 },
    { operation_type_insert, 25 },
    { operation_type_insert, 75 },
    { operation_type_erase,  50 },
    { 0, 0 }
};

/*
      50            25
     /  \    ==>   /  \
   25    75       20  75
   /
  20
*/
static const int test_02_gs[] = { 20, 25, 75, -1 };
static const struct operation_s test_02[] = {
    { operation_type_insert, 50 },
    { operation_type_insert, 25 },
    { operation_type_insert, 75 },
    { operation_type_insert, 20 },
    { operation_type_erase,  50 },
    { 0, 0 }
};

/*
        50                   50
      /    \               /    \
    25      75    ==>    20      75
   /  \    /  \         /  \    /  \
  20  30  70  80       19  30  70  80
 /
19
*/
static const int test_03_gs[] = { 19, 20, 30, 50, 70, 75, 80, -1 };
static const struct operation_s test_03[] = {
    { operation_type_insert, 50 },
    { operation_type_insert, 25 },
    { operation_type_insert, 75 },
    { operation_type_insert, 20 },
    { operation_type_insert, 30 },
    { operation_type_insert, 70 },
    { operation_type_insert, 80 },
    { operation_type_insert, 19 },
    { operation_type_erase,  25 },
    { 0, 0 }
};

/*
         50                    50
      /      \               /    \
    25        75    ==>    30      75
   /  \      /  \         /  \    /  \
  20  30    70  80       20  31  70  80
        \
        31
*/
static const int test_04_gs[] = { 20, 30, 31, 50, 70, 75, 80, -1 };
static const struct operation_s test_04[] = {
    { operation_type_insert, 50 },
    { operation_type_insert, 25 },
    { operation_type_insert, 75 },
    { operation_type_insert, 20 },
    { operation_type_insert, 30 },
    { operation_type_insert, 70 },
    { operation_type_insert, 80 },
    { operation_type_insert, 31 },
    { operation_type_erase,  25 },
    { 0, 0 }
};

/*
               50                             25
           /        \                      /      \
        25             75              20            50
      /    \          /  \     ==>   /    \        /    \
    20      30       70   80        19    21     30      70
   /  \    /  \     /              /            /  \    /  \
  19  21  29  31   69             15           29  31  69  80
 /
15
*/
static const int test_05_gs[] = { 15, 19, 20, 21, 25, 29, 30, 31, 50, 69, 70, 80, -1 };
static const struct operation_s test_05[] = {
    { operation_type_insert, 50 },
    { operation_type_insert, 25 },
    { operation_type_insert, 75 },
    { operation_type_insert, 20 },
    { operation_type_insert, 30 },
    { operation_type_insert, 70 },
    { operation_type_insert, 80 },
    { operation_type_insert, 19 },
    { operation_type_insert, 21 },
    { operation_type_insert, 29 },
    { operation_type_insert, 31 },
    { operation_type_insert, 69 },
    { operation_type_insert, 15 },
    { operation_type_erase,  75 },
};

static const int test_06_gs[] = { 42, 43, 63, 79, 149, 210, -1 };
static const struct operation_s test_06[] =
{
    { operation_type_insert,  42 },
    { operation_type_insert,  43 },
    { operation_type_insert,  12 },
    { operation_type_insert, 210 },
    { operation_type_insert, 149 },
    { operation_type_insert,  79 },
    { operation_type_insert,  63 },
    { operation_type_erase,   12 },
    { 0, 0 }
};

static const int test_07_gs[] = { 4, 26, 133, 175, 208, -1 };
static const struct operation_s test_07[] =
{
    { operation_type_insert, 208 },
    { operation_type_insert,   4 },
    { operation_type_insert,  41 },
    { operation_type_insert, 175 },
    { operation_type_insert,  26 },
    { operation_type_insert, 133 },
    { operation_type_erase,   41 },
    { 0, 0 }
};

static const int test_08_gs[] = { 15, 22, 25, -1 };
static const struct operation_s test_08[] =
{
    { operation_type_insert, 20 },
    { operation_type_insert, 15 },
    { operation_type_insert, 25 },
    { operation_type_insert, 22 },
    { operation_type_erase,  20 },
    { 0, 0 }
};

static const int test_09_gs[] = { 20, 49, 52, 114, 118, 132, 177, -1 };
static const struct operation_s test_09[] =
{
/*
     52                     52
   /    \                 /    \
  49     137            49     118
 /     /    \    ==>   /      /   \
20   118    177       20    114   177
    /   \                        /
   114  132                    132
*/
    { operation_type_insert,  52 },
    { operation_type_insert,  49 },
    { operation_type_insert, 137 },
    { operation_type_insert,  20 },
    { operation_type_insert, 118 },
    { operation_type_insert, 177 },
    { operation_type_insert, 114 },
    { operation_type_insert, 132 },
    { operation_type_erase,  137 },
    { 0, 0 }
};

static const int test_10_gs[] = { 20, 26, 138, 148, 184, 203, -1 };
static const struct operation_s test_10[] =
{
    { operation_type_insert,  38 },
    { operation_type_insert,  20 },
    { operation_type_insert, 184 },
    { operation_type_insert,  26 },
    { operation_type_insert, 138 },
    { operation_type_insert, 203 },
    { operation_type_insert, 148 },
    { operation_type_erase,   38 },
    { 0, 0 }
};

static int run_test(const struct operation_s * operations, const int * golden_set)
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 16, &int_cmp, NULL);
    if (!buffer_set)
    {
        printf("buffer_set_create() failed");
        return -1;
    }

    int rc = 0;
    int inserted;
    void * ptr;

    for (const struct operation_s * operation = operations; operation->type; operation++)
    {
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
        rc = buffer_set_verify(buffer_set, stdout);
        if (rc != 0)
            break;
    }

    //buffer_set_print_debug(buffer_set, stdout, int_printer);
    //printf("\n");

    if (rc == 0)
    {
        buffer_set_iterator_t * it = buffer_set_begin(buffer_set);
        buffer_set_iterator_t * it_end = buffer_set_end(buffer_set);
        for (;;)
        {
            int expected_value = *golden_set;
            if (expected_value == -1)
            {
                if (it == it_end)
                    return 0;
                else
                {
                    printf(
                        "unexpected value %d instead of <end>\n",
                        *((const int*) buffer_set_get_at(buffer_set, it))
                    );
                    rc = -1;
                    break;
                }
            }

            if (it == it_end)
            {
                printf("unexpected <end> instead of %d\n", expected_value);
                rc = -1;
                break;
            }

            const int value = *((const int*) buffer_set_get_at(buffer_set, it));
            if (value != expected_value)
            {
                printf("got %d instead of expected %d\n", value, expected_value);
                rc = -1;
                break;
            }

            it = buffer_set_iterator_next(buffer_set, it);
            golden_set++;
        }
    }

    buffer_set_destroy(buffer_set);
    return rc;
}

int reg()
{
#define RUN_TEST(t) \
    if (run_test(test_##t, test_##t##_gs) != 0) \
    { \
        printf("test %s failed\n", #t); \
        return -1; \
    }

    RUN_TEST(01)
    RUN_TEST(02)
    RUN_TEST(03)
    RUN_TEST(04)
    RUN_TEST(05)
    RUN_TEST(06)
    RUN_TEST(07)
    RUN_TEST(08)
    RUN_TEST(09)
    RUN_TEST(10)

#undef RUN_TEST

    return 0;
}
