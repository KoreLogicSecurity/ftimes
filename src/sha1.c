/*-
 ***********************************************************************
 *
 * $Id: sha1.c,v 1.4 2007/02/23 00:22:35 mavrik Exp $
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
 * SHA1HashToBase64
 *
 ***********************************************************************
 */
int
SHA1HashToBase64(unsigned char *pucHash, char *pcBase64Hash)
{
  int                 i = 0;
  int                 n = 0;
  unsigned long       ul = 0;
  unsigned long       ulLeft = 0;

  for (i = 0; i < SHA1_HASH_SIZE; i++)
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
 * SHA1HashToHex
 *
 ***********************************************************************
 */
int
SHA1HashToHex(unsigned char *pucHash, char *pcHexHash)
{
  int                 n = 0;
  unsigned int       *pui = (unsigned int *) pucHash;

  n += snprintf(&pcHexHash[n], (SHA1_HASH_SIZE * 2 + 1), "%08x%08x%08x%08x%08x",
    (unsigned int) htonl(*(pui    )),
    (unsigned int) htonl(*(pui + 1)),
    (unsigned int) htonl(*(pui + 2)),
    (unsigned int) htonl(*(pui + 3)),
    (unsigned int) htonl(*(pui + 4))
    );
  pcHexHash[n] = 0;

  return n;
}


/*-
 ***********************************************************************
 *
 * SHA1HashStream
 *
 ***********************************************************************
 */
int
SHA1HashStream(FILE *pFile, unsigned char *pucSHA1)
{
  unsigned char       aucData[SHA1_READ_SIZE];
#ifdef SHA1_PRE_MEMSET_MEMCPY
  int                 i;
#endif
  int                 iNRead;
  SHA1_CONTEXT        sSHA1Context;

  SHA1Alpha(&sSHA1Context);
  while ((iNRead = fread(aucData, 1, SHA1_READ_SIZE, pFile)) == SHA1_READ_SIZE)
  {
    SHA1Cycle(&sSHA1Context, aucData, iNRead);
  }
  if (ferror(pFile))
  {
#ifdef SHA1_PRE_MEMSET_MEMCPY
    for (i = 0; i < SHA1_HASH_SIZE; i++)
    {
      pucSHA1[i] = 0;
    }
#else
    memset(pucSHA1, 0, SHA1_HASH_SIZE);
#endif
    return -1;
  }
  SHA1Cycle(&sSHA1Context, aucData, iNRead);
  SHA1Omega(&sSHA1Context, pucSHA1);
  return 0;
}


/*-
 ***********************************************************************
 *
 * SHA1HashString
 *
 ***********************************************************************
 */
void
SHA1HashString(unsigned char *pucData, int iLength, unsigned char *pucSHA1)
{
  SHA1_CONTEXT        sSHA1Context;

  SHA1Alpha(&sSHA1Context);
  while (iLength > SHA1_HUNK_SIZE)
  {
    SHA1Cycle(&sSHA1Context, pucData, SHA1_HUNK_SIZE);
    pucData += SHA1_HUNK_SIZE;
    iLength -= SHA1_HUNK_SIZE;
  }
  SHA1Cycle(&sSHA1Context, pucData, iLength);
  SHA1Omega(&sSHA1Context, pucSHA1);
}


/*-
 ***********************************************************************
 *
 * SHA1Alpha
 *
 ***********************************************************************
 */
void
SHA1Alpha(SHA1_CONTEXT *psSHA1Context)
{
#ifdef SHA1_PRE_MEMSET_MEMCPY
  int                 i;
#endif

  psSHA1Context->A = SHA1_HA;
  psSHA1Context->B = SHA1_HB;
  psSHA1Context->C = SHA1_HC;
  psSHA1Context->D = SHA1_HD;
  psSHA1Context->E = SHA1_HE;
  psSHA1Context->ui64MessageLength = 0;
  psSHA1Context->ui32ResidueLength = 0;
#ifdef SHA1_PRE_MEMSET_MEMCPY
  for (i = 0; i < SHA1_HUNK_SIZE; i++)
  {
    psSHA1Context->aucResidue[i] = 0;
  }
#else
  memset(psSHA1Context->aucResidue, 0, SHA1_HUNK_SIZE);
#endif
}


/*-
 ***********************************************************************
 *
 * SHA1Cycle
 *
 ***********************************************************************
 */
void
SHA1Cycle(SHA1_CONTEXT *psSHA1Context, unsigned char *pucData, K_UINT32 ui32Length)
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
  psSHA1Context->ui64MessageLength += (K_UINT64) ui32Length;

  /*-
   *********************************************************************
   *
   * If residue plus new is less than SHA1_HUNK_SIZE, just store new.
   *
   *********************************************************************
   */
  if ((psSHA1Context->ui32ResidueLength + ui32Length) < SHA1_HUNK_SIZE)
  {
    pucTemp = &psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength];
    psSHA1Context->ui32ResidueLength += ui32Length;
#ifdef SHA1_PRE_MEMSET_MEMCPY
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
  if (psSHA1Context->ui32ResidueLength > 0)
  {
#ifdef SHA1_PRE_MEMSET_MEMCPY
    pucTemp = &psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength];
    ui32Length -= SHA1_HUNK_SIZE - psSHA1Context->ui32ResidueLength;
    ui32 = psSHA1Context->ui32ResidueLength;
    while (ui32++ < SHA1_HUNK_SIZE)
    {
      *pucTemp++ = *pucData++;
    }
#else
    ui32 = SHA1_HUNK_SIZE - psSHA1Context->ui32ResidueLength; /* Note: This ui32 holds a different value than the one in the SHA1_PRE_MEMSET_MEMCPY code. */
    ui32Length -= ui32;
    memcpy(&psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength], pucData, ui32);
    pucData += ui32;
#endif
    SHA1Grind(psSHA1Context, psSHA1Context->aucResidue);
    psSHA1Context->ui32ResidueLength = 0;
  }

  /*-
   *********************************************************************
   *
   * Process new in SHA1_HUNK_SIZE chunks and store any residue.
   *
   *********************************************************************
   */
  while (ui32Length >= SHA1_HUNK_SIZE)
  {
    SHA1Grind(psSHA1Context, pucData);
    ui32Length -= SHA1_HUNK_SIZE;
    pucData += SHA1_HUNK_SIZE;
  }
  pucTemp = psSHA1Context->aucResidue;
  psSHA1Context->ui32ResidueLength = ui32Length;
#ifdef SHA1_PRE_MEMSET_MEMCPY
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
 * SHA1Omega
 *
 ***********************************************************************
 */
void
SHA1Omega(SHA1_CONTEXT *psSHA1Context, unsigned char *pucSHA1)
{
  /*-
   *********************************************************************
   *
   * Append padding, and grind if necessary.
   *
   *********************************************************************
   */
  psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength] = 0x80;
  if (psSHA1Context->ui32ResidueLength++ >= (SHA1_HUNK_SIZE - 8))
  {
#ifdef SHA1_PRE_MEMSET_MEMCPY
    while (psSHA1Context->ui32ResidueLength < SHA1_HUNK_SIZE)
    {
      psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength++] = 0;
    }
#else
    memset(&psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength], 0, SHA1_HUNK_SIZE - psSHA1Context->ui32ResidueLength);
#endif
    SHA1Grind(psSHA1Context, psSHA1Context->aucResidue);
    psSHA1Context->ui32ResidueLength = 0;
  }
#ifdef SHA1_PRE_MEMSET_MEMCPY
  while (psSHA1Context->ui32ResidueLength < (SHA1_HUNK_SIZE - 8))
  {
    psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength++] = 0;
  }
#else
  memset(&psSHA1Context->aucResidue[psSHA1Context->ui32ResidueLength], 0, (SHA1_HUNK_SIZE - 8) - psSHA1Context->ui32ResidueLength);
#endif

  /*-
   *********************************************************************
   *
   * Append length in bits (big-endian), and grind one last time.
   *
   *********************************************************************
   */
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-8] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 53) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-7] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 45) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-6] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 37) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-5] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 29) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-4] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 21) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-3] = (unsigned char) ((psSHA1Context->ui64MessageLength >> 13) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-2] = (unsigned char) ((psSHA1Context->ui64MessageLength >>  5) & 0xff);
  psSHA1Context->aucResidue[SHA1_HUNK_SIZE-1] = (unsigned char) ((psSHA1Context->ui64MessageLength <<  3) & 0xff);

  SHA1Grind(psSHA1Context, psSHA1Context->aucResidue);

  /*-
   *********************************************************************
   *
   * Transfer hash (big-endian) to user-supplied array.
   *
   *********************************************************************
   */
  pucSHA1[ 0] = (unsigned char) (((psSHA1Context->A) >> 24) & 0xff);
  pucSHA1[ 1] = (unsigned char) (((psSHA1Context->A) >> 16) & 0xff);
  pucSHA1[ 2] = (unsigned char) (((psSHA1Context->A) >>  8) & 0xff);
  pucSHA1[ 3] = (unsigned char) ( (psSHA1Context->A)        & 0xff);
  pucSHA1[ 4] = (unsigned char) (((psSHA1Context->B) >> 24) & 0xff);
  pucSHA1[ 5] = (unsigned char) (((psSHA1Context->B) >> 16) & 0xff);
  pucSHA1[ 6] = (unsigned char) (((psSHA1Context->B) >>  8) & 0xff);
  pucSHA1[ 7] = (unsigned char) ( (psSHA1Context->B)        & 0xff);
  pucSHA1[ 8] = (unsigned char) (((psSHA1Context->C) >> 24) & 0xff);
  pucSHA1[ 9] = (unsigned char) (((psSHA1Context->C) >> 16) & 0xff);
  pucSHA1[10] = (unsigned char) (((psSHA1Context->C) >>  8) & 0xff);
  pucSHA1[11] = (unsigned char) ( (psSHA1Context->C)        & 0xff);
  pucSHA1[12] = (unsigned char) (((psSHA1Context->D) >> 24) & 0xff);
  pucSHA1[13] = (unsigned char) (((psSHA1Context->D) >> 16) & 0xff);
  pucSHA1[14] = (unsigned char) (((psSHA1Context->D) >>  8) & 0xff);
  pucSHA1[15] = (unsigned char) ( (psSHA1Context->D)        & 0xff);
  pucSHA1[16] = (unsigned char) (((psSHA1Context->E) >> 24) & 0xff);
  pucSHA1[17] = (unsigned char) (((psSHA1Context->E) >> 16) & 0xff);
  pucSHA1[18] = (unsigned char) (((psSHA1Context->E) >>  8) & 0xff);
  pucSHA1[19] = (unsigned char) ( (psSHA1Context->E)        & 0xff);
}


/*-
 ***********************************************************************
 *
 * SHA1Grind
 *
 ***********************************************************************
 */
void
SHA1Grind(SHA1_CONTEXT *psSHA1Context, unsigned char *pucData)
{
  K_UINT32            a;
  K_UINT32            b;
  K_UINT32            c;
  K_UINT32            d;
  K_UINT32            e;
  K_UINT32            W[80];
  int                 t;

  /*-
   *********************************************************************
   *
   * Prepare the message schedule (big-endian).
   *
   *********************************************************************
   */
  for (t = 0; t <= 15; t++)
  {
    W[t]  = (*pucData++) << 24;
    W[t] |= (*pucData++) << 16;
    W[t] |= (*pucData++) <<  8;
    W[t] |= (*pucData++)      ;
  }
  for (t = 16; t <= 79; t++)
  {
    W[t] = SHA1_ROTL((W[t-3]^W[t-8]^W[t-14]^W[t-16]),1);
  }

  /*-
   *********************************************************************
   *
   * Initialize working variables.
   *
   *********************************************************************
   */
  a = psSHA1Context->A;
  b = psSHA1Context->B;
  c = psSHA1Context->C;
  d = psSHA1Context->D;
  e = psSHA1Context->E;

  /*-
   *********************************************************************
   *
   * Do round 1.
   *
   *********************************************************************
   */
  SHA1_R1(a, b, c, d, e, W[ 0]);
  SHA1_R1(e, a, b, c, d, W[ 1]);
  SHA1_R1(d, e, a, b, c, W[ 2]);
  SHA1_R1(c, d, e, a, b, W[ 3]);
  SHA1_R1(b, c, d, e, a, W[ 4]);
  SHA1_R1(a, b, c, d, e, W[ 5]);
  SHA1_R1(e, a, b, c, d, W[ 6]);
  SHA1_R1(d, e, a, b, c, W[ 7]);
  SHA1_R1(c, d, e, a, b, W[ 8]);
  SHA1_R1(b, c, d, e, a, W[ 9]);
  SHA1_R1(a, b, c, d, e, W[10]);
  SHA1_R1(e, a, b, c, d, W[11]);
  SHA1_R1(d, e, a, b, c, W[12]);
  SHA1_R1(c, d, e, a, b, W[13]);
  SHA1_R1(b, c, d, e, a, W[14]);
  SHA1_R1(a, b, c, d, e, W[15]);
  SHA1_R1(e, a, b, c, d, W[16]);
  SHA1_R1(d, e, a, b, c, W[17]);
  SHA1_R1(c, d, e, a, b, W[18]);
  SHA1_R1(b, c, d, e, a, W[19]);

  /*-
   *********************************************************************
   *
   * Do round 2.
   *
   *********************************************************************
   */
  SHA1_R2(a, b, c, d, e, W[20]);
  SHA1_R2(e, a, b, c, d, W[21]);
  SHA1_R2(d, e, a, b, c, W[22]);
  SHA1_R2(c, d, e, a, b, W[23]);
  SHA1_R2(b, c, d, e, a, W[24]);
  SHA1_R2(a, b, c, d, e, W[25]);
  SHA1_R2(e, a, b, c, d, W[26]);
  SHA1_R2(d, e, a, b, c, W[27]);
  SHA1_R2(c, d, e, a, b, W[28]);
  SHA1_R2(b, c, d, e, a, W[29]);
  SHA1_R2(a, b, c, d, e, W[30]);
  SHA1_R2(e, a, b, c, d, W[31]);
  SHA1_R2(d, e, a, b, c, W[32]);
  SHA1_R2(c, d, e, a, b, W[33]);
  SHA1_R2(b, c, d, e, a, W[34]);
  SHA1_R2(a, b, c, d, e, W[35]);
  SHA1_R2(e, a, b, c, d, W[36]);
  SHA1_R2(d, e, a, b, c, W[37]);
  SHA1_R2(c, d, e, a, b, W[38]);
  SHA1_R2(b, c, d, e, a, W[39]);

  /*-
   *********************************************************************
   *
   * Do round 3.
   *
   *********************************************************************
   */
  SHA1_R3(a, b, c, d, e, W[40]);
  SHA1_R3(e, a, b, c, d, W[41]);
  SHA1_R3(d, e, a, b, c, W[42]);
  SHA1_R3(c, d, e, a, b, W[43]);
  SHA1_R3(b, c, d, e, a, W[44]);
  SHA1_R3(a, b, c, d, e, W[45]);
  SHA1_R3(e, a, b, c, d, W[46]);
  SHA1_R3(d, e, a, b, c, W[47]);
  SHA1_R3(c, d, e, a, b, W[48]);
  SHA1_R3(b, c, d, e, a, W[49]);
  SHA1_R3(a, b, c, d, e, W[50]);
  SHA1_R3(e, a, b, c, d, W[51]);
  SHA1_R3(d, e, a, b, c, W[52]);
  SHA1_R3(c, d, e, a, b, W[53]);
  SHA1_R3(b, c, d, e, a, W[54]);
  SHA1_R3(a, b, c, d, e, W[55]);
  SHA1_R3(e, a, b, c, d, W[56]);
  SHA1_R3(d, e, a, b, c, W[57]);
  SHA1_R3(c, d, e, a, b, W[58]);
  SHA1_R3(b, c, d, e, a, W[59]);

  /*-
   *********************************************************************
   *
   * Do round 4.
   *
   *********************************************************************
   */
  SHA1_R4(a, b, c, d, e, W[60]);
  SHA1_R4(e, a, b, c, d, W[61]);
  SHA1_R4(d, e, a, b, c, W[62]);
  SHA1_R4(c, d, e, a, b, W[63]);
  SHA1_R4(b, c, d, e, a, W[64]);
  SHA1_R4(a, b, c, d, e, W[65]);
  SHA1_R4(e, a, b, c, d, W[66]);
  SHA1_R4(d, e, a, b, c, W[67]);
  SHA1_R4(c, d, e, a, b, W[68]);
  SHA1_R4(b, c, d, e, a, W[69]);
  SHA1_R4(a, b, c, d, e, W[70]);
  SHA1_R4(e, a, b, c, d, W[71]);
  SHA1_R4(d, e, a, b, c, W[72]);
  SHA1_R4(c, d, e, a, b, W[73]);
  SHA1_R4(b, c, d, e, a, W[74]);
  SHA1_R4(a, b, c, d, e, W[75]);
  SHA1_R4(e, a, b, c, d, W[76]);
  SHA1_R4(d, e, a, b, c, W[77]);
  SHA1_R4(c, d, e, a, b, W[78]);
  SHA1_R4(b, c, d, e, a, W[79]);

  /*-
   *********************************************************************
   *
   * Compute intermediate hash value.
   *
   *********************************************************************
   */
  psSHA1Context->A += a;
  psSHA1Context->B += b;
  psSHA1Context->C += c;
  psSHA1Context->D += d;
  psSHA1Context->E += e;
}
