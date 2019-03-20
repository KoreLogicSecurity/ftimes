/*-
 ***********************************************************************
 *
 * $Id: mask.h,v 1.10 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2007 Klayton Monroe, All Rights Reserved.
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
#define MASK_NAME_SIZE 32

/*--------------------------------*/
#define CMP_NAME        0x00000001
#define CMP_DEV         0x00000002
#define CMP_INODE       0x00000004
#define CMP_VOLUME      0x00000008
#define CMP_FINDEX      0x00000010
#define CMP_MODE        0x00000020
#define CMP_ATTRIBUTES  0x00000040
#define CMP_NLINK       0x00000080
#define CMP_UID         0x00000100
#define CMP_GID         0x00000200
#define CMP_RDEV        0x00000400
#define CMP_ATIME       0x00000800
#define CMP_AMS         0x00001000
#define CMP_MTIME       0x00002000
#define CMP_MMS         0x00004000
#define CMP_CTIME       0x00008000
#define CMP_CMS         0x00010000
#define CMP_CHTIME      0x00020000
#define CMP_CHMS        0x00040000
#define CMP_SIZE        0x00080000
#define CMP_ALTSTREAMS  0x00100000
#define CMP_MD5         0x00200000
#define CMP_SHA1        0x00400000
#define CMP_SHA256      0x00800000
#define CMP_MAGIC       0x01000000
/*--------------------------------*/
#define CMP_ALL_MASK    0x01fffffe /* The name field is excluded because it's a mandatory field. */
#define CMP_HASHES_MASK 0x00e00000
#define CMP_TIMES_MASK  0x0007f800
/*--------------------------------*/

/*--------------------------------*/
#define DIG_ALL_MASK            ~0
/*--------------------------------*/

#ifdef WIN32
/*--------------------------------*/
#define MAP_VOLUME      0x00000001
#define MAP_FINDEX      0x00000002
#define MAP_ATTRIBUTES  0x00000004
#define MAP_ATIME       0x00000008
#define MAP_MTIME       0x00000010
#define MAP_CTIME       0x00000020
#define MAP_CHTIME      0x00000040
#define MAP_SIZE        0x00000080
#define MAP_ALTSTREAMS  0x00000100
#define MAP_MD5         0x00000200
#define MAP_SHA1        0x00000400
#define MAP_SHA256      0x00000800
#define MAP_MAGIC       0x00001000
/*--------------------------------*/
#define MAP_ALL_MASK    0x00001fff
#define MAP_HASHES_MASK 0x00000e00
#define MAP_TIMES_MASK  0x00000078
/*--------------------------------*/
#else
/*--------------------------------*/
#define MAP_DEV         0x00000001
#define MAP_INODE       0x00000002
#define MAP_MODE        0x00000004
#define MAP_NLINK       0x00000008
#define MAP_UID         0x00000010
#define MAP_GID         0x00000020
#define MAP_RDEV        0x00000040
#define MAP_ATIME       0x00000080
#define MAP_MTIME       0x00000100
#define MAP_CTIME       0x00000200
#define MAP_SIZE        0x00000400
#define MAP_MD5         0x00000800
#define MAP_SHA1        0x00001000
#define MAP_SHA256      0x00002000
#define MAP_MAGIC       0x00004000
/*--------------------------------*/
#define MAP_ALL_MASK    0x00007fff
#define MAP_HASHES_MASK 0x00003800
#define MAP_TIMES_MASK  0x00000380
/*--------------------------------*/
#endif

#define MASK_MASK_TYPE_USS 1

#define MASK_RUNMODE_TYPE_CMP 1
#define MASK_RUNMODE_TYPE_DIG 2
#define MASK_RUNMODE_TYPE_MAP 3

#define MASK_TABLE_TYPE_B2S 1

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _MASK_B2S_TABLE /* B2S - Bits 2 Set */
{
  char                acName[MASK_NAME_SIZE];
  int                 iBits2Set;
} MASK_B2S_TABLE;

typedef struct _MASK_USS_MASK /* USS - User Supplied String */
{
  char               *pcMask;
  unsigned long       ulMask;
} MASK_USS_MASK;

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define MASK_BIT_IS_SET(mask, bit) (((mask) & (bit)) == (bit))

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
char               *MaskBuildMask(unsigned long ulMask, int iType, char *pcError);
void                MaskFreeMask(MASK_USS_MASK *psMask);
int                 MaskGetTableLength(int iType);
MASK_B2S_TABLE     *MaskGetTableReference(int iType);
MASK_USS_MASK      *MaskNewMask(char *pcError);
MASK_USS_MASK      *MaskParseMask(char *pcMask, int iType, char *pcError);
int                 MaskSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
int                 MaskSetMask(MASK_USS_MASK *psMask, char *pcMask, char *pcError);
