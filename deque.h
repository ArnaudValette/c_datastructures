#ifndef DEQUE_H
#define DEQUE_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define DEQUE_INITIAL_SIZE 128

#ifndef DATASTRUCT_USE_VOID_BASED_DEQUE

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § array deque → macro implementation                      ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

#define DEQUE_TYPEDECL(type, name)                                             \
  typedef struct {                                                             \
    type **data;                                                               \
    size_t start;                                                              \
    size_t end;                                                                \
    size_t capacity;                                                           \
  } name##_deque;

#define DEQUE_IMPL(type, name)                                                 \
  static name##_deque *name##_deque_create() {                                 \
    name##_deque *a = malloc(sizeof(name##_deque));                            \
    if (!a)                                                                    \
      return NULL;                                                             \
    a->capacity = DEQUE_INITIAL_SIZE;                                          \
    a->data = malloc(a->capacity * sizeof(type *));                            \
    if (!a->data) {                                                            \
      free(a);                                                                 \
      return NULL;                                                             \
    }                                                                          \
    a->start = a->end = 0;                                                     \
    return a;                                                                  \
  }                                                                            \
  static inline int name##_deque_is_empty(name##_deque *a) {                   \
    return a->end == a->start;                                                 \
  }                                                                            \
                                                                               \
  static size_t name##_deque_len(name##_deque *a) {                            \
    return (a->end + a->capacity - a->start) % a->capacity;                    \
  }                                                                            \
                                                                               \
  static type *name##_deque_get(name##_deque *a, size_t i) {                   \
    if (i >= name##_deque_len(a)) {                                            \
      return NULL;                                                             \
    }                                                                          \
    return a->data[(a->start + i) % a->capacity];                              \
  }                                                                            \
                                                                               \
  static void name##_deque_destroy(name##_deque *a) {                          \
    free(a->data);                                                             \
    a->data = NULL;                                                            \
    free(a);                                                                   \
    a = NULL;                                                                  \
  }                                                                            \
                                                                               \
  static inline int name##__deque_full(name##_deque *a) {                      \
    return (a->end + 1) % a->capacity == a->start;                             \
  }                                                                            \
                                                                               \
  static inline int name##__deque_resize(name##_deque *a) {                    \
    if (a->capacity > (SIZE_MAX) / 2 / sizeof(type *)) {                       \
      return 1;                                                                \
    }                                                                          \
    size_t len = name##_deque_len(a);                                          \
    size_t new_cap = a->capacity * 2;                                          \
                                                                               \
    type **data = malloc(new_cap * sizeof(type *));                            \
    if (!data)                                                                 \
      return 1;                                                                \
                                                                               \
    for (size_t i = 0; i < len; i++) {                                         \
      data[i] = a->data[(a->start + i) % a->capacity];                         \
    }                                                                          \
    free(a->data);                                                             \
    a->data = data;                                                            \
    a->capacity = new_cap;                                                     \
    a->start = 0;                                                              \
    a->end = len;                                                              \
    return 0;                                                                  \
  }                                                                            \
                                                                               \
  /** @brief append to the end of the array */                                 \
  static int name##_deque_append(name##_deque *a, type *data) {                \
    if (name##__deque_full(a)) {                                               \
      if (name##__deque_resize(a)) {                                           \
        return 1;                                                              \
      }                                                                        \
    }                                                                          \
    a->data[a->end] = data;                                                    \
    a->end = (a->end + 1) % a->capacity;                                       \
    return 0;                                                                  \
  }                                                                            \
                                                                               \
  /** @return the first element of the array (removed). */                     \
  static type *name##_deque_pop(name##_deque *a) {                             \
    if (name##_deque_is_empty(a)) {                                            \
      return NULL;                                                             \
    }                                                                          \
    type *v = a->data[a->start];                                               \
    a->start = (a->start + 1) % a->capacity;                                   \
    return v;                                                                  \
  }                                                                            \
                                                                               \
  /** @return the last element of the array (removed). */                      \
  static type *name##_deque_pop_last(name##_deque *a) {                        \
    if (name##_deque_is_empty(a)) {                                            \
      return NULL;                                                             \
    }                                                                          \
    a->end = (a->end + a->capacity - 1) % a->capacity;                         \
    return a->data[a->end];                                                    \
  }                                                                            \
                                                                               \
  /** @brief push to the start of the array */                                 \
  static int name##_deque_push(name##_deque *a, type *data) {                  \
    if (name##__deque_full(a)) {                                               \
      if (name##__deque_resize(a)) {                                           \
        return 1;                                                              \
      }                                                                        \
    }                                                                          \
    a->start = (a->start + a->capacity - 1) % a->capacity;                     \
    a->data[a->start] = data;                                                  \
    return 0;                                                                  \
  }

#else

/*
╰┭━╾┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅┅╼━┮╮
╭╯ datastructures § array deque → (void *) implementation                   ╭╯╿
╙╼━╾┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄━━╪*/

typedef struct deque {
  void **data;
  size_t start;
  size_t end;
  size_t capacity;
} deque;

static deque *deque_create() {
  deque *a = malloc(sizeof(deque));
  if (!a)
    return NULL;
  a->capacity = DEQUE_INITIAL_SIZE;
  a->data = malloc(a->capacity * sizeof(void *));
  if (!a->data) {
    free(a);
    return NULL;
  }
  a->start = a->end = 0;
  return a;
}

static inline int deque_is_empty(deque *a) { return a->end == a->start; }

static size_t deque_len(deque *a) {
  return (a->end + a->capacity - a->start) % a->capacity;
}

static void *deque_get(deque *a, size_t i) {
  if (i >= deque_len(a)) {
    return NULL;
  }
  return a->data[(a->start + i) % a->capacity];
}

static void deque_destroy(deque *a) {
  free(a->data);
  a->data = NULL;
  free(a);
  a = NULL;
}

static inline int __deque_full(deque *a) {
  return (a->end + 1) % a->capacity == a->start;
}

static inline int __deque_resize(deque *a) {
  if (a->capacity > (SIZE_MAX) / 2 / sizeof(void *)) {
    return 1;
  }
  size_t len = deque_len(a);
  size_t new_cap = a->capacity * 2;

  void **data = malloc(new_cap * sizeof(void *));
  if (!data)
    return 1;

  for (size_t i = 0; i < len; i++) {
    data[i] = a->data[(a->start + i) % a->capacity];
  }
  free(a->data);
  a->data = data;
  a->capacity = new_cap;
  a->start = 0;
  a->end = len;
  return 0;
}

/** @brief append to the end of the array */
static int deque_append(deque *a, void *data) {
  if (__deque_full(a)) {
    if (__deque_resize(a)) {
      return 1;
    }
  }
  a->data[a->end] = data;
  a->end = (a->end + 1) % a->capacity;
  return 0;
}

/** @return the first element of the array (removed). */
static void *deque_pop(deque *a) {
  if (deque_is_empty(a)) {
    return NULL;
  }
  void *v = a->data[a->start];
  a->start = (a->start + 1) % a->capacity;
  return v;
}

/** @return the last element of the array (removed). */
static void *deque_pop_last(deque *a) {
  if (deque_is_empty(a)) {
    return NULL;
  }
  a->end = (a->end + a->capacity - 1) % a->capacity;
  return a->data[a->end];
}

/** @brief push to the start of the array */
static int deque_push(deque *a, void *data) {
  if (__deque_full(a)) {
    if (__deque_resize(a)) {
      return 1;
    }
  }
  a->start = (a->start + a->capacity - 1) % a->capacity;
  a->data[a->start] = data;
  return 0;
}

#endif

#endif
