/*-
 ***********************************************************************
 *
 * $Id: xmagic.h,v 1.25 2006/04/07 22:15:11 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2006 Klayton Monroe, All Rights Reserved.
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
#define XMAGIC_DEFAULT_LOCATION "/usr/local/ftimes/etc/xmagic"
#define XMAGIC_CURRENT_LOCATION "./xmagic"
#endif

#ifdef WIN32
#define XMAGIC_DEFAULT_LOCATION "c:\\ftimes\\etc\\xmagic"
#define XMAGIC_CURRENT_LOCATION ".\\xmagic"
#endif

#define XMAGIC_ISEMPTY             "empty"
#define XMAGIC_DEFAULT           "unknown"

#define XMAGIC_LSB                      0
#define XMAGIC_MSB                      1
#define XMAGIC_READ_BUFSIZE        0x4000
#define XMAGIC_MAX_LINE              8192
#define XMAGIC_MAX_LEVEL               15
#define XMAGIC_STRING_BUFSIZE          32
#ifdef USE_PCRE
#define XMAGIC_REGEXP_CAPTURE_BUFSIZE 128
#define XMAGIC_REGEXP_BUFSIZE         128
#endif
#define XMAGIC_DESCRIPTION_BUFSIZE    256

#define XMAGIC_INDIRECT_OFFSET 0x00000001
#define XMAGIC_RELATIVE_OFFSET 0x00000002
#define XMAGIC_NO_SPACE        0x00000004
#define XMAGIC_HAVE_MASK       0x00000008
#define XMAGIC_HAVE_SIZE       0x00000010

#ifdef USE_PCRE
#ifndef PCRE_MAX_CAPTURE_COUNT
#define PCRE_MAX_CAPTURE_COUNT 1 /* There must be no more than one capturing '()' subpattern. */
#endif
#endif

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef enum _XMAGIC_OPERATORS
{
  XMAGIC_OP_EQ = 0,
  XMAGIC_OP_NE,
  XMAGIC_OP_LT,
  XMAGIC_OP_LE,
  XMAGIC_OP_GT,
  XMAGIC_OP_GE,
  XMAGIC_OP_AND,
  XMAGIC_OP_XOR,
  XMAGIC_OP_NOOP,
#ifdef USE_PCRE
  XMAGIC_OP_REGEXP_EQ,
  XMAGIC_OP_REGEXP_NE,
#endif
} XMAGIC_OPERATORS;

typedef enum _XMAGIC_TYPES
{
  XMAGIC_BYTE = 1,
  XMAGIC_SHORT,
  XMAGIC_LONG,
  XMAGIC_DATE,
  XMAGIC_STRING,
  XMAGIC_BESHORT,
  XMAGIC_BELONG,
  XMAGIC_BEDATE,
  XMAGIC_LESHORT,
  XMAGIC_LELONG,
  XMAGIC_LEDATE,
#ifdef USE_PCRE
  XMAGIC_REGEXP,
#endif
} XMAGIC_TYPES;

typedef struct _XMAGIC_INDIRECTION
{
  K_INT32              i32YOffset; /* Keep this variable consistent with i32XOffset. */
  K_UINT32             uiType;
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
#ifdef USE_PCRE
 * iOperator          '<', '<=", '=', ('!=' or '!'), '>', '>=', '&', '^', 'x', '=~', '!~'
#else
 * iOperator          '<', '<=", '=', ('!=' or '!'), '>', '>=', '&', '^', 'x'
#endif
 * ui32Flags          XMAGIC_{INDIRECT_OFFSET|RELATIVE_OFFSET|NO_SPACE}
 * ui32Level          Level of magic test (i.e. number of '>')
 * ui32Mask           Mask before comparison with value
 * i32XOffset         Offset in file being evaluated (relative offsets may be negative; absolute offsets may not be negative)
 * ui32Type           byte, short, long, date, etc.
 * sIndirection       Indirect type and i32YOffset (x[.[BSLbsl]][+-][y])
 * sValue             A 32 bit number or 32 byte string
#ifdef USE_PCRE
 * acRegExp           User-specified regular expression
 * aucCapturedData    Captured regular expression data
 * iCaptureCount      Number of capturing ()'s in the regular expression
 * iMatchLength       Length of the captured regular expression match
 * iRegExpLength      Length of acRegExp
 * ui32Size           Size of regex type in bytes
 * psPcre             Compiled regular expression
 * psPcreExtra        Results of studying the compiled regular expression
#endif
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
  int                 iOperator;
  K_UINT32            ui32Flags;
  K_UINT32            ui32Level;
  K_UINT32            ui32Mask;
  K_UINT32            ui32Type;
  K_INT32             i32XOffset; /* This variable can be negative for relative offsets. */
  XMAGIC_INDIRECTION  sIndirection;
  XMAGIC_VALUE        sValue;
  int                 iStringLength;
#ifdef USE_PCRE
  char                acRegExp[XMAGIC_REGEXP_BUFSIZE];
  unsigned char       aucCapturedData[XMAGIC_REGEXP_CAPTURE_BUFSIZE];
  int                 iCaptureCount;
  int                 iMatchLength;
  int                 iRegExpLength;
  K_UINT32            ui32Size;
  pcre               *psPcre;
  pcre_extra         *psPcreExtra;
#endif
} XMAGIC;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 XMagicConvert2charHex(char *pcSRC, char *pcDST);
int                 XMagicConvert3charOct(char *pcSRC, char *pcDST);
int                 XMagicConvertHexToInt(int iC);
void                XMagicFormatDescription(void *pvValue, XMAGIC *psXMagic, char *pcDescription);
void                XMagicFreeXMagic(XMAGIC *psXMagic);
int                 XMagicGetDescription(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetOffset(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetTestOperator(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetTestValue(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
int                 XMagicGetType(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError);
K_INT32             XMagicGetValueOffset(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic);
int                 XMagicLoadMagic(char *pcFilename, char *pcError);
XMAGIC             *XMagicNewXMagic(char *pcError);
int                 XMagicParseLine(char *pcLine, XMAGIC *psXMagic, char *pcError);
K_UINT32            XMagicSwapLong(K_UINT32 ui32Value, K_UINT32 ui32MagicType);
K_UINT16            XMagicSwapShort(K_UINT16 ui16Value, K_UINT32 ui32MagicType);
int                 XMagicTestBuffer(unsigned char *pucBuffer, int iBufferLength, char *pcDescription, int iDescriptionLength, char *pcError);
int                 XMagicTestFile(char *pcFilename, char *pcDescription, int iDescriptionLength, char *pcError);
int                 XMagicTestMagic(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, char *pcDescription, int *iBytesUsed, int *iBytesLeft, char *pcError);
int                 XMagicTestNumber(K_UINT32 ui32Value1, K_UINT32 ui32Value2, int iOperator);
#ifdef USE_PCRE
int                 XMagicTestRegExp(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError);
#endif
#ifdef UNIX
int                 XMagicTestSpecial(char *pcFilename, struct stat *psStatEntry, char *pcDescription, int iDescriptionLength, char *pcError);
#endif
int                 XMagicTestString(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError);
int                 XMagicTestValue(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcDescription, char *pcError);
