#ifndef DATASTRUCTURES_HASHING_FUNCS_H
#define DATASTRUCTURES_HASHING_FUNCS_H
#include "crypto.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HASHMAP_INITIAL_SIZE (128)


/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → macro based implementation                    ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

static bool __hashmap_key_compare(uint8_t *b1, size_t len1, uint8_t *b2,
                                  size_t len2) {
  if (len2 != len1)
    return false;
  for (size_t i = 0; i < len1; i++) {
    if (b1[i] != b2[i])
      return false;
  }
  return true;
}

/**
 * @def HASHMAP
 * The user must provide:
 *
 *   uint8_t *name##_key_bytes(kType *key, size_t *len);
 *   void name##_tkey_dispose_hook(uint8_t *key);
 *
 * This function must return a canonical byte representation of the key.
 * The returned buffer must remain valid after the function returns, for
 * the duration of the hashmap operation.
 *
 * The hashmap copies the key bytes and takes ownership of the copy.
 */
#define HASHMAP(kType, vType, name)                                            \
  typedef struct name {                                                        \
    uint8_t *key;                                                              \
    size_t key_len;                                                            \
    uint64_t hash;                                                             \
    vType *value;                                                              \
    struct name *next;                                                         \
  } name;                                                                      \
  typedef struct {                                                             \
    name **entries;                                                            \
    uint64_t seed;                                                             \
    size_t width;                                                              \
    size_t size;                                                               \
  } name##_hashmap;                                                            \
                                                                               \
  uint8_t *name##_key_bytes(kType *k, size_t *len);                            \
  void name##_tkey_dispose_hook(uint8_t *key);                                 \
                                                                               \
  static bool name##__hashmap_check_load_factor(name##_hashmap *hm) {          \
    return (hm->size + 1) * 4 >= hm->width * 3;                                \
  }                                                                            \
                                                                               \
  static void name##__rehash_add_entry(name *e, name **nE, size_t nW) {        \
    uint64_t new_hash = e->hash % nW;                                          \
    e->next = nE[new_hash];                                                    \
    nE[new_hash] = e;                                                          \
  }                                                                            \
                                                                               \
  static bool name##__hashmap_resize_width(name##_hashmap *hm) {               \
    if (hm->width > (SIZE_MAX) / 2 / sizeof(name *)) {                         \
      return false;                                                            \
    }                                                                          \
    size_t new_width = hm->width * 2;                                          \
    name **new_entries = (name **)calloc(new_width, sizeof(name *));           \
                                                                               \
    if (!new_entries)                                                          \
      return false;                                                            \
                                                                               \
    for (size_t i = 0; i < hm->width; i++) {                                   \
      name *e = hm->entries[i];                                                \
      while (e) {                                                              \
        name *next = e->next;                                                  \
        name##__rehash_add_entry(e, new_entries, new_width);                   \
        e = next;                                                              \
      }                                                                        \
    }                                                                          \
    free(hm->entries);                                                         \
    hm->entries = new_entries;                                                 \
    hm->width = new_width;                                                     \
    return true;                                                               \
  }                                                                            \
                                                                               \
  static name##_hashmap *name##_hashmap_new(uint64_t seed) {                   \
    name##_hashmap *hm = (name##_hashmap *)malloc(sizeof(name##_hashmap));     \
    if (!hm)                                                                   \
      return NULL;                                                             \
    hm->width = HASHMAP_INITIAL_SIZE;                                          \
    hm->seed = seed;                                                           \
    hm->size = 0;                                                              \
    hm->entries = (name **)calloc(hm->width, sizeof(name *));                  \
    if (!hm->entries) {                                                        \
      free(hm);                                                                \
      return NULL;                                                             \
    }                                                                          \
    return hm;                                                                 \
  }                                                                            \
                                                                               \
  static bool name##_hashmap_put(name##_hashmap *hm, kType *key,               \
                                 vType *value) {                               \
    name *ne;                                                                  \
    if (name##__hashmap_check_load_factor(hm)) {                               \
      if (!name##__hashmap_resize_width(hm)) {                                 \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t len = 0;                                                            \
    uint8_t *key_bytes = name##_key_bytes(key, &len);                          \
                                                                               \
    uint64_t hash = datastruct_hash(key_bytes, len, hm->seed);                 \
    size_t idx = hash % hm->width;                                             \
    for (name *e = hm->entries[idx]; e; e = e->next) {                         \
      if (e->hash == hash &&                                                   \
          __hashmap_key_compare(key_bytes, len, e->key, e->key_len)) {         \
        e->value = value;                                                      \
        name##_tkey_dispose_hook(key_bytes);                                   \
        return true;                                                           \
      }                                                                        \
    }                                                                          \
    ne = (name *)malloc(sizeof(name));                                         \
    if (!ne) {                                                                 \
      name##_tkey_dispose_hook(key_bytes);                                     \
      return false;                                                            \
    }                                                                          \
    ne->hash = hash;                                                           \
                                                                               \
    ne->key = malloc(len);                                                     \
    if (!ne->key) {                                                            \
      free(ne);                                                                \
      name##_tkey_dispose_hook(key_bytes);                                     \
      return false;                                                            \
    }                                                                          \
    memcpy(ne->key, key_bytes, len);                                           \
    ne->key_len = len;                                                         \
    ne->value = value;                                                         \
    ne->next = hm->entries[idx];                                               \
    hm->entries[idx] = ne;                                                     \
    hm->size++;                                                                \
    name##_tkey_dispose_hook(key_bytes);                                       \
    return true;                                                               \
  }                                                                            \
                                                                               \
  static vType *name##_hashmap_get(name##_hashmap *hm, kType *key) {           \
    size_t len = 0;                                                            \
    uint8_t *key_bytes = name##_key_bytes(key, &len);                          \
    uint64_t hash = datastruct_hash(key_bytes, len, hm->seed);                 \
    uint64_t idx = hash % hm->width;                                           \
                                                                               \
    for (name *e = hm->entries[idx]; e; e = e->next) {                         \
      if (e->hash == hash &&                                                   \
          __hashmap_key_compare(key_bytes, len, e->key, e->key_len)) {         \
        name##_tkey_dispose_hook(key_bytes);                                   \
        return e->value;                                                       \
      }                                                                        \
    }                                                                          \
    name##_tkey_dispose_hook(key_bytes);                                       \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  static bool name##_hashmap_delete(name##_hashmap *hm, kType *key) {          \
    size_t len = 0;                                                            \
    uint8_t *key_bytes = name##_key_bytes(key, &len);                          \
    uint64_t hash = datastruct_hash(key_bytes, len, hm->seed);                 \
    size_t idx = hash % hm->width;                                             \
    name *prev = NULL;                                                         \
    name *e = hm->entries[idx];                                                \
                                                                               \
    while (e) {                                                                \
      if (e->hash == hash &&                                                   \
          __hashmap_key_compare(key_bytes, len, e->key, e->key_len)) {         \
        if (prev) {                                                            \
          prev->next = e->next;                                                \
        } else {                                                               \
          hm->entries[idx] = e->next;                                          \
        }                                                                      \
        free(e->key);                                                          \
        free(e);                                                               \
        hm->size--;                                                            \
        name##_tkey_dispose_hook(key_bytes);                                   \
        return true;                                                           \
      }                                                                        \
      prev = e;                                                                \
      e = e->next;                                                             \
    }                                                                          \
    name##_tkey_dispose_hook(key_bytes);                                       \
    return false;                                                              \
  }                                                                            \
                                                                               \
  static void name##_hashmap_destroy(name##_hashmap *hm) {                     \
    if (!hm)                                                                   \
      return;                                                                  \
    for (size_t i = 0; i < hm->width; i++) {                                   \
      name *e = hm->entries[i];                                                \
      while (e) {                                                              \
        name *next = e->next;                                                  \
        free(e->key);                                                          \
        free(e);                                                               \
        e = next;                                                              \
      }                                                                        \
    }                                                                          \
    free(hm->entries);                                                         \
    free(hm);                                                                  \
  }

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → (void *) based implementation                 ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/**
 * Hashmap entry.
 *
 * The hashmap owns `key`: key data is copied on insertion and freed on removal.
 * The hashmap does NOT own `value`: it is stored as-is and never freed.
 *
 * Key identity is defined as byte-wise equality over (key, key_len).
 */
typedef struct entry {
  void *key;
  size_t key_len;
  uint64_t hash;
  void *value;
  struct entry *next;
} entry;

/**
 * Untyped chained hashmap.
 *
 * Keys are treated as opaque byte sequences.
 * Values are opaque pointers.
 *
 * Keys are immutable byte snapshots.
 * Any logical key semantics beyond raw bytes must be enforced by the caller.
 */
typedef struct {
  entry **buckets;
  uint64_t seed;
  size_t width;
  size_t size;
} hashmap;

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → methods (untyped implementation)              ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/**
 * INTERNAL_DO_NOT_USE
 * Check whether inserting one additional element would exceed
 * the load factor threshold.
 *
 * Uses a fixed 0.75 threshold.
 */
static bool __hashmap_check_load_factor(hashmap *hm) {
  return (hm->size + 1) * 4 >= hm->width * 3;
  /*return ((((double)hm->size + 1) / ((double)hm->width)) >
   * HASHMAP_THRESHOLD);*/
}

/**
 * INTERNAL_DO_NOT_USE
 */
static void __rehash_add_entry(entry *e, entry **nE, size_t nW) {
  uint64_t new_hash = e->hash % nW;
  e->next = nE[new_hash];
  nE[new_hash] = e;
}

/**
 * INTERNAL_DO_NOT_USE
 * Resize the hashmap by doubling its bucket array.
 *
 * Existing entries are rehashed using their stored hash values.
 * Keys and values are not reallocated.
 *
 * Returns false on allocation failure or size overflow.
 */
static bool __hashmap_resize_width(hashmap *hm) {
  if (hm->width > (SIZE_MAX) / 2 / sizeof(entry *)) {
    return false;
  }
  size_t new_width = hm->width * 2;
  entry **new_entries = (entry **)calloc(new_width, sizeof(entry *));

  if (!new_entries)
    return false;

  for (size_t i = 0; i < hm->width; i++) {
    entry *e = hm->buckets[i];
    while (e) {
      entry *next = e->next;
      __rehash_add_entry(e, new_entries, new_width);
      e = next;
    }
  }
  free(hm->buckets);
  hm->buckets = new_entries;
  hm->width = new_width;
  return true;
}

/**
 * INTERNAL_DO_NOT_USE
 */
static bool __hashmap_key_compare(void *key1, size_t len1, void *key2,
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
 * Create a new hashmap.
 *
 * `seed` is mixed into the hash function and should remain constant
 * for the lifetime of the hashmap.
 *
 * Keys are immutable byte snapshots.
 * Any logical key semantics beyond raw bytes must be enforced by the caller.
 *
 * Returns NULL on allocation failure.
 */
static hashmap *hashmap_new(uint64_t seed) {
  hashmap *hm = (hashmap *)malloc(sizeof(hashmap));
  if (!hm)
    return NULL;
  hm->width = HASHMAP_INITIAL_SIZE;
  hm->seed = seed;
  hm->size = 0;
  hm->buckets = (entry **)calloc(hm->width, sizeof(entry *));
  if (!hm->buckets) {
    free(hm);
    return NULL;
  }
  return hm;
}

/**
 * Insert or update a key/value pair.
 *
 * The key is treated as an opaque byte sequence of length `len`
 * and is copied into freshly allocated memory.
 *
 * If an identical key already exists (byte-wise equality),
 * its associated value pointer is replaced.
 *
 * The hashmap takes ownership of the copied key data.
 * The caller retains ownership of the value.
 *
 * Keys are immutable byte snapshots.
 * Any logical key semantics beyond raw bytes must be enforced by the caller.
 *
 * Returns false on allocation failure.
 */
static bool hashmap_put(hashmap *hm, void *key, size_t len, void *value) {
  entry *ne;
  if (__hashmap_check_load_factor(hm)) {
    if (!__hashmap_resize_width(hm)) {
      return false;
    }
  }

  uint64_t hash = datastruct_hash(key, len, hm->seed);
  uint64_t idx = hash % hm->width;
  for (entry *e = hm->buckets[idx]; e; e = e->next) {
    if (e->hash == hash &&
        __hashmap_key_compare(key, len, e->key, e->key_len)) {
      e->value = value;
      return true;
    }
  }
  ne = (entry *)malloc(sizeof(entry));
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
  ne->value = value;
  ne->next = hm->buckets[idx];
  hm->buckets[idx] = ne;
  hm->size++;
  return true;
}

/**
 * Retrieve the value associated with a key.
 *
 * Key comparison is performed using byte-wise equality over (key, len).
 *
 * Returns the stored value pointer, or NULL if the key is not present.
 */
static void *hashmap_get(hashmap *hm, void *key, size_t len) {
  uint64_t hash = datastruct_hash(key, len, hm->seed);
  size_t idx = hash % hm->width;
  for (entry *e = hm->buckets[idx]; e; e = e->next) {
    if (e->hash == hash &&
        __hashmap_key_compare(key, len, e->key, e->key_len)) {
      return e->value;
    }
  }
  return NULL;
}

/**
 * Remove a key/value pair from the hashmap.
 *
 * Frees the internal copy of the key.
 * Does NOT free the associated value.
 *
 * Returns true if an entry was removed, false if the key was not found.
 */
static bool hashmap_delete(hashmap *hm, void *key, size_t len) {
  uint64_t hash = datastruct_hash(key, len, hm->seed);
  size_t idx = hash % hm->width;
  entry *prev = NULL;
  entry *e = hm->buckets[idx];

  while (e) {
    if (e->hash == hash &&
        __hashmap_key_compare(key, len, e->key, e->key_len)) {
      if (prev) {
        prev->next = e->next;
      } else {
        hm->buckets[idx] = e->next;
      }
      free(e->key);
      free(e);
      hm->size--;
      return true;
    }
    prev = e;
    e = e->next;
  }
  return false;
}

static void *hashmap_find_all_predicate(hashmap *hm, void *fun(hashmap *)) {
  /*
    TODO
  for (size_t i = 0; i < hm->width; i++) {
    entry *e = hm->buckets[i];
    while (e) {
      entry *next = e->next;
      e = next;
    }
  }
  */
  return NULL;
}

/**
 * Destroy the hashmap.
 *
 * Frees all internal structures and key buffers.
 * Stored values are NOT freed.
 */
static void hashmap_destroy(hashmap *hm) {
  if (!hm)
    return;
  for (size_t i = 0; i < hm->width; i++) {
    entry *e = hm->buckets[i];
    while (e) {
      entry *next = e->next;
      free(e->key);
      free(e);
      e = next;
    }
  }
  free(hm->buckets);
  free(hm);
}

#endif
