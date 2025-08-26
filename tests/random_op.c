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

// 8 levels should be enough
#define MAX_ELEMENTS 300

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

static int golden_set_get(struct golden_set_s * golden_set)
{
    size_t idx = rand();
    idx %= golden_set->size;
    const int value = golden_set->data[idx];
    golden_set->size--;
    const size_t move_size = (golden_set->size - idx) * sizeof(int);
    if (move_size > 0)
        memmove(&golden_set->data[idx], &golden_set->data[idx+1], move_size);
    return value;
}

static void golden_set_destroy(struct golden_set_s * golden_set)
{
    free(golden_set->data);
    free(golden_set);
}

struct history_s
{
    struct history_s * next;
    size_t count;
    struct operation_s operations[100];
};

static struct history_s * history_create()
{
    struct history_s * history = malloc(sizeof(struct history_s));
    if (history)
    {
        history->next = NULL;
        history->count = 0;
    }
    return history;
}

static void history_destroy(struct history_s * history)
{
    while (history != NULL)
    {
        struct history_s * next = history->next;
        free(history);
        history = next;
    }
}

static void history_append(
    struct history_s ** phistory,
    operation_type_t operation_type,
    int value
) {
    struct history_s * history = *phistory;
    const size_t max_count = (sizeof(history->operations) / sizeof(history->operations[0]));
    if (history->count == max_count)
    {
        struct history_s * new_history = malloc(sizeof(struct history_s));
        history->next = new_history;
        history = new_history;
        history->next = NULL;
        *phistory = history;
    }
    const struct operation_s operation = { operation_type, value };
    history->operations[history->count] = operation;
    history->count++;
}

static void print_operations(const struct history_s * history)
{
    printf("\n");
    printf("static const struct operation_s operations[] =\n");
    printf("{\n");
    for (; history!=NULL; history = history->next)
    {
        for (size_t idx=0; idx<history->count; idx++)
        {
            const struct operation_s * operation = &history->operations[idx];
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
    }
    printf("}\n");
}

int random_op()
{
    static const char * not_enough_memory = "not enough memory";

    struct history_s * history = history_create();
    if (!history)
    {
        printf("%s", not_enough_memory);
        return -1;
    }

    struct golden_set_s * golden_set = golden_set_create(MAX_ELEMENTS * 3);
    if (!golden_set)
    {
        history_destroy(history);
        printf("%s\n", not_enough_memory);
        return -1;
    }

    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), (uint16_t) MAX_ELEMENTS/2, int_cmp);
    int ret = 0;
    int count = 0;

    struct history_s * history_head = history;
    srand((unsigned int) time(NULL));
    int max_elements_reached = 0;

    for (;;)
    {
        const int rnd = rand();
        const operation_type_t operation_type = (rnd < (RAND_MAX / 100 * 70))
            ? (max_elements_reached ? operation_type_erase : operation_type_insert)
            : (max_elements_reached ? operation_type_insert : operation_type_erase);

        if (operation_type == operation_type_insert)
        {
            count++;
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

            history_append(&history, operation_type_insert, value);

            int inserted;
            void * ptr = buffer_set_insert(buffer_set, &value, &inserted);
            if (!inserted)
            {
                printf("unexpected duplicate %d", value);
                print_operations(history_head);
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
                print_operations(history_head);
                ret = -1;
                break;
            }

            if (!max_elements_reached)
                max_elements_reached = (buffer_set_size == MAX_ELEMENTS);

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

            const int value = golden_set_get(golden_set);
            history_append(&history, operation_type_erase, value);

            const void * erased_ptr = buffer_set_erase(buffer_set, &value);
            if (!erased_ptr)
            {
                printf("failed to erase value %d", value);
                print_operations(history);
                ret = -1;
                break;
            }

            const int erased_value = *((const int*)erased_ptr);
            if (erased_value != value)
            {
                printf("erased value unexpectedly %d instead of %d", erased_value, value);
                print_operations(history);
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
                print_operations(history);
                ret = -1;
                break;
            }

            if (max_elements_reached)
            {
                if (buffer_set_size == 0)
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
        printf("%d values inserted and erased", count);

    buffer_set_destroy(buffer_set);
    golden_set_destroy(golden_set);
    history_destroy(history_head);

    return ret;
}
