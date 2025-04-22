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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define NULL_IDX ((uint16_t)0xFFFF)
#define MAX_CAPACITY ((uint16_t)0xFFFE)

struct node_s
{
    uint16_t left;
    uint16_t right;
    int8_t balance;
};

struct free_node_s
{
    uint16_t next;
};

struct buffer_set_s
{
    size_t value_size;
    size_t node_size;
    int (*compar)(const void * v1, const void * v2);
    uint16_t capacity;
    uint16_t size;
    uint16_t root;
    void * buffer;
    uint16_t free_list;
};

static inline size_t _round(size_t v)
{
    const size_t c = (sizeof(long) - 1);
    v += c;
    return (v - (v & c));
}

static inline struct node_s * _get_node(
    const struct buffer_set_s * buffer_set,
    uint16_t idx
) {
    char * ptr = (char*) buffer_set->buffer;
    ptr += (buffer_set->node_size * idx);
    return (struct node_s*) ptr;
}

static inline void * _get_node_value(struct node_s * node)
{
    return ((char*)node) + _round(sizeof(struct node_s));
}

static uint16_t _make_free_list(void * buffer, size_t node_size, uint16_t first_idx, uint16_t count)
{
    struct free_node_s * free_node = (struct free_node_s*) (((char*)buffer) + (node_size * first_idx));
    uint16_t idx = first_idx;
    for (;;)
    {
        if (--count == 0)
        {
            free_node->next = NULL_IDX;
            return first_idx;
        }
        else
        {
            idx++;
            free_node->next = idx;
            free_node = (struct free_node_s*) (((char*)free_node) + node_size);
        }
    }
}

buffer_set_t * buffer_set_create(
    size_t value_size,
    uint16_t capacity,
    int (*compar)(const void * v1, const void * v2)
) {
    if (capacity > MAX_CAPACITY)
        return NULL;

    struct buffer_set_s * buffer_set = malloc(sizeof(struct buffer_set_s));
    if (buffer_set == NULL)
        return NULL;

    const size_t node_size = _round(sizeof(struct node_s)) + _round(value_size);

    buffer_set->value_size = value_size;
    buffer_set->node_size = node_size;
    buffer_set->compar = compar;
    buffer_set->capacity = capacity;
    buffer_set->size = 0;
    buffer_set->root = NULL_IDX;

    if (capacity > 0)
    {
        const size_t buffer_size = (node_size * capacity);
        void * buffer = malloc(buffer_size);
        if (!buffer)
        {
            free(buffer_set);
            return NULL;
        }
        buffer_set->buffer = buffer;
        buffer_set->free_list = _make_free_list(buffer, node_size, 0, capacity);
    }
    else
    {
        buffer_set->buffer = NULL;
        buffer_set->free_list = NULL_IDX;
    }

    return buffer_set;
}

static uint16_t _rotate_right(struct buffer_set_s * buffer_set, uint16_t a_idx, struct node_s * a)
{
    /*       a             b
     *      / \           / \
     *     b   c?   =>   d?  a
     *    / \               / \
     *   d?  e?            e?  c?
     */
    assert(_get_node(buffer_set, a_idx) == a);
    const uint16_t b_idx = a->left;
    struct node_s * b = _get_node(buffer_set, b_idx);
    a->left = b->right;
    b->right = a_idx;
    return b_idx;
}

static uint16_t _rotate_left(struct buffer_set_s * buffer_set, uint16_t a_idx, struct node_s * a)
{
    /*    a               b
     *   / \             / \
     *  c?  b     =>    a   d?
     *     / \         / \
     *    e?  d?      c?  e?
     */
    assert(_get_node(buffer_set, a_idx) == a);
    const uint16_t b_idx = a->right;
    struct node_s * b = _get_node(buffer_set, b_idx);
    a->right = b->left;
    b->left = a_idx;
    return b_idx;
}

struct rebalance_result_s
{
    uint16_t idx;
    uint16_t height_changed;
};

static inline struct rebalance_result_s _make_rebalance_result(uint16_t idx, uint16_t height_changed)
{
    struct rebalance_result_s rebalance_result = { idx, height_changed };
    return rebalance_result;
}

static struct rebalance_result_s _balance_left(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    struct node_s * node
) {
    struct node_s * left = _get_node(buffer_set, node->left);
    if (left->balance == 1)
    {
        node->left = _rotate_left(buffer_set, node->left, left);
        const uint16_t nr_idx = _rotate_right(buffer_set, idx, node);
        struct node_s * nr = _get_node(buffer_set, nr_idx);
        assert((nr->balance >= -1) && (nr->balance <= 1));
        /*
        if (nr->balance == -1)
        {
            left->balance = 0;
            node->balance = 1;
            nr->balance = 0;
        }
        else if (nr->balance == 0)
        {
            left->balance = 0;
            node->balance = 0;
        }
        else
        {
            left->balance = -1;
            node->balance = 0;
            nr->balance = 0;
        }
        */
        left->balance = (nr->balance == 1) ? -1 : 0;
        node->balance = (nr->balance == -1) ? 1 : 0;
        nr->balance = 0;
        return _make_rebalance_result(nr_idx, 0);
    }
    else
    {
        idx = _rotate_right(buffer_set, idx, node);
        assert(_get_node(buffer_set, idx) == left);
        if (left->balance == 0)
        {
            // can happen only on erase
            // height of the subtree does not change
            left->balance = 1;
            node->balance = -1;
            return _make_rebalance_result(idx, 1);
        }
        else
        {
            left->balance = 0;
            node->balance = 0;
            return _make_rebalance_result(idx, 0);
        }
    }
}

static struct rebalance_result_s _balance_right(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    struct node_s * node
) {
    struct node_s * right = _get_node(buffer_set, node->right);
    if (right->balance == -1)
    {
        node->right = _rotate_right(buffer_set, node->right, right);
        const uint16_t nr_idx = _rotate_left(buffer_set, idx, node);
        struct node_s * nr = _get_node(buffer_set, nr_idx);
        assert((nr->balance >= -1) && (nr->balance <= 1));
        /*
        if (nr->balance == 1)
        {
            right->balance = 0;
            node->balance = -1;
            nr->balance = 0;
        }
        else if (nr->balance == 0)
        {
            right->balance = 0;
            node->balance = 0;
        }
        else
        {
            right->balance = 1;
            node->balance = 0;
            nr->balance = 0;
        }
        */
        right->balance = (nr->balance == -1) ? 1 : 0;
        node->balance = (nr->balance == 1) ? -1 : 0;
        nr->balance = 0;
        return _make_rebalance_result(nr_idx, 0);
    }
    else
    {
        idx = _rotate_left(buffer_set, idx, node);
        assert(_get_node(buffer_set, idx) == right);
        if (right->balance == 0)
        {
            // can happen only on erase,
            // height of the subtree does not change
            right->balance = -1;
            node->balance = 1;
            return _make_rebalance_result(idx, 1);
        }
        else
        {
            right->balance = 0;
            node->balance = 0;
            return _make_rebalance_result(idx, 0);
        }
    }
}

struct insert_result_s
{
    uint16_t idx;
    uint16_t height_changed;
};

struct value_node_s
{
    uint16_t idx;
    uint16_t inserted;
};

static struct insert_result_s _make_insert_result(uint16_t idx, uint16_t height_changed)
{
    struct insert_result_s insert_result = { idx, height_changed };
    return insert_result;
}

static inline uint16_t _round_up_power_of_2(uint16_t value)
{
    value--;
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value++;
    return value;
}

static uint16_t _calculate_new_capacity(uint16_t capacity)
{
    if (capacity < 16)
        return 16;

    if (capacity < 1024)
    {
        // double the capacity while it is less than 1024
        uint16_t new_capacity = _round_up_power_of_2(capacity);
        if (new_capacity != capacity)
            return new_capacity;
        new_capacity *= 2;
        return new_capacity;
    }

    const uint16_t remainder = (capacity % 1024);
    uint16_t new_capacity = (capacity - remainder);
    if ((MAX_CAPACITY - new_capacity) < 1024)
        return MAX_CAPACITY;
    new_capacity += 1024;
    return new_capacity;
}

static struct insert_result_s _insert(
    struct buffer_set_s * buffer_set,
    const void * value,
    uint16_t idx,
    struct value_node_s * value_node
) {
    if (idx == NULL_IDX)
    {
        idx = buffer_set->free_list;
        if (idx == NULL_IDX)
        {
            assert(buffer_set->size == buffer_set->capacity);
            if (buffer_set->capacity == MAX_CAPACITY)
            {
                value_node->idx = NULL_IDX;
                return _make_insert_result(idx, 0);
            }

            const uint16_t new_capacity = _calculate_new_capacity(buffer_set->capacity);
            assert(new_capacity > buffer_set->capacity);
            void * buffer = malloc(new_capacity * buffer_set->node_size);
            if (!buffer)
            {
                value_node->idx = NULL_IDX;
                return _make_insert_result(idx, 0);
            }

            memcpy(buffer, buffer_set->buffer, buffer_set->size * buffer_set->node_size);
            free(buffer_set->buffer);

            buffer_set->capacity = new_capacity;
            buffer_set->buffer = buffer;

            buffer_set->free_list = _make_free_list(
                buffer,
                buffer_set->node_size,
                buffer_set->size,
                (new_capacity - buffer_set->size)
            );

            idx = buffer_set->free_list;
        }

        const size_t offs = (buffer_set->node_size * idx);
        struct free_node_s * free_node = (struct free_node_s*) (((char*)buffer_set->buffer) + offs);
        buffer_set->free_list = free_node->next;

        struct node_s * node = (struct node_s*) (((char*)buffer_set->buffer) + offs);
        node->left = NULL_IDX;
        node->right = NULL_IDX;
        node->balance = 0;

        buffer_set->size++;
        value_node->idx = idx;
        value_node->inserted = 1;

        return _make_insert_result(idx, 1);
    }
    else
    {
        struct node_s * node = _get_node(buffer_set, idx);
        int cmp = buffer_set->compar(value, _get_node_value(node));
        if (cmp < 0)
        {
            const struct insert_result_s insert_result = _insert(buffer_set, value, node->left, value_node);
            node = _get_node(buffer_set, idx);
            node->left = insert_result.idx;
            uint16_t height_changed = 0;
            if (insert_result.height_changed)
            {
                const int8_t balance = --node->balance;
                if (balance == -1)
                    height_changed = 1;
                else if (balance == -2)
                {
                    const struct rebalance_result_s rebalance_result = _balance_left(buffer_set, idx, node);
                    idx = rebalance_result.idx;
                    assert(rebalance_result.height_changed == 0);
                    assert(_get_node(buffer_set, idx)->balance == 0);
                }
            }
            return _make_insert_result(idx, height_changed);
        }
        else if (cmp > 0)
        {
            const struct insert_result_s insert_result = _insert(buffer_set, value, node->right, value_node);
            node = _get_node(buffer_set, idx);
            node->right = insert_result.idx;
            uint16_t height_changed = 0;
            if (insert_result.height_changed)
            {
                const int8_t balance = ++node->balance;
                if (balance == 1)
                    height_changed = 1;
                else if (balance == 2)
                {
                    const struct rebalance_result_s rebalance_result = _balance_right(buffer_set, idx, node);
                    idx = rebalance_result.idx;
                    assert(rebalance_result.height_changed == 0);
                    assert(_get_node(buffer_set, idx)->balance == 0);
                }
            }
            return _make_insert_result(idx, height_changed);
        }
        else
        {
            value_node->idx = idx;
            value_node->inserted = 0;
            return _make_insert_result(idx, 0);
        }
    }
}

void * buffer_set_insert(
    buffer_set_t * buffer_set,
    const void * value,
    int * inserted
) {
    struct value_node_s value_node;
    const struct insert_result_s insert_result = _insert(
        buffer_set,
        value,
        buffer_set->root,
        &value_node
    );

    if (value_node.idx == NULL_IDX)
        return NULL;

    buffer_set->root = insert_result.idx;
    *inserted = value_node.inserted;

    struct node_s * node = _get_node(buffer_set, value_node.idx);
    return _get_node_value(node);
}

uint16_t buffer_set_get_size(buffer_set_t * buffer_set)
{
    return buffer_set->size;
}

void * buffer_set_get(
    buffer_set_t * buffer_set,
    const void * value
) {
    uint16_t idx = buffer_set->root;
    for (;;)
    {
        if (idx == NULL_IDX)
            return NULL;
        struct node_s * node = _get_node(buffer_set, idx);
        const int cmp = buffer_set->compar(value, _get_node_value(node));
        if (cmp < 0)
            idx = node->left;
        else if (cmp > 0)
            idx = node->right;
        else
            return _get_node_value(node);
    }
}

struct erase_result_s
{
    uint16_t idx;
    uint16_t height_changed;
};

static inline struct erase_result_s _make_erase_result(uint16_t idx, uint16_t height_changed)
{
    struct erase_result_s erase_result = { idx, height_changed };
    return erase_result;
}

static struct erase_result_s _erase(
    struct buffer_set_s * buffer_set,
    const void * value,
    uint16_t idx,
    uint16_t * erased_idx
) {
    if (idx == NULL_IDX)
    {
        *erased_idx = NULL_IDX;
        return _make_erase_result(NULL_IDX, 0);
    }

    struct node_s * node = _get_node(buffer_set, idx);
    const int cmp = buffer_set->compar(value, _get_node_value(node));
    if (cmp < 0)
    {
        const struct erase_result_s erase_result = _erase(buffer_set, value, node->left, erased_idx);
        if (*erased_idx == NULL_IDX)
            return erase_result;

        node = _get_node(buffer_set, idx);
        node->left = erase_result.idx;
        if (erase_result.height_changed)
        {
            const int8_t balance = ++node->balance;
            if (balance == 0)
                return _make_erase_result(idx, 1);
            else if (balance == 1)
                return _make_erase_result(idx, 0);
            else
            {
                assert(balance == 2);
                const struct rebalance_result_s rebalance_result = _balance_right(buffer_set, idx, node);
                return _make_erase_result(rebalance_result.idx, !rebalance_result.height_changed);
            }
        }
        else
            return _make_erase_result(idx, 0);
    }
    else if (cmp > 0)
    {
        const struct erase_result_s erase_result = _erase(buffer_set, value, node->right, erased_idx);
        if (*erased_idx == NULL_IDX)
            return erase_result;

        node = _get_node(buffer_set, idx);
        node->right = erase_result.idx;
        if (erase_result.height_changed)
        {
            const int8_t balance = --node->balance;
            if (balance == 0)
                return _make_erase_result(idx, 1);
            else if (balance == -1)
                return _make_erase_result(idx, 0);
            else
            {
                assert(balance == -2);
                const struct rebalance_result_s rebalance_result = _balance_left(buffer_set, idx, node);
                return _make_erase_result(rebalance_result.idx, !rebalance_result.height_changed);
            }
        }
        else
            return _make_erase_result(idx, 0);
    }
    else
    {
        if ((node->left == NULL_IDX) && (node->right == NULL_IDX))
        {
            *erased_idx = idx;
            return _make_erase_result(NULL_IDX, 1);
        }
        else if (node->left == NULL_IDX)
        {
            assert(_get_node(buffer_set, node->right)->left == NULL_IDX);
            assert(_get_node(buffer_set, node->right)->right == NULL_IDX);
            *erased_idx = idx;
            return _make_erase_result(node->right, 1);
        }
        else if (node->right == NULL_IDX)
        {
            assert(_get_node(buffer_set, node->left)->left == NULL_IDX);
            assert(_get_node(buffer_set, node->left)->right == NULL_IDX);
            *erased_idx = idx;
            return _make_erase_result(node->left, 1);
        }
        else
        {
            uint16_t tmp_idx = node->right;
            struct node_s * tmp = _get_node(buffer_set, tmp_idx);
            for (;;)
            {
                if (tmp->left == NULL_IDX)
                    break;
                tmp_idx = tmp->left;
                tmp = _get_node(buffer_set, tmp_idx);
            }

            const struct erase_result_s erase_result = _erase(buffer_set, _get_node_value(tmp), node->right, erased_idx);
            assert(*erased_idx == tmp_idx);
            *erased_idx = idx;

            tmp->left = node->left;
            tmp->right = erase_result.idx;
            tmp->balance = node->balance;

            if (erase_result.height_changed)
            {
                const int8_t balance = --tmp->balance;
                if (balance == 0)
                    return _make_erase_result(tmp_idx, 1);
                else if (balance == -1)
                    return _make_erase_result(tmp_idx, 0);
                else
                {
                    assert(balance == -2);
                    const struct rebalance_result_s rebalance_result = _balance_left(buffer_set, tmp_idx, tmp);
                    return _make_erase_result(rebalance_result.idx, !rebalance_result.height_changed);
                }
            }
            else
                return _make_erase_result(tmp_idx, 0);
        }
    }
}

const void * buffer_set_erase(
    buffer_set_t * buffer_set,
    const void * value
) {
    uint16_t erased_idx;
    const struct erase_result_s erase_result = _erase(
        buffer_set,
        value,
        buffer_set->root,
        &erased_idx
    );

    if (erased_idx == NULL_IDX)
        return NULL; // not found

    // FIXME: It may be worth reducing the buffer size
    // if there is a significant amount of free space.
    assert(buffer_set->size > 0);
    buffer_set->size--;
    buffer_set->root = erase_result.idx;

    struct node_s * node = _get_node(buffer_set, erased_idx);
    struct free_node_s * free_node = (struct free_node_s*) node;
    free_node->next = buffer_set->free_list;
    buffer_set->free_list = erased_idx;

    return _get_node_value(node);
}

struct walk_context_s
{
    const struct buffer_set_s * buffer_set;
    int (*func)(const void * value, void * arg);
    void * arg;
};

static int _walk(
    const struct walk_context_s * context,
    uint16_t idx
) {
    const struct buffer_set_s * buffer_set = context->buffer_set;
    struct node_s * node = _get_node(buffer_set, idx);
    int rc;

    if (node->left != NULL_IDX)
    {
        rc = _walk(context, node->left);
        if (rc)
            return rc;
    }

    rc = context->func(_get_node_value(node), context->arg);
    if (rc)
        return rc;

    if (node->right != NULL_IDX)
        rc = _walk(context, node->right);

    return rc;
}

int buffer_set_walk(
    const buffer_set_t * buffer_set,
    int (*func)(const void * value, void * arg),
    void * arg
) {
    const uint16_t root = buffer_set->root;
    if (root == NULL_IDX)
        return 0;

    const struct walk_context_s context = { buffer_set, func, arg };
    return _walk(&context, root);
}

struct print_debug_context_s
{
    const struct buffer_set_s * buffer_set;
    FILE * file;
    void (*value_printer)(FILE * file, const void * value);
};

static void _print_debug(
    const struct print_debug_context_s * context,
    uint16_t idx
) {
    const struct buffer_set_s * buffer_set = context->buffer_set;
    FILE * file = context->file;
    struct node_s * node = _get_node(buffer_set, idx);
    fprintf(file, "    %hu[", idx);
    context->value_printer(file, _get_node_value(node));

    fprintf(file, "]: left=");
    if (node->left == NULL_IDX)
        fprintf(file, "null");
    else
        fprintf(file, "%hu", node->left);

    fprintf(file, " right=");
    if (node->right == NULL_IDX)
        fprintf(file, "null");
    else
        fprintf(file, "%hu", node->right);

    fprintf(file, " balance=%hd\n", node->balance);

    if (node->left != NULL_IDX)
        _print_debug(context, node->left);

    if (node->right != NULL_IDX)
        _print_debug(context, node->right);
}

void buffer_set_print_debug(
    const buffer_set_t * buffer_set,
    FILE * file,
    void (*value_printer)(FILE * file, const void * value)
) {
    fprintf(file, "{");
    const uint16_t root = buffer_set->root;
    if (root != NULL_IDX)
    {
        fprintf(file, "\n");
        const struct print_debug_context_s context = { buffer_set, file, value_printer };
        _print_debug(&context, root);
    }
    fprintf(file, "}");
}

void buffer_set_destroy(buffer_set_t * buffer_set)
{
    free(buffer_set->buffer);
    free(buffer_set);
}
