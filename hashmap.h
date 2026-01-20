#ifndef HASHING_FUNCS_H
#define HASHING_FUNCS_H
#include <cstdint>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HASHMAP_INITIAL_SIZE (128)

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → macro based implementation                    ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

#define HASHMAP(kType, vType, name)                                            \
  typedef struct name {                                                        \
    kType key;                                                                 \
    size_t key_len;                                                            \
    uint64_t hash;                                                             \
    vType value;                                                               \
    struct name *next;                                                         \
  } name;                                                                      \
  typedef struct {                                                             \
    name **entries;                                                            \
    uint64_t seed;                                                             \
    size_t width;                                                              \
    size_t size;                                                               \
  } name##_map;

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → (void *) based implementation                 ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/* *(hashmap+hash("yourKey")) */
typedef struct entry {
  void *key;
  size_t key_len;
  uint64_t hash;
  void *value;
  struct entry *next;
} entry;

typedef struct {
  entry **buckets;
  uint64_t seed;
  size_t width;
  size_t size;
} hashmap;

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → hashing functions                             ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/* xxHash 64-bit like */
static const uint64_t PRIME64_1 = 0x9E3779B185EBCA87ull;
static const uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4Full;
static const uint64_t PRIME64_3 = 0x165667B19E3779F9ull;
static const uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ull;
static const uint64_t PRIME64_5 = 0x27D4EB2F165667C5ull;

static inline uint64_t datastruct_rotl_64(uint64_t n, uint8_t d) {
  return (n << d) | (n >> (64 - d));
}

static inline uint64_t datastruct_round(uint64_t acc, uint64_t lane) {
  acc = acc + (lane * PRIME64_2);
  acc = datastruct_rotl_64(acc, 31);
  return acc * PRIME64_1;
}

static inline uint64_t datastruct_read_64(uint8_t *d, size_t cursor) {
  return ((uint64_t)d[cursor] << 56) | ((uint64_t)d[cursor + 1] << 48) |
         ((uint64_t)d[cursor + 2] << 40) | ((uint64_t)d[cursor + 3] << 32) |
         ((uint64_t)d[cursor + 4] << 24) | ((uint64_t)d[cursor + 5] << 16) |
         ((uint64_t)d[cursor + 6] << 8) | (d[cursor + 7]);
}

static inline uint32_t datastruct_read_32(uint8_t *d, size_t cursor) {
  return ((uint32_t)d[cursor + 0] << 24) | ((uint32_t)d[cursor + 1] << 16) |
         ((uint32_t)d[cursor + 2] << 8) | (d[cursor + 3]);
}

static inline uint64_t datastruct_merge(uint64_t dest, uint64_t src) {
  dest = dest ^ datastruct_round(dest, src);
  dest = dest * PRIME64_1;
  return dest + PRIME64_4;
}

static uint64_t datastruct_hash(void *data, size_t len, uint64_t seed) {
  uint64_t acc;
  uint64_t lanes[4];
  size_t cursor = 0;
  uint8_t *d = (uint8_t *)data;

  if (len < 32) {
    acc = seed + PRIME64_5;
  } else {
    /* 2. Stripes processing */
    uint64_t acc1 = seed + PRIME64_1 + PRIME64_2;
    uint64_t acc2 = seed + PRIME64_2;
    uint64_t acc3 = seed + 0;
    uint64_t acc4 = seed - PRIME64_1;
    while (len >= 32) {
      /* Each lane reads 64 bits */
      for (int i = 0; i < 4; i++) {
        lanes[i] = datastruct_read_64(d, cursor);
        cursor += 8;
      }
      acc1 = datastruct_round(acc1, lanes[0]);
      acc2 = datastruct_round(acc2, lanes[1]);
      acc3 = datastruct_round(acc3, lanes[2]);
      acc4 = datastruct_round(acc4, lanes[3]);
    }
    /* 3. Accumulators merging */
    acc = datastruct_rotl_64(acc1, 1) + datastruct_rotl_64(acc2, 7) +
          datastruct_rotl_64(acc3, 12) + datastruct_rotl_64(acc4, 18);
    acc = datastruct_merge(acc, acc1);
    acc = datastruct_merge(acc, acc2);
    acc = datastruct_merge(acc, acc3);
    acc = datastruct_merge(acc, acc4);
  }
  /* Meet-up */
  acc = acc + len;
  while (len >= 8) {
    lanes[0] = datastruct_read_64(d, cursor);

    acc = acc ^ datastruct_round(acc, lanes[0]);
    acc = datastruct_rotl_64(acc, 27) * PRIME64_1;
    acc = acc + PRIME64_4;
    cursor += 8;
    len -= 8;
  }
  if (len >= 4) {
    lanes[0] = datastruct_read_32(d, cursor);
    acc = acc ^ (lanes[0] * PRIME64_1);
    acc = datastruct_rotl_64(acc, 23) * PRIME64_2;
    acc = acc + PRIME64_3;
    cursor += 4;
    len -= 4;
  }
  while (len >= 1) {
    lanes[0] = d[cursor++];
    acc = acc ^ (lanes[0] * PRIME64_5);
    acc = datastruct_rotl_64(acc, 11) * PRIME64_1;
    len--;
  }
  acc = acc ^ (acc >> 33);
  acc = acc * PRIME64_2;
  acc = acc ^ (acc >> 29);
  acc = acc * PRIME64_3;
  acc = acc ^ (acc >> 32);
  return acc;
}

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → methods (untyped implementation)              ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

static bool __hashmap_check_load_factor(hashmap *hm) {
  return (hm->size + 1) * 4 >= hm->width * 3;
  /*return ((((double)hm->size + 1) / ((double)hm->width)) >
   * HASHMAP_THRESHOLD);*/
}

static void __rehash_add_entry(entry *e, entry **nE, size_t nW) {
  uint64_t new_hash = e->hash % nW;
  e->next = nE[new_hash];
  nE[new_hash] = e;
}

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

static void * hashmap_find_all_predicate(hashmap *hm, void *fun(hashmap *)){
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
