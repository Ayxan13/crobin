#ifndef ROBIN_MAP_H
#define ROBIN_MAP_H
#pragma once

typedef struct robin_map
{
    size_t buffer_size;
    size_t element_count;
    void *data;
} robin_map;

robin_map rm_init();

/* Initial size must be >= 5. Otherwise it is UB. */
robin_map rm_init_with_size(size_t size);

void rm_deinit(robin_map *map);

/* `key` and `map` cannot bu null. Null pointer cause UB */
int *rm_get(robin_map *map, char *key);
int *rm_put(robin_map *map, char *key, int value);

/* Returns removed value. Returns 0 if value not found */
int rm_remove(robin_map *map, char *key);

#endif /* ROBIN_MAP_H */
