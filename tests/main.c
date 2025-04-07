#include <buffer_set/buffer_set.h>
#include <stdlib.h>
#include "test.h"

static void debug_value_printer(FILE * file, const void * value)
{
    const int v = *((const int*) value);
    fprintf(file, "%d", v);
}

// Tests
char * random_op();

struct failed_test_s
{
    struct  failed_test_s * next;
    const char * name;
    char * error_message;
};

static struct failed_test_s * run_test(
    struct failed_test_s * failed_tests,
    const char * name,
    char * (*test_func)()
) {
    printf("[ RUN    ] %s\n", name);
    char * error_message = (*test_func)();
    if (error_message)
    {
        printf("[ FAILED ] %s\n", name);
        struct failed_test_s * ft = malloc(sizeof(struct failed_test_s));
        ft->next = failed_tests;
        ft->name = name;
        ft->error_message = error_message;
        return ft;
    }
    else
    {
        printf("[     OK ] %s\n", name);
        return failed_tests;
    }
}

int main(int argc, const char * argv[])
{
    struct failed_test_s * failed_tests = NULL;

#define RUN_TEST(name) failed_tests = run_test(failed_tests, #name, name)

    RUN_TEST(random_op);

#undef RUN_TEST

    if (failed_tests == NULL)
        return 0;

    printf("Failed tests:\n");
    struct failed_test_s * ft = failed_tests;
    for (;;)
    {
        struct failed_test_s * next = failed_tests->next;

        printf("%s: ", ft->name);
        if (ft->error_message == NOT_ENOUGH_MEMORY)
            printf("not enough memory\n");
        else
        {
            printf(ft->error_message);
            free(ft->error_message);
        }
        free(ft);
        if (next == NULL)
            break;
        ft = next;
    }

    return -1;
}
