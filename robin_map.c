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
    unsigned psl; /* probe sequence lengths */
} rm_element;

static size_t strhash(char *str)
{
    enum { multiplier = 31 };
    size_t hash = 0;
    unsigned char *p = (unsigned char *)str;

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

    for (i = 0; i != old_buffer_size; ++i)
    {
        size_t index;

        if (!slot_is_occupied(old_buffer + i))
            continue;

        old_buffer[i].psl = 0;
        index = strhash(old_buffer[i].key) % map->buffer_size;

        for (;; ++old_buffer[i].psl, index = (index + 1) % map->buffer_size)
        {
            rm_element *slot = ((rm_element *)map->data) + index;

            if (!slot_is_occupied(slot))
            {
                *slot = old_buffer[i];
                break;
            }

            if (old_buffer[i].psl > slot->psl)
            {
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
    double load_factor = ((double)map->element_count) / map->buffer_size;

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
    unsigned psl = 0;

    index = strhash(key) % map->buffer_size;

    for (;; ++psl, index = (index + 1) % map->buffer_size)
    {
        rm_element *slot = buffer + index;

        if (!slot_is_occupied(slot) || psl > slot->psl)
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
    rm_element new_element;
    size_t index; /* Hashing _must_ happen after rehashing */
    bool key_is_duplicated = false;
    int *newly_inserted = NULL;

    grow_and_rehash_if_needed(map);

    index = strhash(key) % map->buffer_size;

    new_element.key = key;
    new_element.value = value;
    new_element.psl = 0;

    for (;; ++new_element.psl, index = (index + 1) % map->buffer_size)
    {
        rm_element *slot = (rm_element *)(map->data) + index;

        if (!slot_is_occupied(slot))
        {
            *slot = new_element;
            ++map->element_count;

            if (!key_is_duplicated)
            {
                slot->key = str_dup(slot->key);
                newly_inserted = &slot->value;
            }
            break;
        }

        if (new_element.psl > slot->psl)
        {
            rm_element tmp = new_element;
            new_element = *slot;
            *slot = tmp;

            if (!key_is_duplicated)
            {
                slot->key = str_dup(slot->key);
                key_is_duplicated = true;
                newly_inserted = &slot->value;
            }
        }

        if (!key_is_duplicated && strcmp(slot->key, new_element.key) == 0)
        {
            slot->value = new_element.value;
            newly_inserted = &slot->value;
            break;
        }
    }
    return newly_inserted;
}

static void rm_pop_element(robin_map *map, rm_element *slot)
{
    rm_element *buffer = map->data;
    size_t i = slot - buffer;

    free(slot->key);
    
    --(map->element_count);

    for (;;) {
        size_t prev = i;
        i = (i + 1) % map->buffer_size;

        if (buffer[i].psl == 0) {
            buffer[prev].key = NULL;
            break;
        } else {
            buffer[prev] = buffer[i];
            --(buffer[prev].psl);
        }
    }
}

int rm_remove(robin_map *map, char *key)
{
    rm_element *slot = rm_get_impl(map, key);
    int popped = 0;

    if (slot)
    {
        popped = slot->value;
        rm_pop_element(map, slot);
    }

    return popped;
}
