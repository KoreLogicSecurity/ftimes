/*
 ***********************************************************************
 *
 * $Id: md5.c,v 1.1.1.1 2002/01/18 03:17:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "md5.h"

int 
md5_file(FILE *fp, unsigned char *message_digest)
{
  int                 i,
                      nread;
  unsigned char       buf[MD5_READ_BUFSIZE]; 
  struct hash_block   x;
 
  /*
   * Let's just make sure that we are at the beginning of the file.
   */
  rewind(fp);

  md5_begin(&x);
  while ((nread = fread(buf, 1, MD5_READ_BUFSIZE, fp)) == MD5_READ_BUFSIZE)    
  { 
    md5_middle(&x, buf, nread);
  }
  if (nread < MD5_READ_BUFSIZE && ferror(fp))
  {
    for (i = 0; i < MD5_HASH_LENGTH; i++)
    {
      message_digest[i] = 0;
    }
    return -1;  
  }
  else
  {
    md5_middle(&x, buf, nread);
  }
  md5_end(&x, message_digest);
  return 0;
}

void 
md5_string(unsigned char *text, int text_len, unsigned char *message_digest)
{
  struct hash_block   x;

  md5_begin(&x);
  while (text_len > MD5_BYTES_PER_BLOCK)
  {
    md5_middle(&x, text, MD5_BYTES_PER_BLOCK);
    text += MD5_BYTES_PER_BLOCK;
    text_len -= MD5_BYTES_PER_BLOCK;
  }
  md5_middle(&x, text, text_len);
  md5_end(&x, message_digest);
}

void 
md5_begin(struct hash_block *x)
{
  x->A = 0x67452301;
  x->B = 0xefcdab89;
  x->C = 0x98badcfe;
  x->D = 0x10325476;
  x->message_length[0] = x->message_length[1] = 0;
  x->amount_left_over = 0;
}

void 
md5_middle(struct hash_block *x, unsigned char *text, unsigned long len)
{
  unsigned char      *tmp_ptr;
  int                 n;

  /* update message length */
  if ((x->message_length[0] += len) < len)
    ++x->message_length[1];

  n = x->amount_left_over;

  /* if left over + new is still less than BLOCK SIZE, just store new */
  if ((n + len) < MD5_BYTES_PER_BLOCK)
  {
    tmp_ptr = &(x->left_over[n]);
    x->amount_left_over += len;
    while (len)
    {
      *tmp_ptr++ = *text++;
      len--;
    }
    return;
  }

  /* copy enough from new to fill left over and process it */
  if (n > 0)
  {
    tmp_ptr = &(x->left_over[n]);
    len -= MD5_BYTES_PER_BLOCK - n;
    while (n++ < MD5_BYTES_PER_BLOCK)
    {
      *tmp_ptr++ = *text++;
    }
    md5_main_loop(x, x->left_over);
    x->amount_left_over = 0;
  }

  /* process new in BLOCK SIZE chunks and store extra */
  while (len >= MD5_BYTES_PER_BLOCK)
  {
    md5_main_loop(x, text);
    len -= MD5_BYTES_PER_BLOCK;
    text += MD5_BYTES_PER_BLOCK;
  }
  tmp_ptr = x->left_over;
  x->amount_left_over = len;
  while (len-- > 0)
  {
    *tmp_ptr++ = *text++;
  }
}

void 
md5_end(struct hash_block *x, unsigned char *message_digest)
{
  int                 i;

  if (x->amount_left_over > ((MD5_BYTES_PER_BLOCK - (64 / 8)) - 1))
  {
    x->left_over[x->amount_left_over++] = 0x80;
    while (x->amount_left_over < MD5_BYTES_PER_BLOCK)
    {
      x->left_over[x->amount_left_over++] = 0;
    }
    md5_main_loop(x, x->left_over);
    for (i = 0; i < (MD5_BYTES_PER_BLOCK - (64 / 8)); i++)
    {
      x->left_over[i] = 0;
    }
    x->amount_left_over = (MD5_BYTES_PER_BLOCK - (64 / 8));
  }
  if (x->amount_left_over < (MD5_BYTES_PER_BLOCK - (64 / 8)))
  {
    x->left_over[x->amount_left_over++] = 0x80;
  }
  while (x->amount_left_over < MD5_BYTES_PER_BLOCK - (64 / 8))
  {
    x->left_over[x->amount_left_over++] = 0;
  }
  x->left_over[MD5_BYTES_PER_BLOCK - 8] = (unsigned char) (((x->message_length[0]) << 3) & 0xff);
  x->left_over[MD5_BYTES_PER_BLOCK - 7] = (unsigned char) (((x->message_length[0]) >> 5) & 0xff);
  x->left_over[MD5_BYTES_PER_BLOCK - 6] = (unsigned char) (((x->message_length[0]) >> 13) & 0xff);
  x->left_over[MD5_BYTES_PER_BLOCK - 5] = (unsigned char) (((x->message_length[0]) >> 21) & 0xff);

  x->left_over[MD5_BYTES_PER_BLOCK - 4] = (unsigned char)
    (
      (
        ((x->message_length[0]) >> 29) |
        ((x->message_length[1]) << 3)
      ) & 0xff
    );

  x->left_over[MD5_BYTES_PER_BLOCK - 3] = (unsigned char) (((x->message_length[1]) >> 5) & 0xff);
  x->left_over[MD5_BYTES_PER_BLOCK - 2] = (unsigned char) (((x->message_length[1]) >> 13) & 0xff);
  x->left_over[MD5_BYTES_PER_BLOCK - 1] = (unsigned char) (((x->message_length[1]) >> 21) & 0xff);

  md5_main_loop(x, x->left_over);

  message_digest[ 0] = (unsigned char) ((x->A) & 0xff);
  message_digest[ 1] = (unsigned char) (((x->A) >> 8) & 0xff);
  message_digest[ 2] = (unsigned char) (((x->A) >> 16) & 0xff);
  message_digest[ 3] = (unsigned char) (((x->A) >> 24) & 0xff);
  message_digest[ 4] = (unsigned char) ((x->B) & 0xff);
  message_digest[ 5] = (unsigned char) (((x->B) >> 8) & 0xff);
  message_digest[ 6] = (unsigned char) (((x->B) >> 16) & 0xff);
  message_digest[ 7] = (unsigned char) (((x->B) >> 24) & 0xff);
  message_digest[ 8] = (unsigned char) ((x->C) & 0xff);
  message_digest[ 9] = (unsigned char) (((x->C) >> 8) & 0xff);
  message_digest[10] = (unsigned char) (((x->C) >> 16) & 0xff);
  message_digest[11] = (unsigned char) (((x->C) >> 24) & 0xff);
  message_digest[12] = (unsigned char) ((x->D) & 0xff);
  message_digest[13] = (unsigned char) (((x->D) >> 8) & 0xff);
  message_digest[14] = (unsigned char) (((x->D) >> 16) & 0xff);
  message_digest[15] = (unsigned char) (((x->D) >> 24) & 0xff);
}

#define F(X,Y,Z) (((X)&(Y))|((~(X))&(Z)))
#define G(X,Y,Z) (((X)&(Z))|((Y)&(~(Z))))
#define H(X,Y,Z) ((X)^(Y)^(Z))
#define I(X,Y,Z) ((Y)^((X)|(~(Z))))

#define CIRC_LSHIFT(x,n) (((x)<<(n))|(((unsigned long)(x))>>(32-(n))))

#define FF(a,b,c,d,M,s,t) { (a)+=F(b,c,d)+(M)+(t); a=(b)+CIRC_LSHIFT((a),s); }
#define GG(a,b,c,d,M,s,t) { (a)+=G(b,c,d)+(M)+(t); a=(b)+CIRC_LSHIFT((a),s); }
#define HH(a,b,c,d,M,s,t) { (a)+=H(b,c,d)+(M)+(t); a=(b)+CIRC_LSHIFT((a),s); }
#define II(a,b,c,d,M,s,t) { (a)+=I(b,c,d)+(M)+(t); a=(b)+CIRC_LSHIFT((a),s); }

void 
md5_main_loop(struct hash_block *x, unsigned char *text)
{
  unsigned long       a,
                      b,
                      c,
                      d;
  unsigned long       M[16],
                     *Mptr;
  int                 i;

  Mptr = M;
  for (i = 0; i < 16; i++)
  {
    *Mptr = (*text++);
    *Mptr |= (*text++) << 8;
    *Mptr |= (*text++) << 16;
    *Mptr |= (*text++) << 24;
    Mptr++;
  }

  a = x->A;
  b = x->B;
  c = x->C;
  d = x->D;

  /* Round 1: */
  FF(a, b, c, d, M[ 0],  7, 0xd76aa478);
  FF(d, a, b, c, M[ 1], 12, 0xe8c7b756);
  FF(c, d, a, b, M[ 2], 17, 0x242070db);
  FF(b, c, d, a, M[ 3], 22, 0xc1bdceee);
  FF(a, b, c, d, M[ 4],  7, 0xf57c0faf);
  FF(d, a, b, c, M[ 5], 12, 0x4787c62a);
  FF(c, d, a, b, M[ 6], 17, 0xa8304613);
  FF(b, c, d, a, M[ 7], 22, 0xfd469501);
  FF(a, b, c, d, M[ 8],  7, 0x698098d8);
  FF(d, a, b, c, M[ 9], 12, 0x8b44f7af);
  FF(c, d, a, b, M[10], 17, 0xffff5bb1);
  FF(b, c, d, a, M[11], 22, 0x895cd7be);
  FF(a, b, c, d, M[12],  7, 0x6b901122);
  FF(d, a, b, c, M[13], 12, 0xfd987193);
  FF(c, d, a, b, M[14], 17, 0xa679438e);
  FF(b, c, d, a, M[15], 22, 0x49b40821);

  /* Round 2: */
  GG(a, b, c, d, M[ 1],  5, 0xf61e2562);
  GG(d, a, b, c, M[ 6],  9, 0xc040b340);
  GG(c, d, a, b, M[11], 14, 0x265e5a51);
  GG(b, c, d, a, M[ 0], 20, 0xe9b6c7aa);
  GG(a, b, c, d, M[ 5],  5, 0xd62f105d);
  GG(d, a, b, c, M[10],  9, 0x02441453);
  GG(c, d, a, b, M[15], 14, 0xd8a1e681);
  GG(b, c, d, a, M[ 4], 20, 0xe7d3fbc8);
  GG(a, b, c, d, M[ 9],  5, 0x21e1cde6);
  GG(d, a, b, c, M[14],  9, 0xc33707d6);
  GG(c, d, a, b, M[ 3], 14, 0xf4d50d87);
  GG(b, c, d, a, M[ 8], 20, 0x455a14ed);
  GG(a, b, c, d, M[13],  5, 0xa9e3e905);
  GG(d, a, b, c, M[ 2],  9, 0xfcefa3f8);
  GG(c, d, a, b, M[ 7], 14, 0x676f02d9);
  GG(b, c, d, a, M[12], 20, 0x8d2a4c8a);

  /* Round 3: */
  HH(a, b, c, d, M[ 5],  4, 0xfffa3942);
  HH(d, a, b, c, M[ 8], 11, 0x8771f681);
  HH(c, d, a, b, M[11], 16, 0x6d9d6122);
  HH(b, c, d, a, M[14], 23, 0xfde5380c);
  HH(a, b, c, d, M[ 1],  4, 0xa4beea44);
  HH(d, a, b, c, M[ 4], 11, 0x4bdecfa9);
  HH(c, d, a, b, M[ 7], 16, 0xf6bb4b60);
  HH(b, c, d, a, M[10], 23, 0xbebfbc70);
  HH(a, b, c, d, M[13],  4, 0x289b7ec6);
  HH(d, a, b, c, M[ 0], 11, 0xeaa127fa);
  HH(c, d, a, b, M[ 3], 16, 0xd4ef3085);
  HH(b, c, d, a, M[ 6], 23, 0x04881d05);
  HH(a, b, c, d, M[ 9],  4, 0xd9d4d039);
  HH(d, a, b, c, M[12], 11, 0xe6db99e5);
  HH(c, d, a, b, M[15], 16, 0x1fa27cf8);
  HH(b, c, d, a, M[ 2], 23, 0xc4ac5665);

  /* Round 4: */
  II(a, b, c, d, M[ 0],  6, 0xf4292244);
  II(d, a, b, c, M[ 7], 10, 0x432aff97);
  II(c, d, a, b, M[14], 15, 0xab9423a7);
  II(b, c, d, a, M[ 5], 21, 0xfc93a039);
  II(a, b, c, d, M[12],  6, 0x655b59c3);
  II(d, a, b, c, M[ 3], 10, 0x8f0ccc92);
  II(c, d, a, b, M[10], 15, 0xffeff47d);
  II(b, c, d, a, M[ 1], 21, 0x85845dd1);
  II(a, b, c, d, M[ 8],  6, 0x6fa87e4f);
  II(d, a, b, c, M[15], 10, 0xfe2ce6e0);
  II(c, d, a, b, M[ 6], 15, 0xa3014314);
  II(b, c, d, a, M[13], 21, 0x4e0811a1);
  II(a, b, c, d, M[ 4],  6, 0xf7537e82);
  II(d, a, b, c, M[11], 10, 0xbd3af235);
  II(c, d, a, b, M[ 2], 15, 0x2ad7d2bb);
  II(b, c, d, a, M[ 9], 21, 0xeb86d391);

  x->A += a;
  x->B += b;
  x->C += c;
  x->D += d;
}
