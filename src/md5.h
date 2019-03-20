/*
 ***********************************************************************
 *
 * $Id: md5.h,v 1.2 2002/09/18 18:05:24 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#define MD5_HASH_LENGTH         16
#define MD5_HASH_STRING_LENGTH  33
#define MD5_READ_BUFSIZE    0x8000
#define MD5_BYTES_PER_BLOCK (512/8)

#ifdef K_CPU_ALPHA
typedef unsigned int  MD5_UINT32;
#else
typedef unsigned long MD5_UINT32;
#endif

struct hash_block
{
  MD5_UINT32          A;
  MD5_UINT32          B;
  MD5_UINT32          C;
  MD5_UINT32          D;
  MD5_UINT32          message_length[2];
  MD5_UINT32          amount_left_over;
  unsigned char       left_over[MD5_BYTES_PER_BLOCK];
};

int                 md5_file(FILE *fp, unsigned char *message_digest);
void                md5_string(unsigned char *text, int text_len, unsigned char *message_digest);
void                md5_begin(struct hash_block *x);
void                md5_middle(struct hash_block *x, unsigned char *text, MD5_UINT32 len);
void                md5_end(struct hash_block *x, unsigned char *message_digest);
void                md5_main_loop(struct hash_block *x, unsigned char *text);
