/*
 *	BIRD -- SHA256 and SHA224 Hash Functions
 *
 *	(c) 2015 CZ.NIC z.s.p.o.
 *
 *	Based on the code from libgcrypt-1.6.0, which is
 *	(c) 2003, 2006, 2008, 2009 Free Software Foundation, Inc.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "lib/sha256.h"
#include "lib/unaligned.h"

void
sha256_init(sha256_context *ctx)
{
  ctx->h0 = 0x6a09e667;
  ctx->h1 = 0xbb67ae85;
  ctx->h2 = 0x3c6ef372;
  ctx->h3 = 0xa54ff53a;
  ctx->h4 = 0x510e527f;
  ctx->h5 = 0x9b05688c;
  ctx->h6 = 0x1f83d9ab;
  ctx->h7 = 0x5be0cd19;

  ctx->nblocks = 0;
  ctx->nblocks_high = 0;
  ctx->count = 0;
}

void
sha224_init(sha224_context *ctx)
{
  ctx->h0 = 0xc1059ed8;
  ctx->h1 = 0x367cd507;
  ctx->h2 = 0x3070dd17;
  ctx->h3 = 0xf70e5939;
  ctx->h4 = 0xffc00b31;
  ctx->h5 = 0x68581511;
  ctx->h6 = 0x64f98fa7;
  ctx->h7 = 0xbefa4fa4;

  ctx->nblocks = 0;
  ctx->nblocks_high = 0;
  ctx->count = 0;
}

/* (4.2) same as SHA-1's F1.  */
static inline u32
f1(u32 x, u32 y, u32 z)
{
  return (z ^ (x & (y ^ z)));
}

/* (4.3) same as SHA-1's F3 */
static inline u32
f3(u32 x, u32 y, u32 z)
{
  return ((x & y) | (z & (x|y)));
}

/* Bitwise rotation of an unsigned int to the right */
static inline u32 ror(u32 x, int n)
{
  return ( (x >> (n&(32-1))) | (x << ((32-n)&(32-1))) );
}

/* (4.4) */
static inline u32
sum0(u32 x)
{
  return (ror(x, 2) ^ ror(x, 13) ^ ror(x, 22));
}

/* (4.5) */
static inline u32
sum1(u32 x)
{
  return (ror(x, 6) ^ ror(x, 11) ^ ror(x, 25));
}

/*
  Transform the message X which consists of 16 32-bit-words. See FIPS
  180-2 for details.  */
#define S0(x) (ror((x),  7) ^ ror((x), 18) ^ ((x) >>  3))	/* (4.6) */
#define S1(x) (ror((x), 17) ^ ror((x), 19) ^ ((x) >> 10))	/* (4.7) */
#define R(a,b,c,d,e,f,g,h,k,w)					\
    do								\
    {								\
      t1 = (h) + sum1((e)) + f1((e),(f),(g)) + (k) + (w);	\
      t2 = sum0((a)) + f3((a),(b),(c));				\
      h = g;							\
      g = f;							\
      f = e;							\
      e = d + t1;						\
      d = c;							\
      c = b;							\
      b = a;							\
      a = t1 + t2;						\
    } while (0)

/*
    The SHA-256 core: Transform the message X which consists of 16
    32-bit-words. See FIPS 180-2 for details.
 */
static unsigned int
transform(sha256_context *ctx, const unsigned char *data)
{
  static const u32 K[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
      0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
      0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
      0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
      0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
      0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
  };

  u32 a,b,c,d,e,f,g,h,t1,t2;
  u32 w[64];
  int i;

  a = ctx->h0;
  b = ctx->h1;
  c = ctx->h2;
  d = ctx->h3;
  e = ctx->h4;
  f = ctx->h5;
  g = ctx->h6;
  h = ctx->h7;

  for (i = 0; i < 16; i++)
    w[i] = get_u32(data + i * 4);
  for (; i < 64; i++)
    w[i] = S1(w[i-2]) + w[i-7] + S0(w[i-15]) + w[i-16];

  for (i = 0; i < 64;)
  {
    t1 = h + sum1(e) + f1(e, f, g) + K[i] + w[i];
    t2 = sum0 (a) + f3(a, b, c);
    d += t1;
    h  = t1 + t2;

    t1 = g + sum1(d) + f1(d, e, f) + K[i+1] + w[i+1];
    t2 = sum0 (h) + f3(h, a, b);
    c += t1;
    g  = t1 + t2;

    t1 = f + sum1(c) + f1(c, d, e) + K[i+2] + w[i+2];
    t2 = sum0 (g) + f3(g, h, a);
    b += t1;
    f  = t1 + t2;

    t1 = e + sum1(b) + f1(b, c, d) + K[i+3] + w[i+3];
    t2 = sum0 (f) + f3(f, g, h);
    a += t1;
    e  = t1 + t2;

    t1 = d + sum1(a) + f1(a, b, c) + K[i+4] + w[i+4];
    t2 = sum0 (e) + f3(e, f, g);
    h += t1;
    d  = t1 + t2;

    t1 = c + sum1(h) + f1(h, a, b) + K[i+5] + w[i+5];
    t2 = sum0 (d) + f3(d, e, f);
    g += t1;
    c  = t1 + t2;

    t1 = b + sum1(g) + f1(g, h, a) + K[i+6] + w[i+6];
    t2 = sum0 (c) + f3(c, d, e);
    f += t1;
    b  = t1 + t2;

    t1 = a + sum1(f) + f1(f, g, h) + K[i+7] + w[i+7];
    t2 = sum0 (b) + f3(b, c, d);
    e += t1;
    a  = t1 + t2;

    i += 8;
  }

  ctx->h0 += a;
  ctx->h1 += b;
  ctx->h2 += c;
  ctx->h3 += d;
  ctx->h4 += e;
  ctx->h5 += f;
  ctx->h6 += g;
  ctx->h7 += h;

  return /*burn_stack*/ 74*4+32;
}
#undef S0
#undef S1
#undef R

static unsigned int
sha256_transform(void *ctx, const unsigned char *data, size_t nblks)
{
  sha256_context *hd = ctx;
  unsigned int burn;

  do
  {
    burn = transform(hd, data);
    data += 64;
  }
  while (--nblks);

  return burn;
}

/* Common function to write a chunk of data to the transform function
   of a hash algorithm.  Note that the use of the term "block" does
   not imply a fixed size block.  Note that we explicitly allow to use
   this function after the context has been finalized; the result does
   not have any meaning but writing after finalize is sometimes
   helpful to mitigate timing attacks. */
void
sha256_update(sha256_context *ctx, const byte *in_buf, size_t in_len)
{
  unsigned int stack_burn = 0;
  const unsigned int blocksize = 64;
  size_t inblocks;

  if (sizeof(ctx->buf) < blocksize)
    debug("BUG: in file %s at line %d", __FILE__ , __LINE__);

  if (ctx->count == blocksize)  /* Flush the buffer. */
  {
    stack_burn = sha256_transform(ctx, ctx->buf, 1);
    stack_burn = 0;
    ctx->count = 0;
    if (!++ctx->nblocks)
      ctx->nblocks_high++;
  }
  if (!in_buf)
    return;

  if (ctx->count)
  {
    for (; in_len && ctx->count < blocksize; in_len--)
      ctx->buf[ctx->count++] = *in_buf++;
    sha256_update(ctx, NULL, 0);
    if (!in_len)
      return;
  }

  if (in_len >= blocksize)
  {
    inblocks = in_len / blocksize;
    stack_burn = sha256_transform(ctx, in_buf, inblocks);
    ctx->count = 0;
    ctx->nblocks_high += (ctx->nblocks + inblocks < inblocks);
    ctx->nblocks += inblocks;
    in_len -= inblocks * blocksize;
    in_buf += inblocks * blocksize;
  }
  for (; in_len && ctx->count < blocksize; in_len--)
    ctx->buf[ctx->count++] = *in_buf++;
}

/*
   The routine finally terminates the computation and returns the
   digest.  The handle is prepared for a new cycle, but adding bytes
   to the handle will the destroy the returned buffer.  Returns: 32
   bytes with the message the digest.  */
byte*
sha256_final(sha256_context *ctx)
{
  u32 t, th, msb, lsb;
  byte *p;
  unsigned int burn;

  sha256_update(ctx, NULL, 0); /* flush */;

  t = ctx->nblocks;
  if (sizeof t == sizeof ctx->nblocks)
    th = ctx->nblocks_high;
  else
    th = ctx->nblocks >> 32;

  /* multiply by 64 to make a byte count */
  lsb = t << 6;
  msb = (th << 6) | (t >> 26);
  /* add the count */
  t = lsb;
  if ((lsb += ctx->count) < t)
    msb++;
  /* multiply by 8 to make a bit count */
  t = lsb;
  lsb <<= 3;
  msb <<= 3;
  msb |= t >> 29;

  if (ctx->count < 56)
  { /* enough room */
    ctx->buf[ctx->count++] = 0x80; /* pad */
    while (ctx->count < 56)
      ctx->buf[ctx->count++] = 0;  /* pad */
  }
  else
  { /* need one extra block */
    ctx->buf[ctx->count++] = 0x80; /* pad character */
    while (ctx->count < 64)
      ctx->buf[ctx->count++] = 0;
    sha256_update(ctx, NULL, 0);  /* flush */;
    memset (ctx->buf, 0, 56 ); /* fill next block with zeroes */
  }
  /* append the 64 bit count */
  put_u32(ctx->buf + 56, msb);
  put_u32(ctx->buf + 60, lsb);
  burn = sha256_transform(ctx, ctx->buf, 1);

  p = ctx->buf;

#define X(a) do { put_u32(p, ctx->h##a); p += 4; } while(0)
  X(0);
  X(1);
  X(2);
  X(3);
  X(4);
  X(5);
  X(6);
  X(7);
#undef X

  return ctx->buf;
}


/**
 * 	HMAC
 */

/* Create a new context.  On error NULL is returned and errno is set
   appropriately.  If KEY is given the function computes HMAC using
   this key; with KEY given as NULL, a plain SHA-256 digest is
   computed.  */
void
sha256_hmac_init(sha256_hmac_context *ctx, const void *key, size_t keylen)
{
  sha256_init(&ctx->ctx);

  ctx->finalized = 0;
  ctx->use_hmac = 0;

  if (key)
  {
    int i;
    unsigned char ipad[64];

    memset(ipad, 0, 64);
    memset(ctx->opad, 0, 64);
    if (keylen <= 64)
    {
      memcpy(ipad, key, keylen);
      memcpy(ctx->opad, key, keylen);
    }
    else
    {
      sha256_hmac_context tmp_ctx;

      sha256_hmac_init(&tmp_ctx, NULL, 0);
      sha256_hmac_update(&tmp_ctx, key, keylen);
      sha256_final(&tmp_ctx.ctx);
      memcpy(ipad, tmp_ctx.ctx.buf, 32);
      memcpy(ctx->opad, tmp_ctx.ctx.buf, 32);
    }
    for(i=0; i < 64; i++)
    {
      ipad[i] ^= 0x36;
      ctx->opad[i] ^= 0x5c;
    }
    ctx->use_hmac = 1;
    sha256_hmac_update(ctx, ipad, 64);
  }
}

void sha224_hmac_init(sha224_hmac_context *ctx, const void *key, size_t keylen)
{
  sha224_init(&ctx->ctx);

  ctx->finalized = 0;
  ctx->use_hmac = 0;

  if (key)
  {
    int i;
    unsigned char ipad[64];

    memset(ipad, 0, 64);
    memset(ctx->opad, 0, 64);
    if (keylen <= 64)
    {
      memcpy(ipad, key, keylen);
      memcpy(ctx->opad, key, keylen);
    }
    else
    {
      sha224_hmac_context tmp_ctx;

      sha224_hmac_init(&tmp_ctx, NULL, 0);
      sha224_hmac_update(&tmp_ctx, key, keylen);
      sha224_final(&tmp_ctx.ctx);
      memcpy(ipad, tmp_ctx.ctx.buf, 32);
      memcpy(ctx->opad, tmp_ctx.ctx.buf, 32);
    }
    for(i=0; i < 64; i++)
    {
      ipad[i] ^= 0x36;
      ctx->opad[i] ^= 0x5c;
    }
    ctx->use_hmac = 1;
    sha224_hmac_update(ctx, ipad, 64);
  }
}

/* Update the message digest with the contents of BUFFER containing
   LENGTH bytes.  */
void
sha256_hmac_update(sha256_hmac_context *ctx, const void *buffer, size_t length)
{
  sha256_update(&ctx->ctx, buffer, length);
}

/* Finalize an operation and return the digest.  If R_DLEN is not NULL
   the length of the digest will be stored at that address.  The
   returned value is valid as long as the context exists.  On error
   NULL is returned. */
const byte *
sha256_hmac_final(sha256_hmac_context *ctx)
{
  sha256_final(&ctx->ctx);
  if (ctx->use_hmac)
  {
    sha256_hmac_context tmp_ctx;

    sha256_hmac_init(&tmp_ctx, NULL, 0);
    sha256_hmac_update(&tmp_ctx, ctx->opad, 64);
    sha256_hmac_update(&tmp_ctx, ctx->ctx.buf, 32);
    sha256_final(&tmp_ctx.ctx);
    memcpy(ctx->ctx.buf, tmp_ctx.ctx.buf, 32);
  }
  return ctx->ctx.buf;
}
