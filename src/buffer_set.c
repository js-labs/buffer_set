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
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Small optimization: because the NULL index is defined as 0, buffer element 0
// is reserved and never used. To simplify access (eliminating the need to subtract 1 from indices),
// we set a pointer to just before the actual buffer start. This way, buffer[1]
// maps directly to the first real element, buffer[2] to the second, etc.,
// streamlining index calculations.

#define NULL_IDX (0)
#define MIN_CAPACITY ((uint16_t)0x0010)
#define MAX_CAPACITY ((uint16_t)0xFFFF)
#define CAPACITY_GROWTH_STEP ((uint16_t)0x400)

struct node_s
{
    uint16_t parent;
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
    size_t node_size;
    int (*compar)(const void * v1, const void * v2, void * thunk);
    void (*move)(void * dst, void * src, void * thunk);
    void * thunk;
    uint16_t capacity;
    uint16_t size;
    uint16_t root;
    void * buffer;
    uint16_t free_list;
};

static inline size_t _round(size_t v)
{
    const size_t c = (sizeof(void*) - 1);
    v += c;
    return (v - (v & c));
}

static inline struct node_s * _get_node(
    struct buffer_set_s * buffer_set,
    uint16_t idx
) {
    char * ptr = (char*) buffer_set->buffer;
    ptr += (buffer_set->node_size * idx);
    return (struct node_s*) ptr;
}

static inline struct free_node_s * _get_free_node(
    struct buffer_set_s * buffer_set,
    uint16_t idx
) {
    char * ptr = (char*) buffer_set->buffer;
    ptr += (buffer_set->node_size * idx);
    return (struct free_node_s*) ptr;
}

static inline void * _node_get_value(struct node_s * node)
{
    return ((char*)node) + _round(sizeof(struct node_s));
}

static uint16_t _make_free_list(
    void * buffer,
    size_t node_size,
    uint16_t first_idx,
    uint16_t count
) {
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
    uint16_t initial_capacity,
    int (*compar)(const void * v1, const void * v2, void * thunk),
    void (*move)(void * dst, void * src, void * thunk),
    void * thunk
) {
    struct buffer_set_s * buffer_set = malloc(sizeof(struct buffer_set_s));
    if (buffer_set == NULL)
    {
        // errno set to ENOMEM by malloc()
        return NULL;
    }

    const size_t node_size = _round(sizeof(struct node_s)) + _round(value_size);

    buffer_set->node_size = node_size;
    buffer_set->compar = compar;
    buffer_set->move = move;
    buffer_set->thunk = thunk;
    buffer_set->capacity = initial_capacity;
    buffer_set->size = 0;
    buffer_set->root = NULL_IDX;

    if (initial_capacity > 0)
    {
        const size_t buffer_size = (node_size * initial_capacity);
        void * buffer = malloc(buffer_size);
        if (!buffer)
        {
            free(buffer_set);
            return NULL;
        }
        buffer_set->buffer = buffer;
        buffer_set->free_list = _make_free_list(buffer, node_size, 1, initial_capacity - 1);
    }
    else
    {
        buffer_set->buffer = NULL;
        buffer_set->free_list = NULL_IDX;
    }

    return buffer_set;
}

uint16_t buffer_set_get_size(buffer_set_t * buffer_set)
{
    return buffer_set->size;
}

uint16_t buffer_set_get_capacity(buffer_set_t * buffer_set)
{
    return buffer_set->capacity;
}

buffer_set_iterator_t * buffer_set_begin(buffer_set_t * buffer_set)
{
    uint16_t idx = buffer_set->root;
    if (idx == NULL_IDX)
        return buffer_set->buffer;
    for (;;)
    {
        struct node_s * node = _get_node(buffer_set, idx);
        if (node->left == NULL_IDX)
            return (buffer_set_iterator_t*) node;
        idx = node->left;
    }
}

buffer_set_iterator_t * buffer_set_end(buffer_set_t * buffer_set)
{
    return buffer_set->buffer;
}

buffer_set_iterator_t * buffer_set_iterator_next(
    buffer_set_t * buffer_set,
    buffer_set_iterator_t * it
) {
    struct node_s * node = (struct node_s*) it;
    if (node->right == NULL_IDX)
    {
        for (;;)
        {
            if (node->parent == NULL_IDX)
                return buffer_set_end(buffer_set);

            const ptrdiff_t offs = (((char*)node) - ((char*)buffer_set->buffer));
            assert((offs % buffer_set->node_size) == 0);
            const uint16_t from_idx = (uint16_t) (offs / buffer_set->node_size);
            node = _get_node(buffer_set, node->parent);
            if (node->left == from_idx)
                return (buffer_set_iterator_t*) node;

            assert(node->right == from_idx);
        }
    }
    else
    {
        node = _get_node(buffer_set, node->right);
        while (node->left != NULL_IDX)
            node = _get_node(buffer_set, node->left);
        return (buffer_set_iterator_t*) node;
    }
}

void * buffer_set_get(
    buffer_set_t * buffer_set,
    const void * value
) {
    buffer_set_iterator_t * it = buffer_set_find(buffer_set, value);
    if (it == buffer_set_end(buffer_set))
        return NULL;
    return buffer_set_get_at(buffer_set, it);
}

void * buffer_set_get_at(
    buffer_set_t * buffer_set,
    buffer_set_iterator_t * it
) {
    struct node_s * node = (struct node_s*) it;
    return _node_get_value(node);
}

buffer_set_iterator_t * buffer_set_find(
    buffer_set_t * buffer_set,
    const void * value
) {
    uint16_t idx = buffer_set->root;
    for (;;)
    {
        if (idx == NULL_IDX)
            return buffer_set_end(buffer_set);
        struct node_s * node = _get_node(buffer_set, idx);
        const int cmp = buffer_set->compar(value, _node_get_value(node), buffer_set->thunk);
        if (cmp == 0)
            return (buffer_set_iterator_t*) node;
        const int side = ((cmp > 0) ? 1 : 0);
        idx = (&node->left)[side];
    }
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
    // The implemented logic doubles the capacity while it less than 1024 elements,
    // then increase capacity by 1024 each time.
    if (capacity < MIN_CAPACITY)
        return MIN_CAPACITY;

    if (capacity <= (CAPACITY_GROWTH_STEP / 2))
    {
        uint16_t new_capacity = _round_up_power_of_2(capacity);
        if (new_capacity != capacity)
            return new_capacity;
        new_capacity *= 2;
        return new_capacity;
    }

    const uint16_t remainder = (capacity % CAPACITY_GROWTH_STEP);
    uint16_t new_capacity = (capacity - remainder);
    if ((MAX_CAPACITY - new_capacity) < CAPACITY_GROWTH_STEP)
        return MAX_CAPACITY;
    new_capacity += CAPACITY_GROWTH_STEP;
    return new_capacity;
}

static inline uint16_t _rotate_right(
    struct buffer_set_s * buffer_set,
    uint16_t a_idx,
    struct node_s * a_node
) {
    /*       a             b
     *      / \           / \
     *     b   c?   =>   d?  a
     *    / \               / \
     *   d?  e?            e?  c?
     */
    assert(_get_node(buffer_set, a_idx) == a_node);
    const uint16_t parent_idx = a_node->parent;
    const uint16_t b_idx = a_node->left;
    struct node_s * b_node = _get_node(buffer_set, b_idx);
    a_node->parent = b_idx;
    a_node->left = b_node->right;
    // a_node->left can be NULL_IDX,
    // but since we have a special dummy node at offset 0,
    // we can safely set the parent there instead of branching
    // even if a_node->left == 0
    _get_node(buffer_set, a_node->left)->parent = a_idx;
    b_node->parent = parent_idx;
    b_node->right = a_idx;
    return b_idx;
}

static inline uint16_t _rotate_left(
    struct buffer_set_s * buffer_set,
    uint16_t a_idx,
    struct node_s * a_node
) {
    /*    a               b
     *   / \             / \
     *  c?  b     =>    a   d?
     *     / \         / \
     *    e?  d?      c?  e?
     */
    assert(_get_node(buffer_set, a_idx) == a_node);
    const uint16_t parent_idx = a_node->parent;
    const uint16_t b_idx = a_node->right;
    struct node_s * b_node = _get_node(buffer_set, b_idx);
    a_node->parent = b_idx;
    a_node->right = b_node->left;
    // a_node->right can be NULL_IDX,
    // but since we have a special dummy node at offset 0,
    // we can safely set the parent there instead of branching
    // even if a_node->right == 0
    _get_node(buffer_set, a_node->right)->parent = a_idx;
    b_node->parent = parent_idx;
    b_node->left = a_idx;
    return b_idx;
}

struct balance_result_s
{
    uint16_t idx;
    uint16_t height_changed;
};

static inline struct balance_result_s _make_balance_result(
    uint16_t idx,
    uint16_t height_changed
) {
    struct balance_result_s balance_result = { idx, height_changed };
    return balance_result;
}

static struct balance_result_s _balance_right(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    struct node_s * node
) {
    assert(_get_node(buffer_set, idx) == node);
    assert(node->balance == 2);
    struct node_s * right_node = _get_node(buffer_set, node->right);
    if (right_node->balance == -1)
    {
        node->right = _rotate_right(buffer_set, node->right, right_node);
        const uint16_t head_idx = _rotate_left(buffer_set, idx, node);
        struct node_s * head_node = _get_node(buffer_set, head_idx);
        assert((head_node->balance >= -1) && (head_node->balance <= 1));
#if defined(USE_REFERENCE_CODE)
        if (head_node->balance == 1)
        {
            right_node->balance = 0;
            node->balance = -1;
            head_node->balance = 0;
        }
        else if (head_node->balance == 0)
        {
            right_node->balance = 0;
            node->balance = 0;
        }
        else
        {
            right_node->balance = 1;
            node->balance = 0;
            head_node->balance = 0;
        }
#else
        right_node->balance = (head_node->balance == -1) ? 1 : 0;
        node->balance = (head_node->balance == 1) ? -1 : 0;
        head_node->balance = 0;
#endif
        return _make_balance_result(head_idx, 0);
    }
    else
    {
        const uint16_t head_idx = _rotate_left(buffer_set, idx, node);
        assert(_get_node(buffer_set, head_idx) == right_node);
        if (right_node->balance == 0)
        {
            // can happen only on erase
            right_node->balance = -1;
            node->balance = 1;
            return _make_balance_result(head_idx, 1);
        }
        else
        {
            right_node->balance = 0;
            node->balance = 0;
            return _make_balance_result(head_idx, 0);
        }
    }
}

static struct balance_result_s _balance_left(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    struct node_s * node
) {
    assert(_get_node(buffer_set, idx) == node);
    assert(node->balance == -2);

    struct node_s * left_node = _get_node(buffer_set, node->left);
    if (left_node->balance == 1)
    {
        node->left = _rotate_left(buffer_set, node->left, left_node);
        const uint16_t head_idx = _rotate_right(buffer_set, idx, node);
        struct node_s * head_node = _get_node(buffer_set, head_idx);
        assert((head_node->balance >= -1) && (head_node->balance <= 1));
#if defined(USE_REFERENCE_CODE)
        if (head_node->balance == -1)
        {
            left_node->balance = 0;
            node->balance = 1;
            head_node->balance = 0;
        }
        else if (head_node->balance == 0)
        {
            left_node->balance = 0;
            node->balance = 0;
        }
        else
        {
            left->balance = -1;
            node->balance = 0;
            nr->balance = 0;
        }
#else
        left_node->balance = (head_node->balance == 1) ? -1 : 0;
        node->balance = (head_node->balance == -1) ? 1 : 0;
        head_node->balance = 0;
#endif
        return _make_balance_result(head_idx, 0);
    }
    else
    {
        const uint16_t head_idx = _rotate_right(buffer_set, idx, node);
        assert(_get_node(buffer_set, head_idx) == left_node);
        if (left_node->balance == 0)
        {
            // can happen only on erase
            left_node->balance = 1;
            node->balance = -1;
            return _make_balance_result(head_idx, 1);
        }
        else
        {
            left_node->balance = 0;
            node->balance = 0;
            return _make_balance_result(head_idx, 0);
        }
    }
}

static inline void _replace_child(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    uint16_t old_child,
    uint16_t new_child
) {
    if (idx == NULL_IDX)
    {
        assert(buffer_set->root == old_child);
        buffer_set->root = new_child;
    }
    else
    {
        struct node_s * node = _get_node(buffer_set, idx);
#if defined(USE_REFERENCE_CODE)
        if (node->left == old_child)
            node->left = new_child;
        else
        {
            assert(node->right == old_node);
            node->right = new_child;
        }
#else
        const int side = ((node->left == old_child) ? 0 : 1);
        assert((&node->left)[side] == old_child);
        (&node->left)[side] = new_child;
#endif
    }
}

void * buffer_set_insert(
    buffer_set_t * buffer_set,
    const void * value,
    int * inserted
) {
    uint16_t parent_idx = NULL_IDX;
    uint16_t idx = buffer_set->root;
    int cmp;

    for (;;)
    {
        if (idx == NULL_IDX)
            break;

        struct node_s * node = _get_node(buffer_set, idx);
        void * node_value = _node_get_value(node);
        cmp = buffer_set->compar(value, node_value, buffer_set->thunk);
        if (cmp == 0)
        {
            *inserted = 0;
            return node_value;
        }

        parent_idx = idx;
        const int side = ((cmp > 0) ? 1 : 0);
        idx = (&node->left)[side];
    }

    idx = buffer_set->free_list;
    if (idx == NULL_IDX)
    {
        assert((buffer_set->size == 0) || ((buffer_set->size + 1) == buffer_set->capacity));
        if (buffer_set->capacity == MAX_CAPACITY)
            return NULL;

        const uint16_t new_capacity = _calculate_new_capacity(buffer_set->capacity);
        assert(buffer_set->capacity < new_capacity);
        void * buffer = malloc(new_capacity * buffer_set->node_size);
        if (!buffer)
            return NULL;

        if (buffer_set->capacity > 0)
        {
            void (*move)(void*, void*, void*) = buffer_set->move;
            if (move)
            {
                void * thunk = buffer_set->thunk;
                const size_t node_size = buffer_set->node_size;
                size_t offs = node_size;
                for (size_t idx=1; idx<buffer_set->capacity; idx++, offs += node_size)
                {
                    struct node_s * src_node = (void*) (((char*) buffer_set->buffer) + offs);
                    struct node_s * dst_node = (void*) (((char*) buffer) + offs);
                    *dst_node = *src_node;
                    void * src_value = ((char*) src_node) + _round(sizeof(struct node_s));
                    void * dst_value = ((char*) dst_node) + _round(sizeof(struct node_s));
                    move(dst_value, src_value, thunk);
                }
            }
            else
                memcpy(buffer, buffer_set->buffer, buffer_set->capacity * buffer_set->node_size);
        }

        free(buffer_set->buffer);
        buffer_set->capacity = new_capacity;
        buffer_set->buffer = buffer;

        buffer_set->free_list = _make_free_list(
            buffer,
            buffer_set->node_size,
            (buffer_set->size + 1),
            (new_capacity - buffer_set->size - 1)
        );

        idx = buffer_set->free_list;
    }

    const size_t offs = (buffer_set->node_size * idx);
    struct free_node_s * free_node = (struct free_node_s*) (((char*)buffer_set->buffer) + offs);
    buffer_set->free_list = free_node->next;

    struct node_s * node = (struct node_s*) free_node;
    node->left = NULL_IDX;
    node->parent = parent_idx;
    node->right = NULL_IDX;
    node->balance = 0;
    void * ret = _node_get_value(node);

    buffer_set->size++;
    *inserted = 1;

    if (parent_idx == NULL_IDX)
    {
        buffer_set->root = idx;
        return ret;
    }

    struct node_s * parent_node = _get_node(buffer_set, parent_idx);
#if defined(USE_REFERENCE_CODE)
    if (cmp < 0)
    {
        parent_node->left = idx;
        const int8_t balance = --parent_node->balance;
        // parent of the newly inserted node can have only balance -1 or 0 here
        if (balance == 0)
            return ret;
        assert(balance == -1);
    }
    else
    {
        assert(cmp > 0);
        parent_node->right = idx;
        const int8_t balance = ++parent_node->balance;
        // parent of the newly inserted node can have only balance 0 or 1 here
        if (balance == 0)
            return ret;
        assert(balance == 1);
    }
#else
    const int side = ((cmp > 0) ? 1 : 0);
    (&parent_node->left)[side] = idx;
    parent_node->balance += ((cmp > 0) ? 1 : 0);
    parent_node->balance -= ((cmp < 0) ? 1 : 0);
    if (parent_node->balance == 0)
        return ret;
    assert(abs(parent_node->balance) == 1);
#endif

    uint16_t from_idx = parent_idx;
    idx = parent_node->parent;
    while (idx != NULL_IDX)
    {
        node = _get_node(buffer_set, idx);
        node->balance += ((node->left == from_idx) ? -1 : 1);
        if (node->balance == 0)
            break;
        else if (node->balance == -2)
        {
            assert(node->left == from_idx);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_left(buffer_set, idx, node);
            assert(balance_result.height_changed == 0);
            _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            break;
        }
        else if (node->balance == 2)
        {
            assert(node->right == from_idx);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_right(buffer_set, idx, node);
            assert(balance_result.height_changed == 0);
            _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            break;
        }
        assert(abs(node->balance) == 1);
        from_idx = idx;
        idx = node->parent;
    }

    return ret;
}

void * buffer_set_erase(
    buffer_set_t * buffer_set,
    const void * value
) {
    buffer_set_iterator_t * it = buffer_set_find(buffer_set, value);
    if (it == buffer_set_end(buffer_set))
        return NULL;
    else
        return buffer_set_erase_at(buffer_set, it);
}

static void _replace_child_and_rebalance(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    uint16_t old_child,
    uint16_t new_child
);

static void _rebalance(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    uint16_t from_child
) {
    while (idx != NULL_IDX)
    {
        struct node_s * node = _get_node(buffer_set, idx);
        const int8_t balance_change = (node->left == from_child) ? -1 : 1;
        node->balance -= balance_change;
        if (abs(node->balance) == 1)
        {
            // stop balancing
            break;
        }
        else if (node->balance == 2)
        {
            assert(node->left == from_child);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_right(buffer_set, idx, node);
            const uint16_t height_changed = !balance_result.height_changed;
            if (height_changed)
                _replace_child_and_rebalance(buffer_set, parent_idx, idx, balance_result.idx);
            else
                _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            break;
        }
        else if (node->balance == -2)
        {
            assert(node->right == from_child);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_left(buffer_set, idx, node);
            const uint16_t height_changed = !balance_result.height_changed;
            if (height_changed)
                _replace_child_and_rebalance(buffer_set, parent_idx, idx, balance_result.idx);
            else
                _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            break;
        }
        assert(node->balance == 0);
        from_child = idx;
        idx = node->parent;
    }
}

static void _replace_child_and_rebalance(
    struct buffer_set_s * buffer_set,
    uint16_t idx,
    uint16_t old_child,
    uint16_t new_child
) {
    if (idx == NULL_IDX)
    {
        assert(buffer_set->root == old_child);
        buffer_set->root = new_child;
    }
    else
    {
        struct node_s * node = _get_node(buffer_set, idx);
        const int8_t balance_change = ((node->left == old_child) ? -1 : 1);
        const int side = ((node->left == old_child) ? 0 : 1);
        (&node->left)[side] = new_child;
        node->balance -= balance_change;
        if (abs(node->balance) == 1)
        {
            // stop balancing
            return;
        }
        else if (node->balance == 2)
        {
            assert(node->left == new_child);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_right(buffer_set, idx, node);
            const uint16_t height_changed = !balance_result.height_changed;
            if (height_changed)
                _replace_child_and_rebalance(buffer_set, parent_idx, idx, balance_result.idx);
            else
                _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            return;
        }
        else if (node->balance == -2)
        {
            assert(node->right == new_child);
            const uint16_t parent_idx = node->parent;
            const struct balance_result_s balance_result = _balance_left(buffer_set, idx, node);
            const uint16_t height_changed = !balance_result.height_changed;
            if (height_changed)
                _replace_child_and_rebalance(buffer_set, parent_idx, idx, balance_result.idx);
            else
                _replace_child(buffer_set, parent_idx, idx, balance_result.idx);
            return;
        }
        assert(node->balance == 0);
        const uint16_t parent = node->parent;
        if (parent != NULL_IDX)
            _rebalance(buffer_set, parent, idx);
    }
}

void * buffer_set_erase_at(
    buffer_set_t * buffer_set,
    buffer_set_iterator_t * it
) {
    struct node_s * node = (struct node_s*) it;
    const ptrdiff_t offs = (((const char*)node) - ((const char*)buffer_set->buffer));
    assert((offs % buffer_set->node_size) == 0);
    const uint16_t idx = (uint16_t) (offs / buffer_set->node_size);
    if ((node->left != NULL_IDX) && (node->right != NULL_IDX))
    {
        uint16_t tmp_idx = node->right;
        struct node_s * tmp_node = _get_node(buffer_set, tmp_idx);
        for (;;)
        {
            if (tmp_node->left == NULL_IDX)
                break;
            tmp_idx = tmp_node->left;
            tmp_node = _get_node(buffer_set, tmp_idx);
        }

        const uint16_t tmp_parent_idx = tmp_node->parent;
        tmp_node->left = node->left;
        _get_node(buffer_set, node->left)->parent = tmp_idx;
        _get_node(buffer_set, node->right)->parent = tmp_idx;
        tmp_node->parent = node->parent;
        tmp_node->balance = node->balance;

        if (tmp_idx == node->right)
        {
            const int8_t balance = --tmp_node->balance;
            if (balance == -2)
            {
                const struct balance_result_s balance_result = _balance_left(buffer_set, tmp_idx, tmp_node);
                const uint16_t height_changed = !balance_result.height_changed;
                if (height_changed)
                    _replace_child_and_rebalance(buffer_set, node->parent, idx, balance_result.idx);
                else
                    _replace_child(buffer_set, node->parent, idx, balance_result.idx);
            }
            else if (balance == -1)
            {
                // rebalancing is not required
                _replace_child(buffer_set, node->parent, idx, tmp_idx);
            }
            else
            {
                assert(balance == 0);
                _replace_child_and_rebalance(buffer_set, node->parent, idx, tmp_idx);
            }
        }
        else
        {
            const uint16_t tmp_right_idx = tmp_node->right;
            tmp_node->right = node->right;
            _replace_child(buffer_set, tmp_node->parent, idx, tmp_idx);
            _get_node(buffer_set, tmp_right_idx)->parent = tmp_parent_idx;
            _replace_child_and_rebalance(buffer_set, tmp_parent_idx, tmp_idx, tmp_right_idx);
        }
    }
    else if (node->left != NULL_IDX)
    {
        // node->right == NULL_IDX
        const uint16_t parent = node->parent;
        _get_node(buffer_set, node->left)->parent = parent;
        _replace_child_and_rebalance(buffer_set, parent, idx, node->left);
    }
    else
    {
        // node->left == NULL_IDX,
        // node->right can be either NULL_IDX or not
        // since we have a special dummy node at 0,
        // we can safely set the parent there instead of branching
        const uint16_t parent = node->parent;
        _get_node(buffer_set, node->right)->parent = parent;
        _replace_child_and_rebalance(buffer_set, parent, idx, node->right);
    }

    buffer_set->size--;
    struct free_node_s * free_node = (struct free_node_s*) node;
    free_node->next = buffer_set->free_list;
    buffer_set->free_list = idx;

    return _node_get_value(node);
}

static void _buffer_set_print_debug(
    struct buffer_set_s * buffer_set,
    FILE * file,
    void (*value_printer)(FILE *, const void *),
    uint16_t idx
) {
    struct node_s * node = _get_node(buffer_set, idx);
    fprintf(file, "    ");
    value_printer(file, _node_get_value(node));
    fprintf(file, "/%hu", idx);

    fprintf(file, ": parent=");
    if (node->parent == NULL_IDX)
        fprintf(file, "NIL");
    else
        fprintf(file, "%hu", node->parent);

    fprintf(file, " left=");
    if (node->left == NULL_IDX)
        fprintf(file, "NIL");
    else
        fprintf(file, "%hu", node->left);

    fprintf(file, " right=");
    if (node->right == NULL_IDX)
        fprintf(file, "NIL");
    else
        fprintf(file, "%hu", node->right);

    fprintf(file, " balance=%hhd\n", node->balance);

    if (node->left != NULL_IDX)
        _buffer_set_print_debug(buffer_set, file, value_printer, node->left);

    if (node->right != NULL_IDX)
        _buffer_set_print_debug(buffer_set, file, value_printer, node->right);
}

void buffer_set_print_debug(
    buffer_set_t * buffer_set,
    FILE * file,
    void (*value_printer)(FILE * file, const void * value)
) {
    fprintf(file, "{");
    const uint16_t root = buffer_set->root;
    if (root != NULL_IDX)
    {
        fprintf(file, "\n");
        _buffer_set_print_debug(buffer_set, file, value_printer, root);
    }
    fprintf(file, "}");
}

static int _buffer_set_verify(
    buffer_set_t * buffer_set,
    FILE * file,
    uint16_t idx,
    int * height
) {
    struct node_s * node = _get_node(buffer_set, idx);
    int left_height = 0;
    if (node->left != NULL_IDX)
    {
        struct node_s * left_node = _get_node(buffer_set, node->left);
        if (left_node->parent != idx)
        {
            fprintf(file, "left_node->parent(%hu)!=idx(%hu)\n", left_node->parent, idx);
            return -1;
        }
        const int cmp = buffer_set->compar(
            _node_get_value(left_node),
            _node_get_value(node),
            buffer_set->thunk
        );
        assert(cmp < 0);
        if (cmp >= 0)
        {
            fprintf(file, "left node (%hu) value is not less than value in (%hu)\n", node->left, idx);
            return -1;
        }
        int rc = _buffer_set_verify(buffer_set, file, node->left, &left_height);
        if (rc != 0)
            return rc;
    }

    int right_height = 0;
    if (node->right != NULL_IDX)
    {
        struct node_s * right_node = _get_node(buffer_set, node->right);
        if (right_node->parent != idx)
        {
            fprintf(file, "right_node->parent(%hu)!=idx(%hu)\n", right_node->parent, idx);
            return -1;
        }
        const int cmp = buffer_set->compar(
            _node_get_value(node),
            _node_get_value(right_node),
            buffer_set->thunk
        );
        assert(cmp < 0);
        if (cmp >= 0)
        {
            fprintf(file, "right node (%hu) value is not greater than value in (%hu)\n", node->left, idx);
            return -1;
        }
        int rc = _buffer_set_verify(buffer_set, file, node->right, &right_height);
        if (rc != 0)
            return rc;
    }

    const int balance = (right_height - left_height);
    // assert(node->balance == balance);

    if (node->balance != balance)
    {
        fprintf(file, "unexpected balance %hhd instead of %d for node %hu\n", node->balance, balance, idx);
        return -1;
    }

    *height = (left_height > right_height) ? left_height : right_height;
    *height += 1;
    return 0;
}

int buffer_set_verify(
    buffer_set_t * buffer_set,
    FILE * file
) {
    if (buffer_set->root != NULL_IDX)
    {
        int height = 0;
        return _buffer_set_verify(buffer_set, file, buffer_set->root, &height);
    }
    return 0;
}

static uint16_t _buffer_set_clear(
    struct buffer_set_s * buffer_set,
    uint16_t free_list,
    uint16_t idx
) {
    struct node_s * node = _get_node(buffer_set, idx);
    if (node->left != NULL_IDX)
        free_list = _buffer_set_clear(buffer_set, free_list, node->left);
    if (node->right != NULL_IDX)
        free_list = _buffer_set_clear(buffer_set, free_list, node->right);
    struct free_node_s * free_node = _get_free_node(buffer_set, idx);
    free_node->next = free_list;
    return idx;
}

static uint16_t _buffer_set_move_tree(
    buffer_set_t * buffer_set,
    uint16_t src_idx,
    void * src_buffer
) {
    uint16_t idx = ++buffer_set->size;
    size_t node_size = buffer_set->node_size;
    struct node_s * dst_node = _get_node(buffer_set, idx);
    struct node_s * src_node = (void*) (((char*)src_buffer) + (src_idx * node_size));

    uint16_t left = src_node->left;
    if (left != NULL_IDX)
    {
        left = _buffer_set_move_tree(buffer_set, left, src_buffer);
        struct node_s * left_node = _get_node(buffer_set, left);
        left_node->parent = idx;
    }
    dst_node->left = left;

    uint16_t right = src_node->right;
    if (right != NULL_IDX)
    {
        right = _buffer_set_move_tree(buffer_set, right, src_buffer);
        dst_node->right = right;
        struct node_s * right_node = _get_node(buffer_set, right);
        right_node->parent = idx;
    }
    dst_node->right = right;

    dst_node->balance = src_node->balance;

    void (*move)(void*, void*, void*) = buffer_set->move;
    if (move == NULL)
    {
        size_t value_size = (node_size - _round(sizeof(struct node_s)));
        memcpy(
            _node_get_value(dst_node),
            _node_get_value(src_node),
            value_size
        );
    }
    else
        move(dst_node, src_node, buffer_set->thunk);

    return idx;
}

void buffer_set_shrink(buffer_set_t * buffer_set)
{
    uint16_t new_capacity = buffer_set->capacity;
    while ((buffer_set->size + 1) < (new_capacity / 4))
        new_capacity /= 2;

    if (new_capacity < MIN_CAPACITY)
    {
        new_capacity = MIN_CAPACITY;
        if (new_capacity >= buffer_set->capacity)
            return;
    }

    void * buffer = malloc(buffer_set->node_size * new_capacity);
    if (buffer == NULL)
        return;

    void * old_buffer = buffer_set->buffer;
    buffer_set->buffer = buffer;
    buffer_set->capacity = new_capacity;

    uint16_t root = buffer_set->root;
    if (root != NULL_IDX)
    {
        buffer_set->size = 0;
        root = _buffer_set_move_tree(buffer_set, root, old_buffer);
        buffer_set->root = root;
    }

    free(old_buffer);

    buffer_set->free_list = _make_free_list(
        buffer,
        buffer_set->node_size,
        buffer_set->size + 1,
        (new_capacity - buffer_set->size - 1)
    );
}

void buffer_set_clear(buffer_set_t * buffer_set)
{
    const uint16_t root = buffer_set->root;
    if (root != NULL_IDX)
    {
        buffer_set->free_list = _buffer_set_clear(buffer_set, buffer_set->free_list, root);
        buffer_set->root = NULL_IDX;
        buffer_set->size = 0;
    }
}

void buffer_set_destroy(buffer_set_t * buffer_set)
{
    free(buffer_set->buffer);
    free(buffer_set);
}
