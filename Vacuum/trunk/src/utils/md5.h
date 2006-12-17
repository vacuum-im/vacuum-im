#ifndef _MD5_H
#define _MD5_H

typedef unsigned char uint8;
typedef unsigned int uint32;

#include "utilsexport.h"

struct md5_context
{
  uint32 total[2];
  uint32 state[4];
  uint8 buffer[64];
};

#ifdef __cplusplus
extern "C" {
#endif

  UTILS_EXPORT void md5_starts(struct md5_context *ctx);
  UTILS_EXPORT void md5_update(struct md5_context *ctx, uint8 *input, uint32 length);
  UTILS_EXPORT void md5_finish(struct md5_context *ctx, uint8 digest[16]);
  UTILS_EXPORT void md5_hash(uint8 digest[16], uint8 *input, uint32 length);
  UTILS_EXPORT void md5_hex(uint8 digest[16], char *hex);

#ifdef __cplusplus
}
#endif


#endif /* md5.h */

