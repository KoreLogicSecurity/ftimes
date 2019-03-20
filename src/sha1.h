/*-
 ***********************************************************************
 *
 * $Id: sha1.h,v 1.4 2007/02/23 00:22:35 mavrik Exp $
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
#define SHA1_HUNK_SIZE     64 /* (512 bits / 8 bits/byte) */
#define SHA1_HASH_SIZE     20
#define SHA1_READ_SIZE 0x8000

#define SHA1_HA 0x67452301
#define SHA1_HB 0xefcdab89
#define SHA1_HC 0x98badcfe
#define SHA1_HD 0x10325476
#define SHA1_HE 0xc3d2e1f0

#define SHA1_K1 0x5a827999
#define SHA1_K2 0x6ed9eba1
#define SHA1_K3 0x8f1bbcdc
#define SHA1_K4 0xca62c1d6

#define SHA1_F1(X,Y,Z) (((X)&(Y))^((~(X))&(Z)))
#define SHA1_F2(X,Y,Z) ((X)^(Y)^(Z))
#define SHA1_F3(X,Y,Z) (((X)&(Y))^((X)&(Z))^((Y)&(Z)))
#define SHA1_F4(X,Y,Z) ((X)^(Y)^(Z))

#define SHA1_ROTL(x,n) (((x)<<(n))|(((K_UINT32)(x))>>(32-(n))))

#define SHA1_R1(a,b,c,d,e,W) {(e)+=SHA1_ROTL((a),5)+SHA1_F1((b),(c),(d))+SHA1_K1+W;(b)=SHA1_ROTL((b),30);}
#define SHA1_R2(a,b,c,d,e,W) {(e)+=SHA1_ROTL((a),5)+SHA1_F2((b),(c),(d))+SHA1_K2+W;(b)=SHA1_ROTL((b),30);}
#define SHA1_R3(a,b,c,d,e,W) {(e)+=SHA1_ROTL((a),5)+SHA1_F3((b),(c),(d))+SHA1_K3+W;(b)=SHA1_ROTL((b),30);}
#define SHA1_R4(a,b,c,d,e,W) {(e)+=SHA1_ROTL((a),5)+SHA1_F4((b),(c),(d))+SHA1_K4+W;(b)=SHA1_ROTL((b),30);}

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _SHA1_CONTEXT
{
  K_UINT32            A;
  K_UINT32            B;
  K_UINT32            C;
  K_UINT32            D;
  K_UINT32            E;
  K_UINT64            ui64MessageLength;
  K_UINT32            ui32ResidueLength;
  unsigned char       aucResidue[SHA1_HUNK_SIZE];
} SHA1_CONTEXT;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                   SHA1HashToBase64(unsigned char *pucHash, char *pcBase64Hash);
int                   SHA1HashToHex(unsigned char *pucHash, char *pcHexHash);
int                   SHA1HashStream(FILE *pFile, unsigned char *pucSHA1);
void                  SHA1HashString(unsigned char *pucData, int iLength, unsigned char *pucSHA1);
void                  SHA1Alpha(SHA1_CONTEXT *psSHA1);
void                  SHA1Cycle(SHA1_CONTEXT *psSHA1, unsigned char *pucData, K_UINT32 ui32Length);
void                  SHA1Omega(SHA1_CONTEXT *psSHA1, unsigned char *pucSHA1);
void                  SHA1Grind(SHA1_CONTEXT *psSHA1, unsigned char *pucData);
