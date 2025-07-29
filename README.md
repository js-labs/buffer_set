[![codecov](https://codecov.io/github/js-labs/buffer_set/graph/badge.svg?token=FIALQ1ROBK)](https://codecov.io/github/js-labs/buffer_set)

# Buffer set

The library enables storing a set of values in a binary search tree within a contiguous memory buffer. Tree nodes use 16-bit indices instead of pointers to reference other nodes, which reduces the amount of memory required for the tree. As a result, the maximum number of values that can be stored is 65,534. All values are supposed to be of the same size.

It can also be used as a map, assuming the value is a (key, value) pair and the comparison function handles it appropriately.

The tree rebalancing implementation is based on the AVL (Adelson-Velsky and Landis) algorithm.

# Usage
Creating an instace of set:
```C
int int_cmp(const void * pv1, const void * pv2)
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
...
buffer_set_t * buffer_set = buffer_set_create(sizeof(int), 16/*initial capacity*/, &int_cmp);
```
Inserting a new element:
```C
int value = 42;
int inserted = 0;
int * buffer_value = buffer_set_insert(buffer_set, &value, &inserted);
if (inserted)
    *buffer_value = value;
```
Getting an elelemnt:
```C
int value = 42;
int * buffer_value = buffer_set_get(buffer_set, &value);
```
