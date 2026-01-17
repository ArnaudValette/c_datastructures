#ifndef HASHING_FUNCS_H
#define HASHING_FUNCS_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → macro based implementation                    ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

#define HASHMAP(kType, vType, name)\
typedef struct name{ \
  kType key; \
  vType value; \
  struct name *next; \
} name; \
typedef struct{ \
  name **entries; \
  uint64_t seed; \
} \
name##_map;

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § hashmap → (void *) based implementation                 ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

/* *(hashmap+hash("yourKey")) */
typedef struct entry {
  char *key;
  void *value;
  struct entry *next;
} entry;

typedef struct {
  entry **buckets;
  uint64_t seed;
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

static inline uint64_t datastruct_rotl_64(uint64_t n, uint8_t d){
  return (n << d) | (n >> (64-d));
}
static inline uint64_t datastruct_round(uint64_t *acc, uint64_t *lane){
  *acc= *acc + ((*lane) * PRIME64_2);
  *acc = datastruct_rotl_64(*acc, 31);
  return *acc * PRIME64_1;
}

static uint64_t datastruct_simple_hash(void *data, size_t len, uint64_t seed){
  uint8_t *d =  (uint8_t *)data;
  uint64_t acc = seed + PRIME64_5;
  uint64_t lane;
  acc = acc + len;
  size_t cursor=0;
  while(len >= 8){
    lane = (d[cursor] << 7) | (d[cursor+1] << 6) | (d[cursor+2] << 5) | (d[cursor+3] << 4) | (d[cursor+4] << 3) | (d[cursor+5] << 2) | (d[cursor+6] << 1) | (d[cursor+7]);
    acc = acc ^ datastruct_round(&acc, &lane); 
    acc = datastruct_rotl_64(acc, 27) * PRIME64_1;
    acc = acc + PRIME64_4;
    cursor+=8; len -=8;
  }
  if(len >= 4){
    lane =  (d[cursor] << 3) | (d[cursor+1] << 2) | (d[cursor+2] << 1) | (d[cursor+3]);
    acc = acc ^ (lane * PRIME64_1);
    acc = datastruct_rotl_64(acc, 23)  * PRIME64_2;
    acc = acc + PRIME64_3;
    cursor+= 4; len-=4;
  }
  while(len >=1){
    lane = d[cursor++];
    acc = acc ^ (lane * PRIME64_5);
    acc = datastruct_rotl_64(acc, 11) * PRIME64_1;
    len--;
  }
  acc = acc & (acc >> 33);
  acc = acc * PRIME64_2;
  acc = acc ^ (acc >> 29);
  acc = acc * PRIME64_3;
  acc = acc ^ (acc >> 32);
  return acc;
}

static uint64_t datastruct_hash(void *data, size_t len, uint64_t seed){
  if(len < 32){
    return datastruct_simple_hash(data,len,seed);
  }
  /* TODO < 32 _BYTES_ input */
  uint64_t acc1 = seed + PRIME64_1 + PRIME64_2;
  uint64_t acc2 = seed + PRIME64_2;
  uint64_t acc3 = seed + 0;
  uint64_t acc4 = seed - PRIME64_1;
    
  /* Not implemented */
  return 0;
}

#endif
