# Buffer set

The library enables storing a set of values in a binary search tree within a contiguous memory buffer. Tree nodes use 16-bit indices instead of pointers to reference other nodes, which reduces the amount of memory required for the tree. As a result, the maximum number of values that can be stored is 65,535.

It can also be used as a map, assuming the value is a (key, value) pair and the comparison function handles it appropriately.

The tree rebalancing implementation is based on the AVL (Adelson-Velsky and Landis) algorithm.