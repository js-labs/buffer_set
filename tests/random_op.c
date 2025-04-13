#include <buffer_set/buffer_set.h>
#include <stdlib.h>
#include <time.h>
#include <search.h>
#include <string.h>
#include "test.h"

#define OPERATIONS 100

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

struct validation_context_s
{
    const struct golden_set_s * golden_set;
    const struct operation_s * history;
    char * error_message;
};

static void print_operations(const struct operation_s * operations, size_t count)
{
    printf("\n");
    printf("static const struct operation_s operations[] =\n");
    printf("{\n");
    for (size_t idx=0; idx<count; idx++)
    {
        const struct operation_s * operation = &operations[idx];
        const char * operation_type;
        switch (operation->type)
        {
            case operation_type_insert:
                operation_type = "operation_type_insert";
                break;
            case operation_type_erase:
                operation_type = "operation_type_erase";
                break;
        }
        printf("    { %s, %d }\n", operation_type, operation->value);
    }
    printf("}\n");
}

static int validation_callback(const void * value, void * arg)
{
    struct validation_context_s * validation_context = (struct validation_context_s*) arg;
    struct golden_set_s * golden_set = (struct golden_set_s*) validation_context->golden_set;
    const void * ptr = bsearch(value, golden_set->data, golden_set->size, sizeof(int), int_cmp);
    if (ptr)
        return 0;
    else
    {
        char buffer[128];
        sprintf(buffer, "value %d not found in the golden set", *((const int*)value));
        validation_context->error_message = strdup(buffer);
        print_operations(validation_context->history, OPERATIONS);
        return -1;
    }
}

char * random_op()
{
    size_t capacity = (OPERATIONS / 2);
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
    int insert_count = 0;
    int erase_count = 0;

    srand((unsigned int) time(NULL));

    for (int idx=0; idx<OPERATIONS; idx++)
    {
        // Distribute insert operations at 75% for growing
        const int rnd = rand();
        const operation_type_t operation_type = (rnd < (RAND_MAX/ 100 * 75))
            ? operation_type_insert
            : operation_type_erase;

        if (operation_type == operation_type_insert)
        {
            insert_count++;
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
                ret = NOT_ENOUGH_MEMORY;
                break;
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
                print_operations(history, idx+1);
                break;
            }
            *((int*)ptr) = value;

            /*
            printf("*** inserted %d\n", value);
            buffer_set_print_debug(buffer_set, stdout, int_printer);
            printf("\n");
            */
        }
        else
        {
            if (golden_set->size == 0)
                continue;
            if (golden_set->size > RAND_MAX)
                abort(); // FIXME

            erase_count++;

            size_t value_idx = rand();
            value_idx %= golden_set->size;
            const int value = golden_set->data[value_idx];
            golden_set->size--;
            const size_t move_size = (golden_set->size - value_idx) * sizeof(int);
            if (move_size > 0)
                memmove(&golden_set->data[value_idx], &golden_set->data[value_idx+1], move_size);

            const struct operation_s operation = { operation_type_erase, value };
            history[idx] = operation;

            const void * erased_ptr = buffer_set_erase(buffer_set, &value);
            if (!erased_ptr)
            {
                char buffer[128];
                sprintf(buffer, "failed to erase value %d", value);
                ret = strdup(buffer);
                print_operations(history, idx+1);
                break;
            }

            const int erased_value = *((const int*)erased_ptr);
            if (erased_value != value)
            {
                char buffer[128];
                sprintf(buffer, "erased value unexpectedly %d instead of %d", erased_value, value);
                ret = strdup(buffer);
                print_operations(history, idx+1);
                break;
            }

            /*
            printf("*** erased %d\n", value);
            buffer_set_print_debug(buffer_set, stdout, int_printer);
            printf("\n");
            */
        }
    }

    if (!ret)
    {
        struct validation_context_s validation_context = { golden_set, history, NULL };
        buffer_set_for_each(buffer_set, validation_callback, &validation_context);
        ret = validation_context.error_message;
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
                    print_operations(history, OPERATIONS);
                    break;
                }
            }
        }
    }

    if (!ret)
        printf(": %d values inserted, %d erased\n", insert_count, erase_count);

    buffer_set_destroy(buffer_set);
    golden_set_destroy(golden_set);
    free(history);

    return ret;
}
