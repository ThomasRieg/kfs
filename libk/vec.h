#include "libk.h"
#include "../vmalloc/vmalloc.h"

#ifndef VEC_H
#define VEC_H

#define VEC_DECL(type, ident)                                                  \
  struct Vec##ident {                                                          \
    type *data;                                                                \
    size_t length;                                                             \
    size_t capacity;                                                           \
  };                                                                           \
  void Vec##ident##_init(struct Vec##ident *vec);                              \
  bool Vec##ident##_reserve(struct Vec##ident *vec, size_t additional);        \
  bool Vec##ident##_append(struct Vec##ident *vec, const type *data,           \
                           size_t length);                                     \
  void Vec##ident##_erase_front(struct Vec##ident *vec, size_t length);        \
  void Vec##ident##_destruct(struct Vec##ident *vec);

#define VEC_IMPL(type, ident)                                                  \
  void Vec##ident##_init(struct Vec##ident *vec) {                             \
    vec->data = NULL;                                                          \
    vec->length = 0;                                                           \
    vec->capacity = 0;                                                         \
  }                                                                            \
  bool Vec##ident##_reserve(struct Vec##ident *vec, size_t additional) {       \
    if (vec->capacity - vec->length < additional) {                            \
      size_t new_capacity = (vec->capacity + additional) * 2;                  \
      type *new_data = vrealloc(vec->data, new_capacity * sizeof(type));        \
      if (!new_data) {                                                         \
        return false;                                                          \
      }                                                                        \
      vec->data = new_data;                                                    \
      vec->capacity = new_capacity;                                            \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
  bool Vec##ident##_append(struct Vec##ident *vec, const type *data,           \
                           size_t length) {                                    \
    if (Vec##ident##_reserve(vec, length)) {                                   \
        memcpy(vec->data + vec->length, data, length * sizeof(type));          \
        vec->length += length;                                                 \
        return true;                                                           \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
  void Vec##ident##_erase_front(struct Vec##ident *vec, size_t length) {       \
    if (length > vec->length)                                                  \
      length = vec->length;                                                    \
    memmove(vec->data, vec->data + length,                                     \
            (vec->length - length) * sizeof(type));                            \
    vec->length -= length;                                                     \
  }                                                                            \
                                                                               \
  void Vec##ident##_destruct(struct Vec##ident *vec) { vfree(vec->data); }

VEC_DECL(unsigned char, U8)

#endif
