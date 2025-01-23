#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include <console/io/keys.h>
#include <mapping.h>
#include <vector.h>

struct keymap_assoc {
        key_ty key;
        struct keymap *next;
};

int compare_keymap_assoc(const void *e1, const void *e2) {
        const struct keymap_assoc
                *km1 = e1,
                *km2 = e2;

        return compare_key_ty(&km1->key, &km2->key);
}

struct keymap {
        vector_t *keys;
        union {
                vector_t *assocs;
                command_t cmd;
        };
        enum {
                KEYMAP_NODE_FINAL,
                KEYMAP_NODE_MIDDLE,
        } type;
};


struct keymap keymap_init(void) {
        return (struct keymap){
                .type = KEYMAP_NODE_FINAL,
                .assocs = vector_init(sizeof(struct keymap_assoc), compare_keymap_assoc)
        };
}

int keymap_add(struct keymap *km, key_ty *arr, size_t len, command_t cmd) {
        if (len == 0) {
                if (km->type == KEYMAP_NODE_MIDDLE) {
                        return -1;
                }
        }
}

#endif
