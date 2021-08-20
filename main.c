#define _CRT_SECURE_NO_WARNINGS
#include "robin_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QUIT "quit"
#define GET "get"
#define PUT "put"
#define DEL "del"
#define PRINT "print"

void rand_str(char *str, size_t sz)
{
    for (size_t i = 0; i != sz; ++i) {
    }
}

int main()
{
    char op[50];
    char key[50];
    int value;
    int *p;
    robin_map map;

    map = rm_init();

    while (1) {
        scanf(" %s", op);

        if (strcmp(op, QUIT) && strcmp(op, GET) && strcmp(op, PUT) && strcmp(op, DEL))
            goto error;

        if (op[0] == 'Q' || strcmp(op, QUIT) == 0)
            return 0;

        if (scanf(" %s", key) != 1)
            goto error;

        if (strcmp(op, PUT) == 0) {
            if (scanf(" %d", &value) != 1)
                goto error;
            p = rm_put(&map, key, value);
        } else if (strcmp(op, DEL) == 0) {
            printf("Removed %d\n", rm_remove(&map, key));
            continue;
        } else {
            p = rm_get(&map, key);
        }

        if (!p)
            puts("Not Found");
        else
            printf("`%s` -> `%d`\n", key, *p);

        void print_map(robin_map * map);
        print_map(&map);

        continue;

    error:
        puts("Illegal input");
    }
    rm_deinit(&map);
}
