#include <buffer_set/buffer_set.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

char * max_capacity()
{
    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 0, int_cmp);
    if (buffer_set == NULL)
        return NOT_ENOUGH_MEMORY;

    char * ret = NULL;

    for (int idx=0; idx<0xFFFE; idx++)
    {
        int inserted;
        void * ptr = buffer_set_insert(buffer_set, &idx, &inserted);
        if (!ptr)
        {
            char buffer[128];
            sprintf(buffer, "buffer_set_insert() unexpectedly returned NULL for %d", idx);
            ret = strdup(buffer);
            break;
        }
        *((int*)ptr) = idx;
    }

    int inserted;
    int value = 0xFFFF;
    void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
    if (ptr)
        ret = strdup("buffer_set_insert() unexpectedly returned non NULL value");

    buffer_set_destroy(buffer_set);

    return ret;
}
