/*
 * Copyright (C) 2025 Sergey Zubarev, info@js-labs.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if !defined(BUFFER_SET_H)
#define BUFFER_SET_H

#include <stdint.h>
#include <stdio.h>

#if defined (_WIN32)
    #if defined(BUILDING_BUFFER_SET_DLL)
        #define BUFFER_SET_EXPORT __declspec(dllexport)
    #else
        #define BUFFER_SET_EXPORT __declspec(dllimport)
    #endif
#else
    #define BUFFER_SET_EXPORT
#endif

typedef struct buffer_set_s buffer_set_t;

BUFFER_SET_EXPORT buffer_set_t * buffer_set_create(
    size_t value_size,
    size_t capacity,
    int (*compar)(const void * v1, const void * v2)
);

BUFFER_SET_EXPORT void * buffer_set_insert(
    buffer_set_t * buffer_set,
    const void * value,
    int * inserted
);

BUFFER_SET_EXPORT size_t buffer_set_get_size(
    buffer_set_t * buffer_set
);

BUFFER_SET_EXPORT void * buffer_set_get(
    buffer_set_t * buffer_set,
    const void * value
);

BUFFER_SET_EXPORT int buffer_set_erase(
    buffer_set_t * buffer_set,
    const void * value,
    void * erased_value
);

BUFFER_SET_EXPORT void buffer_set_for_each(
    const buffer_set_t * buffer_set,
    void (*func)(const void * value, void * arg),
    void * arg
);

BUFFER_SET_EXPORT void buffer_set_print_debug(
    const buffer_set_t * buffer_set,
    FILE * file,
    void (*value_printer)(FILE * file, const void * value)
);

BUFFER_SET_EXPORT void buffer_set_destroy(buffer_set_t * buffer_set);

#endif /* BUFFER_SET_H */
