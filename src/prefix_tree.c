
#include "console/io/keys.h"
#include "hash_map.h"
#include <stdint.h>
#include "prefix_tree.h"

int64_t hash_key_t(const void *e) {
        key_ty *k = (key_ty*)e;
        return (int64_t)k->k << 2 & (k->modif & 0b11);
}


int cmp_key_t(const void *e1, const void *e2) {
        key_ty *k1 = (key_ty*)e1;
        key_ty *k2 = (key_ty*)e2;

        if (k1->k == k2->k) {
                if (k1->modif == k2->modif)
                        return 0;
                else
                        return k1->modif > k2->modif ? 1 : -1;
        } else {
                return k1->k > k2->k ? 1 : -1;
        }
}

void trie_destroy(void *e) {
        trie_node_t *t = (trie_node_t*)e;
        trie_free(t);
}

trie_node_t trie_new() {
        hash_map_t *keys = hashmap_init(sizeof(key_ty), sizeof(trie_node_t), hash_key_t, cmp_key_t);
        hashmap_set_destructor(keys, trie_destroy);
        return (trie_node_t) {
                .cmd_id = -1,
                .keys = keys,
        };
}

void trie_add(trie_node_t *trie, key_ty *keys, size_t keys_len, int cmd_id) {
        trie_node_t *curr = trie;

        for (size_t i = 0; i < keys_len; i++) {
                key_ty key = keys[i];

                trie_node_t *next = hashmap_get_ref(curr->keys, &key);
                if (next == NULL) {
                        trie_node_t tmp = trie_new();
                        hashmap_put(curr->keys, &key, &tmp);
                        next =  hashmap_get_ref(curr->keys, &key);
                }

                curr = next;
        }

        curr->cmd_id = cmd_id;
}

int trie_search(trie_node_t *trie, key_ty *keys, size_t keys_len) {
        trie_node_t *curr = trie;

        for (size_t i = 0; i < keys_len; i++) {
                key_ty key = keys[i];

                trie_node_t *next = hashmap_get_ref(curr->keys, &key);
                if (next == NULL) {
                        return -1;
                }

                curr = next;
        }

        return curr->cmd_id;
}

trie_node_t* trie_get_next(trie_node_t *trie, key_ty key) {
        return hashmap_get_ref(trie->keys, &key);
}

bool trie_is_leaf(trie_node_t *trie) {
        return hashmap_length(trie->keys) == 0;
}

void trie_free(trie_node_t *trie) {
        hashmap_free(trie->keys);
}
