#ifndef DATASTRUCTURES_SET_H
#define DATASTRUCTURES_SET_H
#include <stddef.h>
#include <stdint.h>

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § set → (void *) based implementation                     ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

typedef struct set_entry {
  void *key;
  uint64_t hash;
  uint8_t value;
  struct set_entry *next;
} set_entry;

typedef struct {
  set_entry *buckets;
  uint64_t seed;
  size_t width;
  size_t size;
} set;

#endif
