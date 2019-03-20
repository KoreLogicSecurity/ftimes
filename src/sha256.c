/*-
 ***********************************************************************
 *
 * $Id: sha256.c,v 1.3 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2006-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static unsigned char  gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*-
 ***********************************************************************
 *
 * SHA256HashToBase64
 *
 ***********************************************************************
 */
int
SHA256HashToBase64(unsigned char *pucHash, char *pcBase64Hash)
{
  int                 i = 0;
  int                 n = 0;
  unsigned long       ul = 0;
  unsigned long       ulLeft = 0;

  for (i = 0; i < SHA256_HASH_SIZE; i++)
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
 * SHA256HashToHex
 *
 ***********************************************************************
 */
int
SHA256HashToHex(unsigned char *pucHash, char *pcHexHash)
{
  int                 n = 0;
  unsigned int       *pui = (unsigned int *) pucHash;

  n += snprintf(&pcHexHash[n], (SHA256_HASH_SIZE * 2 + 1), "%08x%08x%08x%08x%08x%08x%08x%08x",
    (unsigned int) htonl(*(pui    )),
    (unsigned int) htonl(*(pui + 1)),
    (unsigned int) htonl(*(pui + 2)),
    (unsigned int) htonl(*(pui + 3)),
    (unsigned int) htonl(*(pui + 4)),
    (unsigned int) htonl(*(pui + 5)),
    (unsigned int) htonl(*(pui + 6)),
    (unsigned int) htonl(*(pui + 7))
    );
  pcHexHash[n] = 0;

  return n;
}

/*-
 ***********************************************************************
 *
 * SHA256HashStream
 *
 ***********************************************************************
 */
int
SHA256HashStream(FILE *pFile, unsigned char *pucSHA256)
{
  unsigned char       aucData[SHA256_READ_SIZE];
#ifdef SHA256_PRE_MEMSET_MEMCPY
  int                 i;
#endif
  int                 iNRead;
  SHA256_CONTEXT        sSHA256Context;

  SHA256Alpha(&sSHA256Context);
  while ((iNRead = fread(aucData, 1, SHA256_READ_SIZE, pFile)) == SHA256_READ_SIZE)
  {
    SHA256Cycle(&sSHA256Context, aucData, iNRead);
  }
  if (ferror(pFile))
  {
#ifdef SHA256_PRE_MEMSET_MEMCPY
    for (i = 0; i < SHA256_HASH_SIZE; i++)
    {
      pucSHA256[i] = 0;
    }
#else
    memset(pucSHA256, 0, SHA256_HASH_SIZE);
#endif
    return -1;
  }
  SHA256Cycle(&sSHA256Context, aucData, iNRead);
  SHA256Omega(&sSHA256Context, pucSHA256);
  return 0;
}


/*-
 ***********************************************************************
 *
 * SHA256HashString
 *
 ***********************************************************************
 */
void
SHA256HashString(unsigned char *pucData, int iLength, unsigned char *pucSHA256)
{
  SHA256_CONTEXT        sSHA256Context;

  SHA256Alpha(&sSHA256Context);
  while (iLength > SHA256_HUNK_SIZE)
  {
    SHA256Cycle(&sSHA256Context, pucData, SHA256_HUNK_SIZE);
    pucData += SHA256_HUNK_SIZE;
    iLength -= SHA256_HUNK_SIZE;
  }
  SHA256Cycle(&sSHA256Context, pucData, iLength);
  SHA256Omega(&sSHA256Context, pucSHA256);
}


/*-
 ***********************************************************************
 *
 * SHA256Alpha
 *
 ***********************************************************************
 */
void
SHA256Alpha(SHA256_CONTEXT *psSHA256Context)
{
#ifdef SHA256_PRE_MEMSET_MEMCPY
  int                 i;
#endif

  psSHA256Context->A = SHA256_HA;
  psSHA256Context->B = SHA256_HB;
  psSHA256Context->C = SHA256_HC;
  psSHA256Context->D = SHA256_HD;
  psSHA256Context->E = SHA256_HE;
  psSHA256Context->F = SHA256_HF;
  psSHA256Context->G = SHA256_HG;
  psSHA256Context->H = SHA256_HH;
  psSHA256Context->ui64MessageLength = 0;
  psSHA256Context->ui32ResidueLength = 0;
#ifdef SHA256_PRE_MEMSET_MEMCPY
  for (i = 0; i < SHA256_HUNK_SIZE; i++)
  {
    psSHA256Context->aucResidue[i] = 0;
  }
#else
  memset(psSHA256Context->aucResidue, 0, SHA256_HUNK_SIZE);
#endif
}


/*-
 ***********************************************************************
 *
 * SHA256Cycle
 *
 ***********************************************************************
 */
void
SHA256Cycle(SHA256_CONTEXT *psSHA256Context, unsigned char *pucData, K_UINT32 ui32Length)
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
  psSHA256Context->ui64MessageLength += (K_UINT64) ui32Length;

  /*-
   *********************************************************************
   *
   * If residue plus new is less than SHA256_HUNK_SIZE, just store new.
   *
   *********************************************************************
   */
  if ((psSHA256Context->ui32ResidueLength + ui32Length) < SHA256_HUNK_SIZE)
  {
    pucTemp = &psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength];
    psSHA256Context->ui32ResidueLength += ui32Length;
#ifdef SHA256_PRE_MEMSET_MEMCPY
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
  if (psSHA256Context->ui32ResidueLength > 0)
  {
#ifdef SHA256_PRE_MEMSET_MEMCPY
    pucTemp = &psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength];
    ui32Length -= SHA256_HUNK_SIZE - psSHA256Context->ui32ResidueLength;
    ui32 = psSHA256Context->ui32ResidueLength;
    while (ui32++ < SHA256_HUNK_SIZE)
    {
      *pucTemp++ = *pucData++;
    }
#else
    ui32 = SHA256_HUNK_SIZE - psSHA256Context->ui32ResidueLength; /* Note: This ui32 holds a different value than the one in the SHA256_PRE_MEMSET_MEMCPY code. */
    ui32Length -= ui32;
    memcpy(&psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength], pucData, ui32);
    pucData += ui32;
#endif
    SHA256Grind(psSHA256Context, psSHA256Context->aucResidue);
    psSHA256Context->ui32ResidueLength = 0;
  }

  /*-
   *********************************************************************
   *
   * Process new in SHA256_HUNK_SIZE chunks and store any residue.
   *
   *********************************************************************
   */
  while (ui32Length >= SHA256_HUNK_SIZE)
  {
    SHA256Grind(psSHA256Context, pucData);
    ui32Length -= SHA256_HUNK_SIZE;
    pucData += SHA256_HUNK_SIZE;
  }
  pucTemp = psSHA256Context->aucResidue;
  psSHA256Context->ui32ResidueLength = ui32Length;
#ifdef SHA256_PRE_MEMSET_MEMCPY
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
 * SHA256Omega
 *
 ***********************************************************************
 */
void
SHA256Omega(SHA256_CONTEXT *psSHA256Context, unsigned char *pucSHA256)
{
  /*-
   *********************************************************************
   *
   * Append padding, and grind if necessary.
   *
   *********************************************************************
   */
  psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength] = 0x80;
  if (psSHA256Context->ui32ResidueLength++ >= (SHA256_HUNK_SIZE - 8))
  {
#ifdef SHA256_PRE_MEMSET_MEMCPY
    while (psSHA256Context->ui32ResidueLength < SHA256_HUNK_SIZE)
    {
      psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength++] = 0;
    }
#else
    memset(&psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength], 0, SHA256_HUNK_SIZE - psSHA256Context->ui32ResidueLength);
#endif
    SHA256Grind(psSHA256Context, psSHA256Context->aucResidue);
    psSHA256Context->ui32ResidueLength = 0;
  }
#ifdef SHA256_PRE_MEMSET_MEMCPY
  while (psSHA256Context->ui32ResidueLength < (SHA256_HUNK_SIZE - 8))
  {
    psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength++] = 0;
  }
#else
  memset(&psSHA256Context->aucResidue[psSHA256Context->ui32ResidueLength], 0, (SHA256_HUNK_SIZE - 8) - psSHA256Context->ui32ResidueLength);
#endif

  /*-
   *********************************************************************
   *
   * Append length in bits (big-endian), and grind one last time.
   *
   *********************************************************************
   */
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-8] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 53) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-7] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 45) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-6] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 37) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-5] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 29) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-4] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 21) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-3] = (unsigned char) ((psSHA256Context->ui64MessageLength >> 13) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-2] = (unsigned char) ((psSHA256Context->ui64MessageLength >>  5) & 0xff);
  psSHA256Context->aucResidue[SHA256_HUNK_SIZE-1] = (unsigned char) ((psSHA256Context->ui64MessageLength <<  3) & 0xff);

  SHA256Grind(psSHA256Context, psSHA256Context->aucResidue);

  /*-
   *********************************************************************
   *
   * Transfer hash (big-endian) to user-supplied array.
   *
   *********************************************************************
   */
  pucSHA256[ 0] = (unsigned char) (((psSHA256Context->A) >> 24) & 0xff);
  pucSHA256[ 1] = (unsigned char) (((psSHA256Context->A) >> 16) & 0xff);
  pucSHA256[ 2] = (unsigned char) (((psSHA256Context->A) >>  8) & 0xff);
  pucSHA256[ 3] = (unsigned char) ( (psSHA256Context->A)        & 0xff);
  pucSHA256[ 4] = (unsigned char) (((psSHA256Context->B) >> 24) & 0xff);
  pucSHA256[ 5] = (unsigned char) (((psSHA256Context->B) >> 16) & 0xff);
  pucSHA256[ 6] = (unsigned char) (((psSHA256Context->B) >>  8) & 0xff);
  pucSHA256[ 7] = (unsigned char) ( (psSHA256Context->B)        & 0xff);
  pucSHA256[ 8] = (unsigned char) (((psSHA256Context->C) >> 24) & 0xff);
  pucSHA256[ 9] = (unsigned char) (((psSHA256Context->C) >> 16) & 0xff);
  pucSHA256[10] = (unsigned char) (((psSHA256Context->C) >>  8) & 0xff);
  pucSHA256[11] = (unsigned char) ( (psSHA256Context->C)        & 0xff);
  pucSHA256[12] = (unsigned char) (((psSHA256Context->D) >> 24) & 0xff);
  pucSHA256[13] = (unsigned char) (((psSHA256Context->D) >> 16) & 0xff);
  pucSHA256[14] = (unsigned char) (((psSHA256Context->D) >>  8) & 0xff);
  pucSHA256[15] = (unsigned char) ( (psSHA256Context->D)        & 0xff);
  pucSHA256[16] = (unsigned char) (((psSHA256Context->E) >> 24) & 0xff);
  pucSHA256[17] = (unsigned char) (((psSHA256Context->E) >> 16) & 0xff);
  pucSHA256[18] = (unsigned char) (((psSHA256Context->E) >>  8) & 0xff);
  pucSHA256[19] = (unsigned char) ( (psSHA256Context->E)        & 0xff);
  pucSHA256[20] = (unsigned char) (((psSHA256Context->F) >> 24) & 0xff);
  pucSHA256[21] = (unsigned char) (((psSHA256Context->F) >> 16) & 0xff);
  pucSHA256[22] = (unsigned char) (((psSHA256Context->F) >>  8) & 0xff);
  pucSHA256[23] = (unsigned char) ( (psSHA256Context->F)        & 0xff);
  pucSHA256[24] = (unsigned char) (((psSHA256Context->G) >> 24) & 0xff);
  pucSHA256[25] = (unsigned char) (((psSHA256Context->G) >> 16) & 0xff);
  pucSHA256[26] = (unsigned char) (((psSHA256Context->G) >>  8) & 0xff);
  pucSHA256[27] = (unsigned char) ( (psSHA256Context->G)        & 0xff);
  pucSHA256[28] = (unsigned char) (((psSHA256Context->H) >> 24) & 0xff);
  pucSHA256[29] = (unsigned char) (((psSHA256Context->H) >> 16) & 0xff);
  pucSHA256[30] = (unsigned char) (((psSHA256Context->H) >>  8) & 0xff);
  pucSHA256[31] = (unsigned char) ( (psSHA256Context->H)        & 0xff);
}


/*-
 ***********************************************************************
 *
 * SHA256Grind
 *
 ***********************************************************************
 */
void
SHA256Grind(SHA256_CONTEXT *psSHA256Context, unsigned char *pucData)
{
  K_UINT32            a;
  K_UINT32            b;
  K_UINT32            c;
  K_UINT32            d;
  K_UINT32            e;
  K_UINT32            f;
  K_UINT32            g;
  K_UINT32            h;
  K_UINT32            W[64];
  K_UINT32            T1;
  K_UINT32            T2;
  int                 t;

  /*-
   *********************************************************************
   *
   * Prepare the message schedule (big-endian).
   *
   *********************************************************************
   */
  for (t = 0; t < 16; t++)
  {
    W[t]  = (*pucData++) << 24;
    W[t] |= (*pucData++) << 16;
    W[t] |= (*pucData++) <<  8;
    W[t] |= (*pucData++)      ;
  }
  for (t = 16; t < 64; t++)
  {
    W[t] = SHA256_sigma1(W[t - 2]) + W[t - 7] + SHA256_sigma0(W[t - 15]) + W[t - 16];
  }

  /*-
   *********************************************************************
   *
   * Initialize working variables.
   *
   *********************************************************************
   */
  a = psSHA256Context->A;
  b = psSHA256Context->B;
  c = psSHA256Context->C;
  d = psSHA256Context->D;
  e = psSHA256Context->E;
  f = psSHA256Context->F;
  g = psSHA256Context->G;
  h = psSHA256Context->H;

  /*-
   *********************************************************************
   *
   * Do rounds.
   *
   *********************************************************************
   */
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x428a2f98, W[ 0]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x71374491, W[ 1]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xb5c0fbcf, W[ 2]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xe9b5dba5, W[ 3]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x3956c25b, W[ 4]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x59f111f1, W[ 5]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x923f82a4, W[ 6]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xab1c5ed5, W[ 7]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xd807aa98, W[ 8]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x12835b01, W[ 9]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x243185be, W[10]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x550c7dc3, W[11]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x72be5d74, W[12]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x80deb1fe, W[13]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x9bdc06a7, W[14]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xc19bf174, W[15]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xe49b69c1, W[16]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xefbe4786, W[17]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x0fc19dc6, W[18]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x240ca1cc, W[19]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x2de92c6f, W[20]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x4a7484aa, W[21]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x5cb0a9dc, W[22]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x76f988da, W[23]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x983e5152, W[24]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xa831c66d, W[25]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xb00327c8, W[26]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xbf597fc7, W[27]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xc6e00bf3, W[28]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xd5a79147, W[29]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x06ca6351, W[30]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x14292967, W[31]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x27b70a85, W[32]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x2e1b2138, W[33]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x4d2c6dfc, W[34]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x53380d13, W[35]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x650a7354, W[36]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x766a0abb, W[37]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x81c2c92e, W[38]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x92722c85, W[39]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xa2bfe8a1, W[40]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xa81a664b, W[41]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xc24b8b70, W[42]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xc76c51a3, W[43]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xd192e819, W[44]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xd6990624, W[45]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xf40e3585, W[46]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x106aa070, W[47]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x19a4c116, W[48]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x1e376c08, W[49]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x2748774c, W[50]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x34b0bcb5, W[51]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x391c0cb3, W[52]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x4ed8aa4a, W[53]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x5b9cca4f, W[54]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x682e6ff3, W[55]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x748f82ee, W[56]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x78a5636f, W[57]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x84c87814, W[58]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x8cc70208, W[59]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0x90befffa, W[60]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xa4506ceb, W[61]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xbef9a3f7, W[62]);
  SHA256_ROUND(a, b, c, d, e, f, g, h, 0xc67178f2, W[63]);

  /*-
   *********************************************************************
   *
   * Compute intermediate hash value.
   *
   *********************************************************************
   */
  psSHA256Context->A += a;
  psSHA256Context->B += b;
  psSHA256Context->C += c;
  psSHA256Context->D += d;
  psSHA256Context->E += e;
  psSHA256Context->F += f;
  psSHA256Context->G += g;
  psSHA256Context->H += h;
}
