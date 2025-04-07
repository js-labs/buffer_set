#include <buffer_set/buffer_set.h>
#include <stdlib.h>
#include <time.h>
#include <search.h>
#include <string.h>
#include "test.h"

#define OPERATIONS 100

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

typedef enum
{
    operation_type_insert,
    operation_type_erase
}
operation_type_t;

struct operation_s
{
    operation_type_t operation_type;
    int value;
};

struct golden_set_s
{
    size_t capacity;
    size_t size;
    int * data;
};

struct golden_set_s * golden_set_create(size_t capacity)
{
    struct golden_set_s * golden_set = malloc(sizeof(struct golden_set_s));
    if (golden_set == NULL)
        return NULL;
    golden_set->capacity = capacity;
    golden_set->size = 0;
    golden_set->data = malloc(sizeof(int*) * capacity);
    if (golden_set->data)
        return golden_set;
    else
    {
        free(golden_set);
        return NULL;
    }
}

static int golden_set_add(struct golden_set_s * golden_set, int value)
{
    if (golden_set->size == golden_set->capacity)
    {
        golden_set->capacity *= 2;
        int * new_data = malloc(sizeof(int) * golden_set->capacity);
        if (!new_data)
            return -1;
        memcpy(new_data, golden_set->data, sizeof(int) * golden_set->size);
        free(golden_set->data);
        golden_set->data = new_data;
    }

    size_t jdx = 0;
    for (; jdx<golden_set->size; jdx++)
    {
        if (golden_set->data[jdx] > value)
        {
            const size_t size = sizeof(int) * (golden_set->size - jdx);
            memmove(&golden_set->data[jdx+1], &golden_set->data[jdx], size);
            golden_set->data[jdx] = value;
            break;
        }
    }

    if (jdx == golden_set->size)
        golden_set->data[golden_set->size] = value;

    golden_set->size++;

    return 0;
}

static void golden_set_destroy(struct golden_set_s * golden_set)
{
    free(golden_set->data);
    free(golden_set);
}

struct check_context_s
{
    struct golden_set_s * golden_set;
    char * error_message;
};

static int check_callback(const void * value, void * arg)
{
    struct check_context_s * check_context = (struct check_context_s*) arg;
    struct golden_set_s * golden_set = (struct golden_set_s*) check_context->golden_set;
    const void * ptr = bsearch(value, golden_set->data, golden_set->size, sizeof(int), int_cmp);
    if (ptr)
        return 0;
    else
    {
        char buffer[128];
        sprintf(buffer, "value %d not found in the golden set", *((const int*)value));
        check_context->error_message = strdup(buffer);
        return -1;
    }
}

char * random_op()
{
    size_t capacity = (OPERATIONS / 10 * 7);
    if (capacity < 16)
        capacity = 16;

    struct operation_s * history = malloc(sizeof(struct operation_s) * OPERATIONS);
    if (history == NULL)
        return NOT_ENOUGH_MEMORY;

    struct golden_set_s * golden_set = golden_set_create(capacity);
    if (golden_set == NULL)
    {
        free(history);
        return NOT_ENOUGH_MEMORY;
    }

    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), capacity, int_cmp);
    char * ret = NULL;

    srand((unsigned int) time(NULL));

    for (int idx=0; idx<OPERATIONS; idx++)
    {
        // Distribute insert operations at 60% for growing
        const int rnd = rand();
        const operation_type_t operation_type = (rnd > (RAND_MAX/10*6))
            ? operation_type_insert
            : operation_type_erase;

        if (operation_type == operation_type_insert)
        {
            int value = rand();
            for (;;)
            {
                void * ptr = bsearch(&value, golden_set->data, golden_set->size, sizeof(int), int_cmp);
                if (ptr == NULL)
                    break;
                value = rand();
            }

            if (golden_set_add(golden_set, value) != 0)
            {
                buffer_set_destroy(buffer_set);
                golden_set_destroy(golden_set);
                free(history);
                return NOT_ENOUGH_MEMORY;
            }

            const struct operation_s operation = { operation_type_insert, value };
            history[idx] = operation;

            int inserted;
            void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
            if (!inserted)
            {
                char buffer[128];
                sprintf(buffer, "unexpected duplicate %d", value);
                ret = strdup(buffer);
                break;
            }
            *((int*)ptr) = value;
        }
        else
        {
        }
    }

    if (!ret)
    {
        struct check_context_s check_context = { golden_set, NULL };
        buffer_set_for_each(buffer_set, check_callback, &check_context);
        ret = check_context.error_message;
        if (!ret)
        {
            for (size_t idx=0; idx<golden_set->size; idx++)
            {
                const int value = golden_set->data[idx];
                const void * ptr = buffer_set_get(buffer_set, &value);
                if (!ptr)
                {
                    char buffer[128];
                    sprintf(buffer, "value %d not found in the buffer set", value);
                    ret = strdup(buffer);
                    break;
                }
            }
        }
    }

    buffer_set_destroy(buffer_set);
    golden_set_destroy(golden_set);
    free(history);

    return ret;
}
