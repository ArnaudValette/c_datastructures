#ifndef DATASTRUCTURES_SET_H
#define DATASTRUCTURES_SET_H
#include "crypto.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DATASTRUCTURES_SET_INITIAL_SIZE (128)
/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § set → {(void *) based implementation}                   ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

typedef struct set_entry {
  void *key;
  size_t key_len;
  uint64_t hash;
  uint8_t value;
  struct set_entry *next;
} set_entry;

typedef struct set {
  set_entry **buckets;
  uint64_t seed;
  size_t width;
  size_t size;
} set;

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § set → methods (untyped)                                 ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/**
 * INTERNAL_DO_NOT_USE
 * Check whether inserting one additional element would exceed
 * the load factor threshold.
 *
 * Uses a fixed 0.75 threshold.
 */
static bool __set_check_load_factor(set *set) {
  return (set->size + 1) * 4 >= set->width * 3;
}

/**
 * INTERNAL_DO_NOT_USE
 */
static void __set_rehash_add_entry(set_entry *e, set_entry **nE, size_t nW) {
  uint64_t new_hash = e->hash % nW;
  e->next = nE[new_hash];
  nE[new_hash] = e;
}

/**
 * INTERNAL_DO_NOT_USE
 * Resize the set by doubling its bucket array.
 *
 * Existing entries are rehashed using their stored hash values.
 * Keys are not reallocated.
 *
 * Returns false on allocation failure or size overflow.
 */
static bool __set_resize_width(set *set) {
  if (set->width > (SIZE_MAX) / 2 / sizeof(set_entry *)) {
    return false;
  }
  size_t new_width = set->width * 2;
  set_entry **new_entries =
      (set_entry **)calloc(new_width, sizeof(set_entry *));

  if (!new_entries)
    return false;

  for (size_t i = 0; i < set->width; i++) {
    set_entry *e = set->buckets[i];
    while (e) {
      set_entry *next = e->next;
      __set_rehash_add_entry(e, new_entries, new_width);
      e = next;
    }
  }
  free(set->buckets);
  set->buckets = new_entries;
  set->width = new_width;
  return true;
}

/**
 * INTERNAL_DO_NOT_USE
 */
static bool __set_key_compare(void *key1, size_t len1, void *key2,
                              size_t len2) {
  if (len2 != len1)
    return false;
  uint8_t *b1 = (uint8_t *)key1;
  uint8_t *b2 = (uint8_t *)key2;
  for (size_t i = 0; i < len1; i++) {
    if (b1[i] != b2[i])
      return false;
  }
  return true;
}

/**
 * Create a new set.
 *
 * `seed` is mixed into the hash function and should remain constant
 * for the lifetime of the hashmap.
 *
 * Keys are immutable byte snapshots.
 * Any logical key semantics beyond raw bytes must be enforced by the caller.
 *
 * Returns NULL on allocation failure.
 */
static set *set_new(uint64_t seed) {
  set *set = (struct set *)malloc(sizeof(struct set));
  if (!set)
    return NULL;
  set->width = DATASTRUCTURES_SET_INITIAL_SIZE;
  set->seed = seed;
  set->size = 0;
  set->buckets = (set_entry **)calloc(set->width, sizeof(set_entry *));
  if (!set->buckets) {
    free(set);
    return NULL;
  }
  return set;
}

/**
 * Include the object in the set
 *
 * The key is treated as an opaque byte sequence of length `len`
 * and is copied into freshly allocated memory.
 *
 * If an identical key already exists (byte-wise equality),
 * its associated value is replaced.
 *
 * The set takes ownership of the copied key data.
 *
 * Keys are immutable byte snapshots.
 * Any logical key semantics beyond raw bytes must be enforced by the caller.
 *
 * Returns false on allocation failure.
 */
static bool set_insert(set *set, void *key, size_t len) {
  set_entry *ne;
  if (__set_check_load_factor(set)) {
    if (!__set_resize_width(set)) {
      return false;
    }
  }

  uint64_t hash = datastruct_hash(key, len, set->seed);
  uint64_t idx = hash % set->width;
  for (set_entry *e = set->buckets[idx]; e; e = e->next) {
    if (e->hash == hash && __set_key_compare(key, len, e->key, e->key_len)) {
      e->value = 0x0001;
      return true;
    }
  }
  ne = (set_entry *)malloc(sizeof(set_entry));
  if (!ne)
    return false;
  ne->hash = hash;
  ne->key = malloc(len);
  if (!ne->key) {
    free(ne);
    return false;
  }
  memcpy(ne->key, key, len);
  ne->key_len = len;
  ne->value = 0x0001;
  ne->next = set->buckets[idx];
  set->buckets[idx] = ne;
  set->size++;
  return true;
}

/**
 * Does the data exist in the set:
 *
 * Key comparison is performed using byte-wise equality over (key, len).
 *
 * Returns the stored value, or 0 if the key is not present.
 */
static uint8_t set_contains(set *set, void *key, size_t len) {
  uint64_t hash = datastruct_hash(key, len, set->seed);
  size_t idx = hash % set->width;
  for (set_entry *e = set->buckets[idx]; e; e = e->next) {
    if (e->hash == hash && __set_key_compare(key, len, e->key, e->key_len)) {
      return e->value & 0x0001;
    }
  }
  return 0;
}

/**
 * Removes the object from the set
 *
 * Returns true if an entry was removed, false if the entry was not found.
 */
static bool set_remove(set *set, void *key, size_t len) {
  uint64_t hash = datastruct_hash(key, len, set->seed);
  size_t idx = hash % set->width;
  set_entry *prev = NULL;
  set_entry *e = set->buckets[idx];

  while (e) {
    if (e->hash == hash && __set_key_compare(key, len, e->key, e->key_len)) {
      if (prev) {
        prev->next = e->next;
      } else {
        set->buckets[idx] = e->next;
      }
      free(e->key);
      free(e);
      set->size--;
      return true;
    }
    prev = e;
    e = e->next;
  }
  return false;
}

/**
 * Destroy the set
 *
 * Frees all internal structures and key buffers.
 */
static void set_destroy(set *set) {
  if (!set)
    return;
  for (size_t i = 0; i < set->width; i++) {
    set_entry *e = set->buckets[i];
    while (e) {
      set_entry *next = e->next;
      free(e->key);
      free(e);
      e = next;
    }
  }
  free(set->buckets);
  free(set);
}

#endif
