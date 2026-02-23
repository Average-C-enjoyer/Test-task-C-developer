#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "stat_data.h"

#define MODE_MAX(a,b)  ((a) ^ (((a) ^ (b)) & -( (a) < (b) )))

// Hash Table implementation (SoA and Robin Hood hash)
typedef struct {
    StatData       *data;
	size_t         *dest_idx;       // index in the output array for each entry
    unsigned char  *occupied_bits;  // bit flags
} HashEntry;

typedef struct {
    HashEntry       arr;
	size_t          size;           // power of 2 so load factor < 0.5
} HashTable;


// Utility functions
static inline size_t next_pow2(size_t x)
{
    size_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

static inline void merge_data(StatData *dest,
    const StatData *el1,
    const StatData *el2)
{
    dest->id = el1->id;
    dest->cost = el1->cost + el2->cost;
    dest->count = el1->count + el2->count;
    dest->primary = el1->primary && el2->primary;
    dest->mode = MODE_MAX(el1->mode, el2->mode);
}

static inline size_t hash_long(long key, size_t hsize) {
    return ((unsigned long)key) & (hsize - 1);
}

// bitset helpers for occupied_bits
static inline int occupied_get(const unsigned char *bits, size_t idx) {
    return (bits[idx >> 3] >> (idx & 7)) & 1;
}
static inline void occupied_set(unsigned char *bits, size_t idx) {
    bits[idx >> 3] |= (1u << (idx & 7));
}
static inline void occupied_clear(unsigned char *bits, size_t idx) {
    bits[idx >> 3] &= ~(1u << (idx & 7));
}


// Hash Table functions
extern inline void ht_init(HashTable *ht, size_t count1, size_t count2) {
	ht->size = next_pow2((count1 + count2) * 2);
    ht->arr.data = (StatData *)malloc(ht->size * sizeof(StatData));
    ht->arr.dest_idx = (size_t *)malloc(ht->size * sizeof(size_t));

    size_t bytes = (ht->size + 7) / 8;
    ht->arr.occupied_bits = (unsigned char *)calloc(bytes, 1);

    if (!ht->arr.data || !ht->arr.dest_idx || !ht->arr.occupied_bits) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
}

extern inline void ht_free(HashTable *ht) {
    if (ht->arr.data) { 
        free(ht->arr.data); 
        ht->arr.data = NULL; 
    }
    if (ht->arr.dest_idx) { 
        free(ht->arr.dest_idx); 
        ht->arr.dest_idx = NULL; 
    }
    if (ht->arr.occupied_bits) { 
        free(ht->arr.occupied_bits); 
        ht->arr.occupied_bits = NULL; 
    }
}

// Probe-distance for Robin Hood hash
static inline size_t probe_distance(size_t cur_idx, size_t ideal_idx, size_t size) {
    return (cur_idx + size - ideal_idx) & (size - 1);
}


// Find element by probing. returns pointer to StatData or NULL if not found
extern inline StatData *ht_get(HashTable *ht, long id) {
    size_t size = ht->size;
    size_t ideal = hash_long(id, size);
    size_t idx = ideal;
    for (;;) {
        if (!occupied_get(ht->arr.occupied_bits, idx))
            return NULL; // not found
        if (ht->arr.data[idx].id == id)
            return &ht->arr.data[idx];
        idx = (idx + 1) & (size - 1);
    }
}

// Insert or merge using Robin Hood hashing.
// dest: external array where we store compacted results.
// curr_idx: pointer to next free index in dest 
// (only incremented when a NEW element is assigned a dest index)
extern inline void ht_merge_or_insert(HashTable *ht, const StatData *el, StatData *dest, size_t *curr_idx) {
    size_t size = ht->size;
    size_t ideal = hash_long(el->id, size);
    size_t idx = ideal;

    StatData cur = *el;
    size_t cur_dest = *curr_idx; // tentative dest index for the incoming element
    int is_initial = 1; // true for original element, false for displaced ones

    for (;;) {
        if (!occupied_get(ht->arr.occupied_bits, idx)) {
			// Empty slot, place element here
            ht->arr.data[idx] = cur;
            ht->arr.dest_idx[idx] = cur_dest;
            occupied_set(ht->arr.occupied_bits, idx);
            dest[cur_dest] = cur;
            if (is_initial) (*curr_idx)++;
            return;
        }

		// Merge if same ID
        if (ht->arr.data[idx].id == cur.id) {
            merge_data(&ht->arr.data[idx], &ht->arr.data[idx], &cur);
            dest[ht->arr.dest_idx[idx]] = ht->arr.data[idx];
            return;
        }

		// Compare probe distances
        size_t ideal_existing = hash_long(ht->arr.data[idx].id, size);
        size_t dist_existing = probe_distance(idx, ideal_existing, size);
        size_t dist_cur = probe_distance(idx, hash_long(cur.id, size), size);

        if (dist_cur > dist_existing) {
			// Current element probed farther, so it steals this slot
            StatData tmp = ht->arr.data[idx];
            size_t tmp_dest = ht->arr.dest_idx[idx];

			// Place current element here
            ht->arr.data[idx] = cur;
            ht->arr.dest_idx[idx] = cur_dest;
            dest[cur_dest] = cur; // update dest for element now residing in slot
            if (is_initial) { (*curr_idx)++; is_initial = 0; }

			// Reinsert displaced element
            cur = tmp;
            cur_dest = tmp_dest;
			// Displaced element isn't initial
            is_initial = 0;
        }

        idx = (idx + 1) & (size - 1);
    }
}
