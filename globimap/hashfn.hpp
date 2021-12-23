

#ifndef HASHFN_HPP
#define HASHFN_HPP

#define GLOBIMAP_USE_MURMUR

//#define GLOBIMAP_USE_MURMUR_PREFIX
//#define GLOBIMAP_USE_DJB64

#include <string.h>

#ifdef GLOBIMAP_USE_MURMUR
#include "murmur.hpp"

#ifdef GLOBIMAP_USE_MURMUR_PREFIX
inline void hash(const uint64_t *data, size_t len, uint64_t *v1, uint64_t *v2,
                 const char *prefix = "") {
  uint64_t hash[2];
  char buffer[1024];
  snprintf(buffer, 1024, "%s-%lu-%lu", prefix, data[0], data[1]);
  murmur::MurmurHash3_x86_128(data, strlen(buffer), *v1,
                              (void *)hash); // len * sizeof(uint64_t)
  *v1 = hash[0];
  *v2 = hash[1];
}
#else
inline void hash(const uint64_t *data, const size_t len, uint64_t *v1,
                 uint64_t *v2) {
  uint64_t hash[2];
  /*    char buffer[1024];
      snprintf(buffer, 1024,"%s-%lu-%lu", prefix,data[0],data[1]);*/
  murmur::MurmurHash3_x64_128(data, 16, *v1,
                              (void *)hash); // len*sizeof(uint64_t)
  *v1 = hash[0];
  *v2 = hash[1];
}
#endif
#endif

#ifdef GLOBIMAP_USE_DJB64

// This should not be used, it would need careful evaluation on many layers.
// Stays here for completeness!!!

inline void hash(uint32_t *data, size_t len, uint32_t *v1, uint32_t *v2) {
  const char *prefix1 = "p1";
  const char *prefix2 = "x8";
  char *str = reinterpret_cast<char *>(data);
  uint32_t hash1 = 5381;
  uint32_t hash2 = 5381;
  for (size_t i = 0; i < 2; i++) {
    hash1 = ((hash1 << 5) + hash1) + prefix1[i]; /* hash * 33 + c */
    hash2 = ((hash2 << 5) + hash2) + prefix2[i]; /* hash * 33 + c */
  }

  for (size_t i = 0; i < len * sizeof(uint32_t); i++) {
    hash1 = ((hash1 << 5) + hash1) + *str; /* hash * 33 + c */
    hash2 = ((hash2 << 5) + hash2) + *str; /* hash * 33 + c */
    str++;
  }
  //    *v1 = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000LL) >> 32);
  //    *v2 = static_cast<uint32_t>(hash & 0xFFFFFFFFLL) ;
  *v1 = hash1;
  *v2 = hash2;
}
#endif

#ifdef GLOBIMAP_USE_LOOKUP3
// This should not be used, it would need careful evaluation on many layers. Not
// published, google for lookup3.h ;-)
#include "lookup3.hpp"

inline void hash(uint32_t *data, size_t len, uint32_t *v1, uint32_t *v2) {
  hashword2(data, len, v1, v2);
}
#endif

#endif