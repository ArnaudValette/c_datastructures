#ifndef DATASTRUCTURES_CRYPTO_H
#define DATASTRUCTURES_CRYPTO_H
#include <stddef.h>
#include <stdint.h>

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

#endif
