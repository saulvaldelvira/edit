#ifndef __PREFIX_TREE_H__
#define __PREFIX_TREE_H__

#include "console/io/keys.h"
#include "hash_map.h"
#include "mapping.h"

typedef struct trie_node {
        hash_map_t *keys;
        mapping_t mapping;
} trie_node_t;

trie_node_t trie_new();

void trie_add(trie_node_t *trie, key_ty *keys, size_t keys_len, mapping_t mapping);

mapping_t trie_search(trie_node_t *trie, key_ty *keys, size_t keys_len);

trie_node_t* trie_get_next(trie_node_t *trie, key_ty key);

bool trie_is_leaf(const trie_node_t *trie);

void trie_free(trie_node_t *trie);

void trie_foreach(const trie_node_t *trie, void (*func)(const key_ty*, size_t, mapping_t));

#endif /* __PREFIX_TREE_H__ */
