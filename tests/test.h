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

#if !defined(BUFFER_SET_TEST_H)
#define BUFFER_SET_TEST_H

#include <stdio.h>

typedef enum
{
    operation_type_insert,
    operation_type_erase
}
operation_type_t;

struct operation_s
{
    operation_type_t type;
    int value;
};

extern int int_cmp(const void * v1, const void * v2, void * thunk);
extern void int_printer(FILE * file, const void * v);

#endif /* BUFFER_SET_TEST_H */
