#ifndef MINION_HASH_MD5_H
#define MINION_HASH_MD5_H

typedef struct md5_ctx_t {
 uint8_t wbuffer[64];
 void (*process_block)(struct md5_ctx_t*) ;
 uint64_t total64;
 uint32_t hash[8];
} md5_ctx_t;

void md5_begin(md5_ctx_t *ctx) ;
void md5_hash(md5_ctx_t *ctx, const void *buffer, size_t len);
void md5_end(md5_ctx_t *ctx);
unsigned char *hash_bin_to_hex(md5_ctx_t *ctx);
uint8_t *hash_buf(const void *in_buf, int count);

#endif
