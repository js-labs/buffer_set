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

#include <set>
#include <sys/time.h>

#define COUNT (10*1000)

int main(int argc, const char * argv[])
{
    std::set<int> set;

    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);

    for (int idx=0; idx<COUNT; idx++)
        set.insert(idx);

    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);

    const unsigned int delay = ((tv_end.tv_sec - tv_start.tv_sec) * 1000000 + tv_end.tv_usec - tv_start.tv_usec);
    printf("inserted values [0...%u] \(%zu) @ %u usec\n", COUNT-1, set.size(), delay);

    return 0;
}
