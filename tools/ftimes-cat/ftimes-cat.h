/*-
 ***********************************************************************
 *
 * $Id: ftimes-cat.h,v 1.12 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2014 The FTimes Project, All Rights Reserved.
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
#define VERSION "1.0.0"

#define XER_OK    0
#define XER_Usage 1
#define XER_Abort 2

#define FTIMES_CAT_READ_SIZE 32768
#define FTIMES_CAT_EXTENDED_PATH_PREFIX "\\\\?\\"

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
