/*-
 ***********************************************************************
 *
 * $Id: ftimes-cat.h,v 1.18 2019/03/14 16:07:43 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FTIMES_CAT_H_INCLUDED
#define _FTIMES_CAT_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define PROGRAM_NAME "ftimes-cat"
#define VERSION "1.1.0"

#define XER_OK    0
#define XER_Usage 1
#define XER_Abort 2

#define FTIMES_CAT_ENCODED_PREFIX "file://"
#define FTIMES_CAT_ENCODED_PREFIX_LENGTH 7
#define FTIMES_CAT_EXTENDED_PATH_PREFIX "\\\\?\\"
#define FTIMES_CAT_READ_SIZE 32768

#define MESSAGE_SIZE 1024

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _FTIMES_CAT_HANDLE
{
  char               *pcDecodedName;
  FILE               *pFile;
#ifdef WIN32
  char               *pcFileA;
  HANDLE              hFile;
  int                 iFile;
  wchar_t            *pcFileW;
#endif
} FTIMES_CAT_HANDLE;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
char               *FTimesCatDecodeString(char *pcEncoded, char *pcError);
#ifdef WINNT
void                FTimesCatFormatWinxError(DWORD dwError, TCHAR **pptcMessage);
#endif
void                FTimesCatFreeHandle(FTIMES_CAT_HANDLE *psHandle);
FTIMES_CAT_HANDLE  *FTimesCatGetHandle(char *pcEncodedName, char *pcError);
void                FTimesCatUsage(void);
wchar_t            *FTimesCatUtf8ToWide(char *pcString, int iUtf8Size, char *pcError);
void                FTimesCatVersion(void);

#endif /* !_FTIMES_CAT_H_INCLUDED */
