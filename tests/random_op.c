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
#include <time.h>
#include <search.h>
#include <string.h>
#include "test.h"

#define OPERATIONS 1000

struct golden_set_s
{
    size_t capacity;
    size_t size;
    int * data;
};

struct golden_set_s * golden_set_create(size_t capacity)
{
    struct golden_set_s * golden_set = malloc(sizeof(struct golden_set_s));
    if (!golden_set)
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

struct validation_context_s
{
    const struct golden_set_s * golden_set;
    const struct operation_s * history;
    int result;
};

static int validation_callback(const void * value, void * arg)
{
    struct validation_context_s * validation_context = (struct validation_context_s*) arg;
    struct golden_set_s * golden_set = (struct golden_set_s*) validation_context->golden_set;
    const void * ptr = bsearch(value, golden_set->data, golden_set->size, sizeof(int), int_cmp);
    if (ptr)
        return 0;
    else
    {
        printf("value %d not found in the golden set", *((const int*)value));
        print_operations(validation_context->history, OPERATIONS);
        validation_context->result = -1;
        return -1;
    }
}

int random_op()
{
    static const char * not_enough_memory = "not enough memory";

    size_t capacity = (OPERATIONS / 2);
    if (capacity < 16)
        capacity = 16;

    struct operation_s * history = malloc(sizeof(struct operation_s) * OPERATIONS);
    if (!history)
    {
        printf("%s", not_enough_memory);
        return -1;
    }

    struct golden_set_s * golden_set = golden_set_create(capacity);
    if (!golden_set)
    {
        free(history);
        printf("%s", not_enough_memory);
        return -1;
    }

    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), (uint16_t) capacity, int_cmp);
    int ret = 0;
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
                const void * ptr = bsearch(&value, golden_set->data, golden_set->size, sizeof(int), int_cmp);
                if (!ptr)
                    break;
                value = rand();
            }

            if (golden_set_add(golden_set, value) != 0)
            {
                printf("%s", not_enough_memory);
                ret = -1;
                break;
            }

            const struct operation_s operation = { operation_type_insert, value };
            history[idx] = operation;

            int inserted;
            void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
            if (!inserted)
            {
                printf("unexpected duplicate %d", value);
                print_operations(history, idx+1);
                ret = -1;
                break;
            }
            *((int*)ptr) = value;

            const uint16_t buffer_set_size = buffer_set_get_size(buffer_set);
            if (golden_set->size != buffer_set_size)
            {
                printf(
                    "buffer set size %hu not equal to the golden set size %zu",
                    buffer_set_size,
                    golden_set->size
                );
                print_operations(history, idx+1);
                ret = -1;
                break;
            }

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
                printf("failed to erase value %d", value);
                print_operations(history, idx+1);
                ret = -1;
                break;
            }

            const int erased_value = *((const int*)erased_ptr);
            if (erased_value != value)
            {
                printf("erased value unexpectedly %d instead of %d", erased_value, value);
                print_operations(history, idx+1);
                ret = -1;
                break;
            }

            const uint16_t buffer_set_size = buffer_set_get_size(buffer_set);
            if (golden_set->size != buffer_set_size)
            {
                printf(
                    "buffer set size %hu not equal to the golden set size %zu",
                    buffer_set_size,
                    golden_set->size
                );
                print_operations(history, idx+1);
                ret = -1;
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
        struct validation_context_s validation_context = { golden_set, history, 0 };
        buffer_set_walk(buffer_set, validation_callback, &validation_context);
        ret = validation_context.result;
        if (!ret)
        {
            for (size_t idx=0; idx<golden_set->size; idx++)
            {
                const int value = golden_set->data[idx];
                const void * ptr = buffer_set_get(buffer_set, &value);
                if (!ptr)
                {
                    printf("value %d not found in the buffer set", value);
                    print_operations(history, OPERATIONS);
                    ret = -1;
                    break;
                }
            }
        }
    }

    if (!ret)
        printf("%d values inserted, %d erased", insert_count, erase_count);

    buffer_set_destroy(buffer_set);
    golden_set_destroy(golden_set);
    free(history);

    return ret;
}
