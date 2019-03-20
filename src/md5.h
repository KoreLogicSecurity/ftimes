/*-
 ***********************************************************************
 *
 * $Id: md5.h,v 1.10 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2003-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */

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

#define MD5_ROTL(x,n) (((x)<<(n))|(((K_UINT32)(x))>>(32-(n))))

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
  K_UINT32            A;
  K_UINT32            B;
  K_UINT32            C;
  K_UINT32            D;
  K_UINT64            ui64MessageLength;
  K_UINT32            ui32ResidueLength;
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
int                   MD5HashStream(FILE *pFile, unsigned char *pucMD5);
void                  MD5HashString(unsigned char *pucData, int iLength, unsigned char *pucMD5);
void                  MD5Alpha(MD5_CONTEXT *psMD5);
void                  MD5Cycle(MD5_CONTEXT *psMD5, unsigned char *pucData, K_UINT32 ui32Length);
void                  MD5Omega(MD5_CONTEXT *psMD5, unsigned char *pucMD5);
void                  MD5Grind(MD5_CONTEXT *psMD5, unsigned char *pucData);
