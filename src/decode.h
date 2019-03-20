/*-
 ***********************************************************************
 *
 * $Id: decode.h,v 1.2 2003/02/23 17:40:08 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
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
#define DECODE_RESET        0
#define DECODE_DECODE       1
#define DECODE_SEPARATOR_C '|'
#define DECODE_SEPARATOR_S "|"


/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _DECODE_TABLE
{
  char                ZName[32];
  char                UName[32];
  int                 (*Routine) ();
} DECODE_TABLE;


/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 Decode32bFieldBase16To08(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 Decode32bFieldBase16To10(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 Decode32bValueBase16To10(char *pcData, int iLength, K_UINT32 *pui32ValueNew, K_UINT32 *pui32ValueOld, char *pcError);
int                 Decode64bFieldBase16To10(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 Decode64bValueBase16To10(char *pcData, int iLength, K_UINT64 *pvalue_new, K_UINT64 *pvalue_old, char *pcError);
void                DecodeBuildFromBase64Table(void);
int                 DecodeFile(char *pcFilename, FILE *pOutFile, char *pcNewLine, char *pcError);
int                 DecodeFormatOutOfBandTime(char *pcToken, int iLength, char *pcOutput, char *pcError);
#ifdef WIN32
int                 DecodeFormatTime(FILETIME *pFileTime, char *pcTime);
#endif
int                 DecodeGetBase64Hash(char *pcData, unsigned char *pucHash, int iLength, char *pcError);
K_UINT32            DecodeGetRecordsDecoded(void);
K_UINT32            DecodeGetRecordsLost(void);
int                 DecodeLine(char *pcLine, char *pcOutput, char *pcError);
int                 DecodeMagic(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeMd5(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeMilliseconds(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeName(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeTime(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError);
