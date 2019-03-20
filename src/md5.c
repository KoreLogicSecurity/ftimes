/*-
 ***********************************************************************
 *
 * $Id: md5.c,v 1.14 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2003-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static unsigned char  gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*-
 ***********************************************************************
 *
 * MD5HashToBase64
 *
 ***********************************************************************
 */
int
MD5HashToBase64(unsigned char *pucHash, char *pcBase64Hash)
{
  int                 i = 0;
  int                 n = 0;
  unsigned long       ul = 0;
  unsigned long       ulLeft = 0;

  for (i = 0; i < MD5_HASH_SIZE; i++)
  {
    ul = (ul << 8) | pucHash[i];
    ulLeft += 8;
    while (ulLeft > 6)
    {
      pcBase64Hash[n++] = gaucBase64[(ul >> (ulLeft - 6)) & 0x3f];
      ulLeft -= 6;
    }
  }
  if (ulLeft != 0)
  {
    pcBase64Hash[n++] = gaucBase64[(ul << (6 - ulLeft)) & 0x3f];
  }
  pcBase64Hash[n] = 0;

  return n;
}

/*-
 ***********************************************************************
 *
 * MD5HashToHex
 *
 ***********************************************************************
 */
int
MD5HashToHex(unsigned char *pucHash, char *pcHexHash)
{
  int                 n = 0;
  unsigned int       *pui = (unsigned int *) pucHash;

  n += snprintf(&pcHexHash[n], (MD5_HASH_SIZE * 2 + 1), "%08x%08x%08x%08x",
    (unsigned int) htonl(*(pui    )),
    (unsigned int) htonl(*(pui + 1)),
    (unsigned int) htonl(*(pui + 2)),
    (unsigned int) htonl(*(pui + 3))
    );
  pcHexHash[n] = 0;

  return n;
}


/*-
 ***********************************************************************
 *
 * MD5HashStream
 *
 ***********************************************************************
 */
int
MD5HashStream(FILE *pFile, unsigned char *pucMD5)
{
  unsigned char       aucData[MD5_READ_SIZE];
#ifdef MD5_PRE_MEMSET_MEMCPY
  int                 i;
#endif
  int                 iNRead;
  MD5_CONTEXT         sMD5Context;

  MD5Alpha(&sMD5Context);
  while ((iNRead = fread(aucData, 1, MD5_READ_SIZE, pFile)) == MD5_READ_SIZE)
  {
    MD5Cycle(&sMD5Context, aucData, iNRead);
  }
  if (ferror(pFile))
  {
#ifdef MD5_PRE_MEMSET_MEMCPY
    for (i = 0; i < MD5_HASH_SIZE; i++)
    {
      pucMD5[i] = 0;
    }
#else
    memset(pucMD5, 0, MD5_HASH_SIZE);
#endif
    return -1;
  }
  MD5Cycle(&sMD5Context, aucData, iNRead);
  MD5Omega(&sMD5Context, pucMD5);
  return 0;
}


/*-
 ***********************************************************************
 *
 * MD5HashString
 *
 ***********************************************************************
 */
void
MD5HashString(unsigned char *pucData, int iLength, unsigned char *pucMD5)
{
  MD5_CONTEXT         sMD5Context;

  MD5Alpha(&sMD5Context);
  while (iLength > MD5_HUNK_SIZE)
  {
    MD5Cycle(&sMD5Context, pucData, MD5_HUNK_SIZE);
    pucData += MD5_HUNK_SIZE;
    iLength -= MD5_HUNK_SIZE;
  }
  MD5Cycle(&sMD5Context, pucData, iLength);
  MD5Omega(&sMD5Context, pucMD5);
}


/*-
 ***********************************************************************
 *
 * MD5Alpha
 *
 ***********************************************************************
 */
void
MD5Alpha(MD5_CONTEXT *psMD5Context)
{
#ifdef MD5_PRE_MEMSET_MEMCPY
  int                 i;
#endif

  psMD5Context->A = MD5_HA;
  psMD5Context->B = MD5_HB;
  psMD5Context->C = MD5_HC;
  psMD5Context->D = MD5_HD;
  psMD5Context->ui64MessageLength = 0;
  psMD5Context->ui32ResidueLength = 0;
#ifdef MD5_PRE_MEMSET_MEMCPY
  for (i = 0; i < MD5_HUNK_SIZE; i++)
  {
    psMD5Context->aucResidue[i] = 0;
  }
#else
  memset(psMD5Context->aucResidue, 0, MD5_HUNK_SIZE);
#endif
}


/*-
 ***********************************************************************
 *
 * MD5Cycle
 *
 ***********************************************************************
 */
void
MD5Cycle(MD5_CONTEXT *psMD5Context, unsigned char *pucData, K_UINT32 ui32Length)
{
  unsigned char      *pucTemp;
  K_UINT32            ui32;

  /*-
   *********************************************************************
   *
   * Update length. Track bytes here -- not bits.
   *
   *********************************************************************
   */
  psMD5Context->ui64MessageLength += (K_UINT64) ui32Length;

  /*-
   *********************************************************************
   *
   * If residue plus new is less than MD5_HUNK_SIZE, just store new.
   *
   *********************************************************************
   */
  if ((psMD5Context->ui32ResidueLength + ui32Length) < MD5_HUNK_SIZE)
  {
    pucTemp = &psMD5Context->aucResidue[psMD5Context->ui32ResidueLength];
    psMD5Context->ui32ResidueLength += ui32Length;
#ifdef MD5_PRE_MEMSET_MEMCPY
    while (ui32Length-- > 0)
    {
      *pucTemp++ = *pucData++;
    }
#else
    memcpy(pucTemp, pucData, ui32Length);
#endif
    return;
  }

  /*-
   *********************************************************************
   *
   * Copy enough from new to fill residue, and process it.
   *
   *********************************************************************
   */
  if (psMD5Context->ui32ResidueLength > 0)
  {
#ifdef MD5_PRE_MEMSET_MEMCPY
    pucTemp = &psMD5Context->aucResidue[psMD5Context->ui32ResidueLength];
    ui32Length -= MD5_HUNK_SIZE - psMD5Context->ui32ResidueLength;
    ui32 = psMD5Context->ui32ResidueLength;
    while (ui32++ < MD5_HUNK_SIZE)
    {
      *pucTemp++ = *pucData++;
    }
#else
    ui32 = MD5_HUNK_SIZE - psMD5Context->ui32ResidueLength; /* Note: This ui32 holds a different value than the one in the MD5_PRE_MEMSET_MEMCPY code. */
    ui32Length -= ui32;
    memcpy(&psMD5Context->aucResidue[psMD5Context->ui32ResidueLength], pucData, ui32);
    pucData += ui32;
#endif
    MD5Grind(psMD5Context, psMD5Context->aucResidue);
    psMD5Context->ui32ResidueLength = 0;
  }

  /*-
   *********************************************************************
   *
   * Process new in MD5_HUNK_SIZE chunks and store any residue.
   *
   *********************************************************************
   */
  while (ui32Length >= MD5_HUNK_SIZE)
  {
    MD5Grind(psMD5Context, pucData);
    ui32Length -= MD5_HUNK_SIZE;
    pucData += MD5_HUNK_SIZE;
  }
  pucTemp = psMD5Context->aucResidue;
  psMD5Context->ui32ResidueLength = ui32Length;
#ifdef MD5_PRE_MEMSET_MEMCPY
  while (ui32Length-- > 0)
  {
    *pucTemp++ = *pucData++;
  }
#else
  memcpy(pucTemp, pucData, ui32Length);
#endif
}


/*-
 ***********************************************************************
 *
 * MD5Omega
 *
 ***********************************************************************
 */
void
MD5Omega(MD5_CONTEXT *psMD5Context, unsigned char *pucMD5)
{
  /*-
   *********************************************************************
   *
   * Append padding, and grind if necessary.
   *
   *********************************************************************
   */
  psMD5Context->aucResidue[psMD5Context->ui32ResidueLength] = 0x80;
  if (psMD5Context->ui32ResidueLength++ >= (MD5_HUNK_SIZE - 8))
  {
#ifdef MD5_PRE_MEMSET_MEMCPY
    while (psMD5Context->ui32ResidueLength < MD5_HUNK_SIZE)
    {
      psMD5Context->aucResidue[psMD5Context->ui32ResidueLength++] = 0;
    }
#else
    memset(&psMD5Context->aucResidue[psMD5Context->ui32ResidueLength], 0, MD5_HUNK_SIZE - psMD5Context->ui32ResidueLength);
#endif
    MD5Grind(psMD5Context, psMD5Context->aucResidue);
    psMD5Context->ui32ResidueLength = 0;
  }
#ifdef MD5_PRE_MEMSET_MEMCPY
  while (psMD5Context->ui32ResidueLength < (MD5_HUNK_SIZE - 8))
  {
    psMD5Context->aucResidue[psMD5Context->ui32ResidueLength++] = 0;
  }
#else
  memset(&psMD5Context->aucResidue[psMD5Context->ui32ResidueLength], 0, (MD5_HUNK_SIZE - 8) - psMD5Context->ui32ResidueLength);
#endif

  /*-
   *********************************************************************
   *
   * Append length in bits (little-endian), and grind one last time.
   *
   *********************************************************************
   */
  psMD5Context->aucResidue[MD5_HUNK_SIZE-8] = (unsigned char) ((psMD5Context->ui64MessageLength <<  3) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-7] = (unsigned char) ((psMD5Context->ui64MessageLength >>  5) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-6] = (unsigned char) ((psMD5Context->ui64MessageLength >> 13) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-5] = (unsigned char) ((psMD5Context->ui64MessageLength >> 21) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-4] = (unsigned char) ((psMD5Context->ui64MessageLength >> 29) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-3] = (unsigned char) ((psMD5Context->ui64MessageLength >> 37) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-2] = (unsigned char) ((psMD5Context->ui64MessageLength >> 45) & 0xff);
  psMD5Context->aucResidue[MD5_HUNK_SIZE-1] = (unsigned char) ((psMD5Context->ui64MessageLength >> 53) & 0xff);

  MD5Grind(psMD5Context, psMD5Context->aucResidue);

  /*-
   *********************************************************************
   *
   * Transfer hash (little-endian) to user-supplied array.
   *
   *********************************************************************
   */
  pucMD5[ 0] = (unsigned char) ( (psMD5Context->A)        & 0xff);
  pucMD5[ 1] = (unsigned char) (((psMD5Context->A) >>  8) & 0xff);
  pucMD5[ 2] = (unsigned char) (((psMD5Context->A) >> 16) & 0xff);
  pucMD5[ 3] = (unsigned char) (((psMD5Context->A) >> 24) & 0xff);
  pucMD5[ 4] = (unsigned char) ( (psMD5Context->B)        & 0xff);
  pucMD5[ 5] = (unsigned char) (((psMD5Context->B) >>  8) & 0xff);
  pucMD5[ 6] = (unsigned char) (((psMD5Context->B) >> 16) & 0xff);
  pucMD5[ 7] = (unsigned char) (((psMD5Context->B) >> 24) & 0xff);
  pucMD5[ 8] = (unsigned char) ( (psMD5Context->C)        & 0xff);
  pucMD5[ 9] = (unsigned char) (((psMD5Context->C) >>  8) & 0xff);
  pucMD5[10] = (unsigned char) (((psMD5Context->C) >> 16) & 0xff);
  pucMD5[11] = (unsigned char) (((psMD5Context->C) >> 24) & 0xff);
  pucMD5[12] = (unsigned char) ( (psMD5Context->D)        & 0xff);
  pucMD5[13] = (unsigned char) (((psMD5Context->D) >>  8) & 0xff);
  pucMD5[14] = (unsigned char) (((psMD5Context->D) >> 16) & 0xff);
  pucMD5[15] = (unsigned char) (((psMD5Context->D) >> 24) & 0xff);
}


/*-
 ***********************************************************************
 *
 * MD5Grind
 *
 ***********************************************************************
 */
void
MD5Grind(MD5_CONTEXT *psMD5Context, unsigned char *pucData)
{
  K_UINT32            a;
  K_UINT32            b;
  K_UINT32            c;
  K_UINT32            d;
  K_UINT32            M[16];
  int                 i;

  /*-
   *********************************************************************
   *
   * Prepare the message schedule (little-endian).
   *
   *********************************************************************
   */
  for (i = 0; i < 16; i++)
  {
    M[i]  = (*pucData++)      ;
    M[i] |= (*pucData++) <<  8;
    M[i] |= (*pucData++) << 16;
    M[i] |= (*pucData++) << 24;
  }

  /*-
   *********************************************************************
   *
   * Initialize working variables.
   *
   *********************************************************************
   */
  a = psMD5Context->A;
  b = psMD5Context->B;
  c = psMD5Context->C;
  d = psMD5Context->D;

  /*-
   *********************************************************************
   *
   * Do round 1.
   *
   *********************************************************************
   */
  MD5_R1(a, b, c, d, M[ 0],  7, 0xd76aa478);
  MD5_R1(d, a, b, c, M[ 1], 12, 0xe8c7b756);
  MD5_R1(c, d, a, b, M[ 2], 17, 0x242070db);
  MD5_R1(b, c, d, a, M[ 3], 22, 0xc1bdceee);
  MD5_R1(a, b, c, d, M[ 4],  7, 0xf57c0faf);
  MD5_R1(d, a, b, c, M[ 5], 12, 0x4787c62a);
  MD5_R1(c, d, a, b, M[ 6], 17, 0xa8304613);
  MD5_R1(b, c, d, a, M[ 7], 22, 0xfd469501);
  MD5_R1(a, b, c, d, M[ 8],  7, 0x698098d8);
  MD5_R1(d, a, b, c, M[ 9], 12, 0x8b44f7af);
  MD5_R1(c, d, a, b, M[10], 17, 0xffff5bb1);
  MD5_R1(b, c, d, a, M[11], 22, 0x895cd7be);
  MD5_R1(a, b, c, d, M[12],  7, 0x6b901122);
  MD5_R1(d, a, b, c, M[13], 12, 0xfd987193);
  MD5_R1(c, d, a, b, M[14], 17, 0xa679438e);
  MD5_R1(b, c, d, a, M[15], 22, 0x49b40821);

  /*-
   *********************************************************************
   *
   * Do round 2.
   *
   *********************************************************************
   */
  MD5_R2(a, b, c, d, M[ 1],  5, 0xf61e2562);
  MD5_R2(d, a, b, c, M[ 6],  9, 0xc040b340);
  MD5_R2(c, d, a, b, M[11], 14, 0x265e5a51);
  MD5_R2(b, c, d, a, M[ 0], 20, 0xe9b6c7aa);
  MD5_R2(a, b, c, d, M[ 5],  5, 0xd62f105d);
  MD5_R2(d, a, b, c, M[10],  9, 0x02441453);
  MD5_R2(c, d, a, b, M[15], 14, 0xd8a1e681);
  MD5_R2(b, c, d, a, M[ 4], 20, 0xe7d3fbc8);
  MD5_R2(a, b, c, d, M[ 9],  5, 0x21e1cde6);
  MD5_R2(d, a, b, c, M[14],  9, 0xc33707d6);
  MD5_R2(c, d, a, b, M[ 3], 14, 0xf4d50d87);
  MD5_R2(b, c, d, a, M[ 8], 20, 0x455a14ed);
  MD5_R2(a, b, c, d, M[13],  5, 0xa9e3e905);
  MD5_R2(d, a, b, c, M[ 2],  9, 0xfcefa3f8);
  MD5_R2(c, d, a, b, M[ 7], 14, 0x676f02d9);
  MD5_R2(b, c, d, a, M[12], 20, 0x8d2a4c8a);

  /*-
   *********************************************************************
   *
   * Do round 3.
   *
   *********************************************************************
   */
  MD5_R3(a, b, c, d, M[ 5],  4, 0xfffa3942);
  MD5_R3(d, a, b, c, M[ 8], 11, 0x8771f681);
  MD5_R3(c, d, a, b, M[11], 16, 0x6d9d6122);
  MD5_R3(b, c, d, a, M[14], 23, 0xfde5380c);
  MD5_R3(a, b, c, d, M[ 1],  4, 0xa4beea44);
  MD5_R3(d, a, b, c, M[ 4], 11, 0x4bdecfa9);
  MD5_R3(c, d, a, b, M[ 7], 16, 0xf6bb4b60);
  MD5_R3(b, c, d, a, M[10], 23, 0xbebfbc70);
  MD5_R3(a, b, c, d, M[13],  4, 0x289b7ec6);
  MD5_R3(d, a, b, c, M[ 0], 11, 0xeaa127fa);
  MD5_R3(c, d, a, b, M[ 3], 16, 0xd4ef3085);
  MD5_R3(b, c, d, a, M[ 6], 23, 0x04881d05);
  MD5_R3(a, b, c, d, M[ 9],  4, 0xd9d4d039);
  MD5_R3(d, a, b, c, M[12], 11, 0xe6db99e5);
  MD5_R3(c, d, a, b, M[15], 16, 0x1fa27cf8);
  MD5_R3(b, c, d, a, M[ 2], 23, 0xc4ac5665);

  /*-
   *********************************************************************
   *
   * Do round 4.
   *
   *********************************************************************
   */
  MD5_R4(a, b, c, d, M[ 0],  6, 0xf4292244);
  MD5_R4(d, a, b, c, M[ 7], 10, 0x432aff97);
  MD5_R4(c, d, a, b, M[14], 15, 0xab9423a7);
  MD5_R4(b, c, d, a, M[ 5], 21, 0xfc93a039);
  MD5_R4(a, b, c, d, M[12],  6, 0x655b59c3);
  MD5_R4(d, a, b, c, M[ 3], 10, 0x8f0ccc92);
  MD5_R4(c, d, a, b, M[10], 15, 0xffeff47d);
  MD5_R4(b, c, d, a, M[ 1], 21, 0x85845dd1);
  MD5_R4(a, b, c, d, M[ 8],  6, 0x6fa87e4f);
  MD5_R4(d, a, b, c, M[15], 10, 0xfe2ce6e0);
  MD5_R4(c, d, a, b, M[ 6], 15, 0xa3014314);
  MD5_R4(b, c, d, a, M[13], 21, 0x4e0811a1);
  MD5_R4(a, b, c, d, M[ 4],  6, 0xf7537e82);
  MD5_R4(d, a, b, c, M[11], 10, 0xbd3af235);
  MD5_R4(c, d, a, b, M[ 2], 15, 0x2ad7d2bb);
  MD5_R4(b, c, d, a, M[ 9], 21, 0xeb86d391);

  /*-
   *********************************************************************
   *
   * Compute intermediate hash value.
   *
   *********************************************************************
   */
  psMD5Context->A += a;
  psMD5Context->B += b;
  psMD5Context->C += c;
  psMD5Context->D += d;
}
