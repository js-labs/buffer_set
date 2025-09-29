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

// ~11 levels should be enough
#define MAX_ELEMENTS 2100

struct golden_set_s
{
    size_t capacity;
    size_t size;
    int * data;
};

static int bsearch_int_cmp(const void * pv1, const void * pv2)
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

static int golden_set_init(struct golden_set_s * golden_set, size_t capacity)
{
    golden_set->capacity = capacity;
    golden_set->size = 0;
    golden_set->data = malloc(sizeof(int*) * capacity);
    if (golden_set->data)
        return 0;
    else
        return -1;
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

static void golden_set_deinit(struct golden_set_s * golden_set)
{
    free(golden_set->data);
}

struct history_block_s
{
    struct history_block_s * next;
    struct operation_s operations[100];
};

struct history_s
{
    size_t count;
    struct history_block_s * head;
    struct history_block_s * tail;
};

static int history_init(struct history_s * history)
{
    history->head = malloc(sizeof(struct history_block_s));
    if (history->head)
    {
        history->head->next = NULL;
        history->count = 0;
        history->tail = history->head;
        return 0;
    }
    return -1;
}

static void history_deinit(struct history_s * history)
{
    struct history_block_s * history_block = history->head;
    while (history_block != NULL)
    {
        struct history_block_s * next = history_block->next;
        free(history_block);
        history_block = next;
    }
}

static void history_append(
    struct history_s * history,
    operation_type_t operation_type,
    int value
) {
    struct history_block_s * history_block = history->tail;
    const size_t max_count = (sizeof(history_block->operations) / sizeof(history_block->operations[0]));
    if (history->count == max_count)
    {
        struct history_block_s * new_history_block = malloc(sizeof(struct history_block_s));
        new_history_block->next = NULL;
        history->count = 0;
        history->tail = new_history_block;
        history_block->next = new_history_block;
        history_block = new_history_block;
    }
    const struct operation_s operation = { operation_type, value };
    history_block->operations[history->count] = operation;
    history->count++;
}

static void history_print(const struct history_s * history)
{
    printf("\n");
    printf("static const struct operation_s operations[] =\n");
    printf("{\n");
    struct history_block_s * history_block = history->head;
    for (; history_block!=NULL; history_block = history_block->next)
    {
        const size_t count = (history_block == history->tail)
            ? history->count
            : (sizeof(history_block->operations) / sizeof(history_block->operations[0]));

        for (size_t idx=0; idx<count; idx++)
        {
            const struct operation_s * operation = &history_block->operations[idx];
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
            printf("    { %s, %d },\n", operation_type, operation->value);
        }
    }
    printf("};\n");
}

int random_op()
{
    static const char * not_enough_memory = "not enough memory";

    struct history_s history;
    if (history_init(&history))
    {
        printf("%s", not_enough_memory);
        return -1;
    }

    struct golden_set_s golden_set;
    if (golden_set_init(&golden_set, MAX_ELEMENTS * 3))
    {
        history_deinit(&history);
        printf("%s\n", not_enough_memory);
        return -1;
    }

    buffer_set_t * buffer_set = buffer_set_create(sizeof(int), (uint16_t) MAX_ELEMENTS/4, &int_cmp, NULL);
    int ret = 0;

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
            int value = rand();
            for (;;)
            {
                const void * ptr = bsearch(&value, golden_set.data, golden_set.size, sizeof(int), &bsearch_int_cmp);
                if (!ptr)
                    break;
                value = rand();
            }

            if (golden_set_add(&golden_set, value) != 0)
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
                history_print(&history);
                ret = -1;
                break;
            }
            *((int*)ptr) = value;

            if (!max_elements_reached)
            {
                const uint16_t buffer_set_size = buffer_set_get_size(buffer_set);
                max_elements_reached = (buffer_set_size == MAX_ELEMENTS);
            }

            /*
            printf("*** inserted %d\n", value);
            buffer_set_print_debug(buffer_set, stdout, int_printer);
            printf("\n");
            */
        }
        else
        {
            if (golden_set.size == 0)
                continue;

            const int value = golden_set_get(&golden_set);
            history_append(&history, operation_type_erase, value);

            const void * erased_ptr = buffer_set_erase(buffer_set, &value);
            if (!erased_ptr)
            {
                printf("failed to erase value %d", value);
                history_print(&history);
                ret = -1;
                break;
            }

            const int erased_value = *((const int*)erased_ptr);
            if (erased_value != value)
            {
                printf("erased value unexpectedly %d instead of %d", erased_value, value);
                history_print(&history);
                ret = -1;
                break;
            }

            if (max_elements_reached)
            {
                const uint16_t buffer_set_size = buffer_set_get_size(buffer_set);
                if (buffer_set_size == 0)
                    break;
            }

            /*
            printf("*** erased %d\n", value);
            buffer_set_print_debug(buffer_set, stdout, int_printer);
            printf("\n");
            */
        }

        for (size_t idx=0; idx<golden_set.size; idx++)
        {
            const void * value = buffer_set_get(buffer_set, &golden_set.data[idx]);
            if (!value)
            {
                printf("value %d not found", golden_set.data[idx]);
                history_print(&history);
                ret = -1;
                break;
            }

            if (*((const int*)value) != golden_set.data[idx])
            {
                printf("got %d instead of %d", *((const int*)value), golden_set.data[idx]);
                history_print(&history);
                ret = -1;
                break;
            }
        }
        if (ret != 0)
            break;

        const uint16_t buffer_set_size = buffer_set_get_size(buffer_set);
        if (golden_set.size != buffer_set_size)
        {
            printf(
                "buffer set size %hu not equal to the golden set size %zu",
                buffer_set_size,
                golden_set.size
            );
            history_print(&history);
            ret = -1;
            break;
        }
    }

    if (ret == 0)
        printf("%d values inserted and erased", MAX_ELEMENTS);

    buffer_set_destroy(buffer_set);
    golden_set_deinit(&golden_set);
    history_deinit(&history);

    return ret;
}
