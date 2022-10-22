# crobin

This is an implementation of a "string to int" hash table with [Robin Hood hashing](https://en.wikipedia.org/wiki/Hash_table#Robin_Hood_hashing). It is kept ANSI C for simplicity.

Sample usage:

```c
#include <stdio.h>
#include "robin_map.h"

int main() {
    robin_map map = rm_init();
    char* keys[] = {
        "hello", "world"
    };
    int sz = (int) sizeof keys / sizeof *keys;
    int i;

    for (i = 0; i != sz; ++i) rm_put(&map, keys[i], i);

    for (i = 0; i != sz; ++i) 
        printf("`%s` -> %d\n", keys[i], *rm_get(&map, keys[i]));
}
```
