/*
 * Copyright (C) 2025 Sergey Zubarev, info@js-labs.org
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
char * max_capacity();
char * random_op();
char * reg();

void run_test(
    int * failed_tests,
    const char * name,
    char * (*test_func)()
) {
    printf("[ RUN    ] %s", name);
    char * error_message = (*test_func)();
    if (error_message)
    {
        printf("[ FAILED ] %s: %s\n", name, error_message);
        free(error_message);
        (*failed_tests)++;
    }
    else
        printf("[     OK ] %s\n", name);
}

int main(int argc, const char * argv[])
{
    int failed_tests = 0;

#define RUN_TEST(name) run_test(&failed_tests, #name, name)

    RUN_TEST(max_capacity);
    RUN_TEST(random_op);
    //RUN_TEST(reg);

#undef RUN_TEST

    return failed_tests ? -1 : 0;
}
