/*-
 ***********************************************************************
 *
 * $Id: xmagic.h,v 1.11 2004/04/22 02:59:27 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2004 Klayton Monroe, All Rights Reserved.
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
#ifdef UNIX
#define XMAGIC_DEFAULT_LOCATION "/usr/local/integrity/etc/xmagic"
#define XMAGIC_CURRENT_LOCATION "./xmagic"
#endif

#ifdef WIN32
#define XMAGIC_DEFAULT_LOCATION "c:\\integrity\\etc\\xmagic"
#define XMAGIC_CURRENT_LOCATION ".\\xmagic"
#endif

#define XMAGIC_ISEMPTY             "empty"
#define XMAGIC_DEFAULT           "unknown"

#define XMAGIC_LSB                      0
#define XMAGIC_MSB                      1
#define XMAGIC_READ_BUFSIZE        0x4000
#define XMAGIC_MAX_LINE              8192
#define XMAGIC_MAX_LEVEL               10
#define XMAGIC_STRING_BUFSIZE          32
#define XMAGIC_DESCRIPTION_BUFSIZE    128

#define XMAGIC_INDIRECT_OFFSET 0x00000001
#define XMAGIC_RELATIVE_OFFSET 0x00000002
#define XMAGIC_NO_SPACE        0x00000004

#define XMAGIC_BYTE            0x00000001
#define XMAGIC_SHORT           0x00000002
#define XMAGIC_LONG            0x00000004
#define XMAGIC_DATE            0x00000005
#define XMAGIC_STRING          0x00000006
#define XMAGIC_BESHORT         0x00000007
#define XMAGIC_BELONG          0x00000008
#define XMAGIC_BEDATE          0x00000009
#define XMAGIC_LESHORT         0x0000000a
#define XMAGIC_LELONG          0x0000000b
#define XMAGIC_LEDATE          0x0000000c

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _XMAGIC_INDIRECTION
{
  K_UINT32             ui32YOffset;
  K_UINT32             ui32BSL;
} XMAGIC_INDIRECTION;

typedef union _XMAGIC_VALUE
{
  K_UINT32             ui32Number;
  K_UINT08             ui08String[XMAGIC_STRING_BUFSIZE];
} XMAGIC_VALUE;

/*-
 ***********************************************************************
 *
 * The meaning of XMAGIC...
 *
 * psParent           Pointer to parent magic
 * psSibling          Pointer to next magic
 * psChild            Pointer to subordinate magic
 * acDescription      Description to use on a match
 * cOperator          '<', '=', '>', '!', '&', '^', 'x'
 * ui32Flags          XMAGIC_{INDIRECT_OFFSET|RELATIVE_OFFSET|NO_SPACE}
 * ui32Level          Level of magic test (i.e. number of '>')
 * ui32Mask           Mask before comparison with value
 * ui32XOffset        Offset in file being evaluated
 * ui32Type           byte, short, long, date, etc.
 * sIndirection       Indirect type and ui32YOffset x[.[bsl]][+-][y]
 * sValue             A 32 bit number or 32 byte string
 * iStringLength      Length of sValue.ui08String
 *
 ***********************************************************************
 */
typedef struct _XMAGIC
{
  struct _XMAGIC     *psParent;
  struct _XMAGIC     *psSibling;
  struct _XMAGIC     *psChild;
  char                acDescription[XMAGIC_DESCRIPTION_BUFSIZE];
  char                cOperator;
  K_UINT32            ui32Flags;
  K_UINT32            ui32Level;
  K_UINT32            ui32Mask;
  K_UINT32            ui32Type;
  K_UINT32            ui32XOffset;
  XMAGIC_INDIRECTION  sIndirection;
  XMAGIC_VALUE        sValue;
  int                 iStringLength;
} XMAGIC;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 XMagicCompareNumbers(K_UINT32 ui32Value1, K_UINT32 ui32Value2, char cOperator);
int                 XMagicCompareStrings(unsigned char *pucS1, unsigned char *pucS2, int iLength, char cOperator);
int                 XMagicCompareValues(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, K_UINT32 ui32Offset, char *pcDescription, char *pcError);
int                 XMagicConvert2charHex(char *pcSRC, char *pcDST);
int                 XMagicConvert3charOct(char *pcSRC, char *pcDST);
int                 XMagicConvertHexToInt(int iC);
void                XMagicFormatDescription(void *pvValue, XMAGIC *psXMagic, char *pcDescription);
int                 XMagicGetDescription(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetOffset(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetTestOperator(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetTestValue(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetType(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
K_UINT32            XMagicGetValueOffset(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, char *pcError);
int                 XMagicLoadMagic(char *pcFilename, char *pcError);
int                 XMagicParseLine(char *pcLine, XMAGIC *psXMagic, char *pcError);
K_UINT32            XMagicSwapLong(K_UINT32 ui32Value, K_UINT32 ui32MagicType);
K_UINT16            XMagicSwapShort(K_UINT16 ui16Value, K_UINT32 ui32MagicType);
int                 XMagicTestBuffer(unsigned char *pucBuffer, int iBufferLength, char *pcDescription, int iDescriptionLength, char *pcError);
int                 XMagicTestFile(char *pcFilename, char *pcDescription, int iDescriptionLength, char *pcError);
int                 XMagicTestMagic(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, char *pcDescription, int *iBytesUsed, int *iBytesLeft, char *pcError);
int                 XMagicTestSpecial(char *pcFilename, struct stat *psStatEntry, char *pcDescription, int iDescriptionLength, char *pcError);
