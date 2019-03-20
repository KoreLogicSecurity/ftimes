/*
 ***********************************************************************
 *
 * $Id: md5.h,v 1.1.1.1 2002/01/18 03:17:37 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include <stdio.h>

#define MD5_HASH_LENGTH         16
#define MD5_HASH_STRING_LENGTH  33
#define MD5_READ_BUFSIZE    0x8000
#define MD5_BYTES_PER_BLOCK (512/8)

struct hash_block
{
  unsigned long       A;
  unsigned long       B;
  unsigned long       C;
  unsigned long       D;
  unsigned long       message_length[2];
  unsigned char       left_over[MD5_BYTES_PER_BLOCK];
  unsigned long       amount_left_over;
};

int                 md5_file(FILE *fp, unsigned char *message_digest);
void                md5_string(unsigned char *text, int text_len, unsigned char *message_digest);
void                md5_begin(struct hash_block *x);
void                md5_middle(struct hash_block *x, unsigned char *text, unsigned long len);
void                md5_end(struct hash_block *x, unsigned char *message_digest);
void                md5_main_loop(struct hash_block *x, unsigned char *text);
