/*-
 ***********************************************************************
 *
 * $Id: ftimes-cat.c,v 1.14 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * FTimesCatMain
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcEncodedName = NULL;
  FTIMES_CAT_HANDLE  *psHandle = NULL;
  int                 i = 0;
  int                 iNRead = 0;
  int                 iNWritten = 0;
  int                 iStdinCount = 0;
  unsigned char       aucData[FTIMES_CAT_READ_SIZE];

#ifdef WINNT
  if (_setmode(_fileno(stdout), _O_BINARY) == -1)
  {
    fprintf(stderr, "%s: Error='Unable to put stdout in binary mode (%s).'\n", PROGRAM_NAME, strerror(errno));
    return XER_Abort;
  }
#endif

  if (iArgumentCount < 2)
  {
    FTimesCatUsage();
  }

  if (strcmp(ppcArgumentVector[1], "-v") == 0 || strcmp(ppcArgumentVector[1], "--version") == 0)
  {
    FTimesCatVersion();
  }

  for (i = 1; i < iArgumentCount; i++)
  {
    pcEncodedName = ppcArgumentVector[i];
    if
    (
      (strcmp(pcEncodedName, "-") == 0 || strcmp(pcEncodedName, "%2D") == 0 || strcmp(pcEncodedName, "%2d") == 0)
    )
    {
      if (++iStdinCount > 1)
      {
        fprintf(stderr, "%s: Error='The \"-\" token (i.e., stdin) may not be specified more than once.'\n", PROGRAM_NAME);
        return XER_Abort;
      }
    }
  }

  for (i = 1; i < iArgumentCount; i++)
  {
    pcEncodedName = ppcArgumentVector[i];
    psHandle = FTimesCatGetHandle(pcEncodedName, acLocalError);
    if (psHandle == NULL)
    {
      fprintf(stderr, "%s: Error='Unable to obtain a handle for \"%s\" (%s).'\n", PROGRAM_NAME, pcEncodedName, acLocalError);
      return XER_Abort;
    }

    while ((iNRead = fread(aucData, 1, FTIMES_CAT_READ_SIZE, psHandle->pFile)) == FTIMES_CAT_READ_SIZE)
    {
      iNWritten = fwrite(aucData, 1, iNRead, stdout);
      if (iNWritten != iNRead)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s).'\n", PROGRAM_NAME, pcEncodedName, strerror(errno));
        return XER_Abort;
      }
    }

    if (iNRead < FTIMES_CAT_READ_SIZE && ferror(psHandle->pFile))
    {
      fprintf(stderr, "%s: Error='Read error while processing \"%s\" (%s).'\n", PROGRAM_NAME, pcEncodedName, strerror(errno));
      return XER_Abort;
    }
    else
    {
      iNWritten = fwrite(aucData, 1, iNRead, stdout);
      if (iNWritten != iNRead)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s).'\n", PROGRAM_NAME, pcEncodedName, strerror(errno));
        return XER_Abort;
      }
    }

    FTimesCatFreeHandle(psHandle);
  }

  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesCatDecodeString
 *
 ***********************************************************************
 */
char *
FTimesCatDecodeString(char *pcEncoded, char *pcError)
{
  char               *pcDecoded = NULL;
  int                 i = 0;
  int                 iLength = strlen(pcEncoded);
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcDecoded = malloc(iLength + 1);
  if (pcDecoded == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return NULL;
  }
  pcDecoded[0] = 0;

  /*-
   *********************************************************************
   *
   * Convert each %HH value to its corresponding character value, and
   * convert plus signs to ' '.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    switch (pcEncoded[i])
    {
    case '%':
      if (isxdigit((int) pcEncoded[i + 1]) && isxdigit((int) pcEncoded[i + 2]))
      {
        char cUpper = tolower(pcEncoded[++i]);
        char cLower = tolower(pcEncoded[++i]);
        unsigned int ui = 0;
        ui =             (((cUpper >= '0') && (cUpper <= '9')) ? (cUpper - '0') : ((cUpper - 'a') + 10));
        ui = (ui << 4) | (((cLower >= '0') && (cLower <= '9')) ? (cLower - '0') : ((cLower - 'a') + 10));
        n += sprintf(&pcDecoded[n], "%c", ui);
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "bad value in string");
        free(pcDecoded);
        return NULL;
      }
      break;
    case '+':
      pcDecoded[n++] = ' ';
      break;
    default:
      pcDecoded[n++] = pcEncoded[i];
      break;
    }
  }
  pcDecoded[n] = 0;

  return pcDecoded;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesCatFormatWinxError
 *
 ***********************************************************************
 */
void
FTimesCatFormatWinxError(DWORD dwError, TCHAR **pptcMessage)
{
  static TCHAR        atcMessage[MESSAGE_SIZE / 2];
  int                 i = 0;
  int                 j = 0;
  LPVOID              lpvMessage = NULL;
  TCHAR              *ptc = NULL;

  /*-
   *********************************************************************
   *
   * Initialize the result buffer/pointer.
   *
   *********************************************************************
   */
  atcMessage[0] = 0;

  *pptcMessage = atcMessage;

  /*-
   *********************************************************************
   *
   * Format the message that corresponds to the specified error code.
   *
   *********************************************************************
   */
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dwError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* default language */
    (LPTSTR) &lpvMessage,
    0,
    NULL
    );

  /*-
   *********************************************************************
   *
   * Replace linefeeds with spaces. Eliminate carriage returns.
   *
   *********************************************************************
   */
  for (i = 0, j = 0, ptc = (TCHAR *) lpvMessage; (i < (int) _tcslen(lpvMessage)) && (i < (MESSAGE_SIZE / 2)); i++, j++)
  {
    if (ptc[i] == _T('\n'))
    {
      atcMessage[j] = _T(' ');
    }
    else if (ptc[i] == _T('\r'))
    {
      j--;
      continue;
    }
    else
    {
      atcMessage[j] = ptc[i];
    }
  }
  atcMessage[j] = 0;

  /*-
   *********************************************************************
   *
   * Removing trailing spaces.
   *
   *********************************************************************
   */
  while (j > 1 && atcMessage[j - 1] == _T(' '))
  {
    atcMessage[--j] = 0;
  }

  LocalFree(lpvMessage);
}
#endif


/*-
 ***********************************************************************
 *
 * FTimesCatFreeHandle
 *
 ***********************************************************************
 */
void
FTimesCatFreeHandle(FTIMES_CAT_HANDLE *psHandle)
{
  if (psHandle)
  {
    if (psHandle->pFile)
    {
      fclose(psHandle->pFile);
    }
#ifdef WINNT
    if (psHandle->iFile != -1)
    {
      _close(psHandle->iFile);
    }
    if (psHandle->pcFileA)
    {
      free(psHandle->pcFileA);
    }
    if (psHandle->pcFileW)
    {
      free(psHandle->pcFileW);
    }
#endif
    if (psHandle->pcDecodedName)
    {
      free(psHandle->pcDecodedName);
    }
    free(psHandle);
  }
}


/*-
 ***********************************************************************
 *
 * FTimesCatGetHandle
 *
 ***********************************************************************
 */
FTIMES_CAT_HANDLE *
FTimesCatGetHandle(char *pcEncodedName, char *pcError)
{
  const char          acRoutine[] = "FTimesCatGetHandle()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_CAT_HANDLE  *psHandle = NULL;
#ifdef WINNT
  int                 iSize = 0;
  TCHAR              *ptcWinxError = NULL;
#endif

  psHandle = (FTIMES_CAT_HANDLE *) calloc(sizeof(FTIMES_CAT_HANDLE), 1);
  if (psHandle == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  psHandle->pcDecodedName = FTimesCatDecodeString(pcEncodedName, acLocalError);
  if (psHandle->pcDecodedName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: FTimesCatDecodeString(): %s", acRoutine, acLocalError);
    goto ABORT;
  }
  if (strcmp(psHandle->pcDecodedName, "-") == 0)
  {
    psHandle->pFile = stdin;
    return psHandle;
  }

#ifdef WINNT
  iSize = strlen(psHandle->pcDecodedName) + strlen(FTIMES_CAT_EXTENDED_PATH_PREFIX) + 1;
  psHandle->pcFileA = (char *) calloc(iSize, 1);
  if (psHandle->pcFileA == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
  snprintf(psHandle->pcFileA, iSize, "%s%s",
    (isalpha((int) psHandle->pcDecodedName[0]) && psHandle->pcDecodedName[1] == ':') ? FTIMES_CAT_EXTENDED_PATH_PREFIX : "",
    psHandle->pcDecodedName
    );
  psHandle->pcFileW = FTimesCatUtf8ToWide(psHandle->pcFileA, iSize, acLocalError);
  if (psHandle->pcFileW == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto ABORT;
  }
  psHandle->hFile = CreateFileW
  (
    psHandle->pcFileW,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
    NULL
  );
  if (psHandle->hFile == INVALID_HANDLE_VALUE)
  {
    FTimesCatFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s: CreateFileW(): %s", acRoutine, ptcWinxError);
    goto ABORT;
  }
  psHandle->iFile = _open_osfhandle((long) psHandle->hFile, 0);
  if (psHandle->iFile == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: _open_osfhandle(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
  psHandle->pFile = fdopen(psHandle->iFile, "rb");
  if (psHandle->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fdopen(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
#else
  psHandle->pFile = fopen(psHandle->pcDecodedName, "rb");
  if (psHandle->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
#endif

  return psHandle;
ABORT:
  FTimesCatFreeHandle(psHandle);
  return NULL;
}


/*-
 ***********************************************************************
 *
 * FTimesCatUsage
 *
 ***********************************************************************
 */
void
FTimesCatUsage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s {file|-} [...]\n", PROGRAM_NAME);
  fprintf(stderr, "       %s {-v|--version}\n", PROGRAM_NAME);
  fprintf(stderr, "\n");
  exit(XER_Usage);
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesCatUtf8ToWide
 *
 ***********************************************************************
 */
wchar_t *
FTimesCatUtf8ToWide(char *pcString, int iUtf8Size, char *pcError)
{
  const char          acRoutine[] = "FTimesCatUtf8ToWide()";
  int                 iWideSize = 0;
  TCHAR              *ptcWinxError = NULL;
  wchar_t            *pwcString = NULL;

  iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, NULL, 0); /* The byte count returned includes the NULL terminator. */
  if (iWideSize)
  {
    pwcString = malloc(iWideSize * sizeof(wchar_t));
    if (pwcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
    iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, pwcString, iWideSize);
    if (!iWideSize)
    {
      FTimesCatFormatWinxError(GetLastError(), &ptcWinxError);
      snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, ptcWinxError);
      free(pwcString);
      return NULL;
    }
  }
  else
  {
    FTimesCatFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, ptcWinxError);
    return NULL;
  }

  return pwcString;
}
#endif


/*-
 ***********************************************************************
 *
 * FTimesCatVersion
 *
 ***********************************************************************
 */
void
FTimesCatVersion(void)
{
  fprintf(stdout, "%s %s %d-bit\n", PROGRAM_NAME, VERSION, (int) (sizeof(&FTimesCatVersion) * 8));
  exit(XER_OK);
}
