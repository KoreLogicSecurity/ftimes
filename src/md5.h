/*-
 ***********************************************************************
 *
 * $Id: md5.h,v 1.25 2019/03/14 16:07:42 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2003-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _MD5_H_INCLUDED
#define _MD5_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define MD5_HUNK_SIZE     64 /* (512 bits / 8 bits/byte) */
#define MD5_HASH_SIZE     16
#define MD5_READ_SIZE 0x8000

#define MD5_HA 0x67452301
#define MD5_HB 0xefcdab89
#define MD5_HC 0x98badcfe
#define MD5_HD 0x10325476

#define MD5_F1(X,Y,Z) (((X)&(Y))|((~(X))&(Z)))
#define MD5_F2(X,Y,Z) (((X)&(Z))|((Y)&(~(Z))))
#define MD5_F3(X,Y,Z) ((X)^(Y)^(Z))
#define MD5_F4(X,Y,Z) ((Y)^((X)|(~(Z))))

#define MD5_ROTL(x,n) (((x)<<(n))|(((APP_UI32)(x))>>(32-(n))))

#define MD5_R1(a,b,c,d,M,s,k) {(a)+=MD5_F1((b),(c),(d))+(M)+(k);(a)=(b)+MD5_ROTL((a),(s));}
#define MD5_R2(a,b,c,d,M,s,k) {(a)+=MD5_F2((b),(c),(d))+(M)+(k);(a)=(b)+MD5_ROTL((a),(s));}
#define MD5_R3(a,b,c,d,M,s,k) {(a)+=MD5_F3((b),(c),(d))+(M)+(k);(a)=(b)+MD5_ROTL((a),(s));}
#define MD5_R4(a,b,c,d,M,s,k) {(a)+=MD5_F4((b),(c),(d))+(M)+(k);(a)=(b)+MD5_ROTL((a),(s));}

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _MD5_CONTEXT
{
  APP_UI32            A;
  APP_UI32            B;
  APP_UI32            C;
  APP_UI32            D;
  APP_UI64            ui64MessageLength;
  APP_UI32            ui32ResidueLength;
  unsigned char       aucResidue[MD5_HUNK_SIZE];
} MD5_CONTEXT;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                   MD5HashToBase64(unsigned char *pucHash, char *pcBase64Hash);
int                   MD5HashToHex(unsigned char *pucHash, char *pcHexHash);
int                   MD5HashStream(FILE *pFile, unsigned char *pucMD5, APP_UI64 *pui64Size);
void                  MD5HashString(unsigned char *pucData, int iLength, unsigned char *pucMD5);
void                  MD5HexToHash(char *pcHexHash, unsigned char *pucHash);
void                  MD5Alpha(MD5_CONTEXT *psMD5);
void                  MD5Cycle(MD5_CONTEXT *psMD5, unsigned char *pucData, APP_UI32 ui32Length);
void                  MD5Omega(MD5_CONTEXT *psMD5, unsigned char *pucMD5);
void                  MD5Grind(MD5_CONTEXT *psMD5, unsigned char *pucData);

#endif /* !_MD5_H_INCLUDED */
