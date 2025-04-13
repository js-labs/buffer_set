#if !defined(BUFFER_SET_TEST_H)
#define BUFFER_SET_TEST_H

#include <stdio.h>

#define NOT_ENOUGH_MEMORY ((void*)1)

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

extern int int_cmp(const void * v1, const void * v2);
extern void int_printer(FILE * file, const void * v);

#endif /* BUFFER_SET_TEST_H */
