/*-
 ***********************************************************************
 *
 * $Id: sha256.h,v 1.18 2019/03/14 16:07:42 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2006-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _SHA256_H_INCLUDED
#define _SHA256_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define SHA256_HUNK_SIZE     64 /* (512 bits / 8 bits/byte) */
#define SHA256_HASH_SIZE     32
#define SHA256_READ_SIZE 0x8000

#define SHA256_HA 0x6a09e667
#define SHA256_HB 0xbb67ae85
#define SHA256_HC 0x3c6ef372
#define SHA256_HD 0xa54ff53a
#define SHA256_HE 0x510e527f
#define SHA256_HF 0x9b05688c
#define SHA256_HG 0x1f83d9ab
#define SHA256_HH 0x5be0cd19

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define SHA256_ROTR(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define SHA256_SHR(x,n) ((x)>>(n))

#define SHA256_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA256_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA256_SIGMA0(x) (SHA256_ROTR((x), 2)^SHA256_ROTR((x),13)^SHA256_ROTR((x),22))
#define SHA256_SIGMA1(x) (SHA256_ROTR((x), 6)^SHA256_ROTR((x),11)^SHA256_ROTR((x),25))
#define SHA256_sigma0(x) (SHA256_ROTR((x), 7)^SHA256_ROTR((x),18)^SHA256_SHR((x), 3))
#define SHA256_sigma1(x) (SHA256_ROTR((x),17)^SHA256_ROTR((x),19)^SHA256_SHR((x),10))

#define SHA256_ROUND(a,b,c,d,e,f,g,h,Kt,Wt) \
  { \
    T1 = h + SHA256_SIGMA1(e) + SHA256_CH(e,f,g) + Kt + Wt; \
    T2 = SHA256_SIGMA0(a) + SHA256_MAJ(a,b,c); \
    h = g; \
    g = f; \
    f = e; \
    e = d + T1; \
    d = c; \
    c = b; \
    b = a; \
    a = T1 + T2; \
  }

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _SHA256_CONTEXT
{
  APP_UI32            A;
  APP_UI32            B;
  APP_UI32            C;
  APP_UI32            D;
  APP_UI32            E;
  APP_UI32            F;
  APP_UI32            G;
  APP_UI32            H;
  APP_UI64            ui64MessageLength;
  APP_UI32            ui32ResidueLength;
  unsigned char       aucResidue[SHA256_HUNK_SIZE];
} SHA256_CONTEXT;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                   SHA256HashToBase64(unsigned char *pucHash, char *pcBase64Hash);
int                   SHA256HashToHex(unsigned char *pucHash, char *pcHexHash);
int                   SHA256HashStream(FILE *pFile, unsigned char *pucSHA256, APP_UI64 *pui64Size);
void                  SHA256HashString(unsigned char *pucData, int iLength, unsigned char *pucSHA256);
void                  SHA256HexToHash(char *pcHexHash, unsigned char *pucHash);
void                  SHA256Alpha(SHA256_CONTEXT *psSHA256);
void                  SHA256Cycle(SHA256_CONTEXT *psSHA256, unsigned char *pucData, APP_UI32 ui32Length);
void                  SHA256Omega(SHA256_CONTEXT *psSHA256, unsigned char *pucSHA256);
void                  SHA256Grind(SHA256_CONTEXT *psSHA256, unsigned char *pucData);

#endif /* !_SHA256_H_INCLUDED */
