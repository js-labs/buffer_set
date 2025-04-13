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

char * reg()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 16, int_cmp);
    if (!buffer_set)
        return NOT_ENOUGH_MEMORY;

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
                printf("*** inserted %d\n", operation->value);
                break;
            case operation_type_erase:
                buffer_set_erase(buffer_set, &operation->value);
                printf("*** erased %d\n", operation->value);
                break;
        }
        buffer_set_print_debug(buffer_set, stdout, int_printer);
        printf("\n");
    }

    buffer_set_destroy(buffer_set);
    return NULL;
}
