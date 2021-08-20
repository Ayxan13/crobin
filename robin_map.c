#include "robin_map.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum { false = 0, true = 1 } bool;

static void *xmalloc(size_t size)
{
    void *p = malloc(size);

    if (!p)
        exit(EXIT_FAILURE);

    return p;
}

static void *xcalloc(size_t count, size_t size)
{
    void *p = calloc(count, size);

    if (!p)
        exit(EXIT_FAILURE);

    return p;
}

static char *str_dup(char *str)
{
    size_t len = strlen(str) + 1;
    char *duplicate = xmalloc(len);

    memcpy(duplicate, str, len);

    return duplicate;
}

/* Type of elements in a robin_map buffer */
typedef struct rm_element
{
    char *key;
    int value;
    int psl; /* probe sequence lengths */
} rm_element;

static size_t strhash(char *str)
{
    enum { multiplier = 31 };
    size_t hash = 0;
    unsigned char *p = (unsigned char *) str;

    for (; *p; ++p)
        hash = multiplier * hash + *p;

    return hash;
}

/* Allocates `size` elements and sets all values to null/zero */
static rm_element *alloc_elements(size_t size)
{
    /* calloc is not guaranteed to set pointers to null,
     * but it does work everywhere
     **/
    return xcalloc(size, sizeof(rm_element));
}

static bool slot_is_dead(rm_element *slot)
{
    return slot->psl == -1;
}

static bool slot_is_occupied(rm_element *slot)
{
    return slot->key != NULL;
}

static void rehash(robin_map *map)
{
    /* 
     * Since our hash algorithm is good, 
     * a growth factor of 2 should be fine
     **/
    enum { growth_factor = 2 };

    rm_element *old_buffer = map->data;
    size_t old_buffer_size = map->buffer_size;
    size_t i;

    map->buffer_size *= growth_factor;
    map->data = alloc_elements(map->buffer_size);

    for (i = 0; i != old_buffer_size; ++i) {
        size_t index;

        if (!slot_is_occupied(old_buffer + i))
            continue;

        old_buffer[i].psl = 0;
        index = strhash(old_buffer[i].key) % map->buffer_size;

        for (;; ++old_buffer[i].psl, index = (index + 1) % map->buffer_size) {
            rm_element *slot = ((rm_element *) map->data) + index;

            if (!slot_is_occupied(slot)) {
                *slot = old_buffer[i];
            }

            if (old_buffer[i].psl > slot->psl) {
                rm_element tmp = old_buffer[i];
                old_buffer[i] = *slot;
                *slot = tmp;
            }
        }
    }

    free(old_buffer);
}

static void grow_and_rehash_if_needed(robin_map *map)
{
    double max_load_factor = 0.7;
    double load_factor = ((double) map->element_count) / map->buffer_size;

    if (load_factor >= max_load_factor)
        rehash(map);
}

void rm_deinit(robin_map *map)
{
    rm_element *buffer = map->data;
    size_t i;

    for (i = 0; i != map->buffer_size; ++i)
        free(buffer[i].key);

    free(buffer);
}

robin_map rm_init()
{
    enum { initial_buffer_size = 47 };
    return rm_init_with_size(initial_buffer_size);
}

robin_map rm_init_with_size(size_t initial_buffer_size)
{
    robin_map map;

    assert(initial_buffer_size >= 5);

    map.data = alloc_elements(initial_buffer_size);
    map.buffer_size = initial_buffer_size;
    map.element_count = 0;

    return map;
}

static rm_element *rm_get_impl(robin_map *map, char *key)
{
    rm_element *buffer = map->data;
    size_t index = 0;
    int psl = 0;

    index = strhash(key) % map->buffer_size;

    for (;; ++psl, index = (index + 1) % map->buffer_size) {
        rm_element *slot = buffer + index;

        if (slot_is_dead(slot))
            continue;

        if (!slot_is_occupied(slot) || slot->psl > psl)
            return NULL;

        if (strcmp(key, slot->key) == 0)
            return slot;
    }
}

int *rm_get(robin_map *map, char *key)
{
    rm_element *slot = rm_get_impl(map, key);
    return slot ? &slot->value : NULL;
}

int *rm_put(robin_map *map, char *key, int value)
{
    rm_element new_elem;
    rm_element *buffer = map->data;
    size_t index = strhash(key) % map->buffer_size;
    bool key_is_duplicated = false;

    grow_and_rehash_if_needed(map);

    new_elem.key = key;
    new_elem.psl = 0;
    new_elem.value = value;

    for (;; ++new_elem.psl, index = (index + 1) % map->buffer_size) {
        rm_element *slot = buffer + index;

        if (!slot_is_occupied(slot)) {
            *slot = new_elem;

            if (!key_is_duplicated)
                slot->key = str_dup(slot->key);

            ++map->element_count;

            return &buffer[index].value;
        }

        if (strcmp(slot->key, new_elem.key) == 0) {
            slot->value = new_elem.value;
            return &slot->value;
        }

        if (slot->psl > new_elem.psl) {
            rm_element tmp = new_elem;
            new_elem = *slot;
            *slot = tmp;

            slot->key = str_dup(slot->key);
            key_is_duplicated = true;
        }
    }
}

int rm_remove(robin_map *map, char *key)
{
    rm_element *slot = rm_get_impl(map, key);
    if (!slot)
        return 0;

    free(slot->key);
    slot->key = NULL;
    slot->psl = -1;
    --map->element_count;

    return slot->value;
}

// TODO delete this
void print_map(robin_map *map)
{
    system("cls");
    size_t i;
    int printf(char const *, ...);
    rm_element *data = map->data;
    printf("%3zu - %2zu\n--------------------------\n", map->element_count, map->buffer_size);

    for (i = 0; i != map->buffer_size; ++i) {
        printf("%3zu. %10s -> %4d | pls: %d\n", i, data[i].key, data[i].value, data[i].psl);
    }
}
