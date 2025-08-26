#ifndef __PREFIX_TREE_H__
#define __PREFIX_TREE_H__

#include "console/io/keys.h"
#include "hash_map.h"

typedef struct trie_node {
        hash_map_t *keys;
        int cmd_id;
} trie_node_t;

trie_node_t trie_new();


void trie_add(trie_node_t *trie, key_ty *keys, size_t keys_len, int cmd_id);

int trie_search(trie_node_t *trie, key_ty *keys, size_t keys_len);

trie_node_t* trie_get_next(trie_node_t *trie, key_ty key);

bool trie_is_leaf(trie_node_t *trie);

void trie_free(trie_node_t *trie);

#endif /* __PREFIX_TREE_H__ */
