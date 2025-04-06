#include <buffer_set/buffer_set.h>
#include <stdlib.h>

static int int_less(const void * pv1, const void * pv2)
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

static void debug_value_printer(FILE * file, const void * value)
{
    const int v = *((const int*) value);
    fprintf(file, "%d", v);
}

int main(int argc, const char * argv[])
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 32, int_less);
    int inserted;
    int value;
    void * ptr;

    for (int idx=1; idx<20; idx++)
    {
        value = idx;
        ptr = buffer_set_insert(buffer_set, &value, &inserted);
        *((int*)ptr) = idx;
    }

    buffer_set_print_debug(buffer_set, stdout, debug_value_printer);
    printf("\n");
    return 0;
}
