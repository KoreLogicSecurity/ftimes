/*-
 ***********************************************************************
 *
 * $Id: ftimes-xpa.c,v 1.14 2019/04/23 13:08:58 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Global Variables.
 *
 ***********************************************************************
 */
FTIMES_XPA_PROPERTIES *gpsProperties = NULL;

/*-
 ***********************************************************************
 *
 * FTimesXpaMain
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  char                acLine[PROPERTIES_MAX_LINE];
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pc = NULL;
  char               *pcDecodedName = NULL;
  char               *pcTargetFile = NULL;
  FILE               *pListFile = NULL;
  FTIMES_XPA_PROPERTIES *psProperties = NULL;
  int                 iDecoded = 0;
  int                 iError = 0;
  int                 iLineNumber = 0;
  int                 iLineLength = 0;
  int                 iSaveLength = 0;

  /*-
   *********************************************************************
   *
   * Punch in and go to work.
   *
   *********************************************************************
   */
  iError = FTimesXpaBootStrap(acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Unable to bootstrap program (%s).\n", PROGRAM_NAME, acLocalError);
    return XER_BootStrap;
  }
  psProperties = FTimesXpaGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Process command line arguments.
   *
   *********************************************************************
   */
  iError = FTimesXpaProcessArguments(iArgumentCount, ppcArgumentVector, psProperties, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Unable to process arguments (%s).\n", PROGRAM_NAME, acLocalError);
    return XER_ProcessArguments;
  }

  /*-
   *********************************************************************
   *
   * Do final setup.
   *
   *********************************************************************
   */
  psProperties->pucData = (unsigned char *) malloc(psProperties->ui32Blocksize);
  if (psProperties->pucData == NULL)
  {
    fprintf(stderr, "%s: Error='Unable to allocate chunk buffer (%s).'\n", PROGRAM_NAME, strerror(errno));
    return XER_Abort;
  }

  psProperties->pucData = (unsigned char *) malloc(psProperties->ui32Blocksize);
  if (psProperties->pucData == NULL)
  {
    fprintf(stderr, "%s: Error='Unable to allocate chunk buffer (%s).'\n", PROGRAM_NAME, strerror(errno));
    return XER_Abort;
  }

  /*-
   *********************************************************************
   *
   * Conditionally open and process the list file, which may be stdin.
   *
   *********************************************************************
   */
  if (psProperties->pcListFile)
  {
    if (strcmp(psProperties->pcListFile, "-") == 0)
    {
      pListFile = stdin;
    }
    else
    {
      pListFile = fopen(psProperties->pcListFile, "rb");
      if (pListFile == NULL)
      {
        fprintf(stderr, "%s: Error='Unable to open \"%s\" (%s).'\n", PROGRAM_NAME, psProperties->pcListFile, strerror(errno));
        return XER_Abort;
      }
    }

    for (acLine[0] = 0, iLineNumber = 1; fgets(acLine, PROPERTIES_MAX_LINE, pListFile) != NULL; acLine[0] = 0, iLineNumber++)
    {
      /*-
       *****************************************************************
       *
       * Conditionally free up previously allocated resources.
       *
       *****************************************************************
       */
      if (iDecoded && pcDecodedName != NULL)
      {
        free(pcDecodedName);
      }
      iDecoded = 0;

      /*-
       *****************************************************************
       *
       * Remove EOL characters.
       *
       *****************************************************************
       */
      iLineLength = iSaveLength = strlen(acLine);
      while (iLineLength > 0 && ((acLine[iLineLength - 1] == '\r') || (acLine[iLineLength - 1] == '\n')))
      {
        iLineLength--;
      }
      if (iLineLength == iSaveLength)
      {
        fprintf(stderr, "%s: Error='Line %d is too long or is not properly terminated.'\n", PROGRAM_NAME, iLineNumber);
        return XER_Abort;
      }
      acLine[iLineLength] = 0;

      /*-
       *****************************************************************
       *
       * Look for the first embedded comment and truncate the line.
       *
       *****************************************************************
       */
      if ((pc = strstr(acLine, PROPERTIES_COMMENT_S)) != NULL)
      {
        *pc = 0;
        iLineLength = pc - acLine;
      }

      /*-
       *****************************************************************
       *
       * Look for a field delimiter and truncate the line.
       *
       *****************************************************************
       */
      if ((pc = strstr(acLine, PROPERTIES_DELIMITER_S)) != NULL)
      {
        *pc = 0;
        iLineLength = pc - acLine;
      }

      /*-
       *****************************************************************
       *
       * Burn any trailing white space off line.
       *
       *****************************************************************
       */
      while (iLineLength > 0 && isspace((int) acLine[iLineLength - 1]))
      {
        acLine[iLineLength--] = 0;
      }

      /*-
       *****************************************************************
       *
       * Ignore header and blank lines.
       *
       *****************************************************************
       */
      if (strcmp(acLine, "name") == 0 || iLineLength == 0)
      {
        continue;
      }

      /*-
       *****************************************************************
       *
       * Make sure the filename is properly quoted. If it is, drop the
       * quotes and decode the filename.
       *
       *****************************************************************
       */
      if (!(iLineLength > 2 && acLine[0] == '"' && acLine[iLineLength - 1] == '"'))
      {
        fprintf(stderr, "%s: Error='Line %d has an incorrectly formated name field.'\n", PROGRAM_NAME, iLineNumber);
        continue;
      }
      acLine[iLineLength - 1] = 0;
      iLineLength -= 2;
      pcDecodedName = FTimesXpaDecodeString(&acLine[1], acLocalError);
      if (pcDecodedName == NULL)
      {
        fprintf(stderr, "%s: Error='Unable to decode \"%s\" on line %d (%s).'\n", PROGRAM_NAME, &acLine[1], iLineNumber, acLocalError);
        continue;
      }
      iDecoded = 1;

      /*-
       *****************************************************************
       *
       * Process the file.
       *
       *****************************************************************
       */
      FTimesXpaAddMember(pcDecodedName, psProperties->pucData, psProperties->ui32Blocksize, acLocalError);
    }
    if (ferror(pListFile))
    {
      fprintf(stderr, "%s: Error='Unable to read \"%s\" (%s).'\n", PROGRAM_NAME, psProperties->pcListFile, strerror(errno));
      return XER_Abort;
    }
    fclose(pListFile);
  }

  /*-
   *********************************************************************
   *
   * Process any files listed on the command line.
   *
   *********************************************************************
   */
  while ((pcTargetFile = OptionsGetNextOperand(psProperties->psOptionsContext)) != NULL)
  {
    /*-
     *******************************************************************
     *
     * Conditionally free up previously allocated resources.
     *
     *******************************************************************
     */
    if (iDecoded && pcDecodedName != NULL)
    {
      free(pcDecodedName);
    }
    iDecoded = 0;

    /*-
     *******************************************************************
     *
     * Conditionally decode the filename.
     *
     *******************************************************************
     */
    if (strncmp(pcTargetFile, FTIMES_XPA_ENCODED_PREFIX, FTIMES_XPA_ENCODED_PREFIX_LENGTH) == 0)
    {
      pcDecodedName = FTimesXpaDecodeString(&pcTargetFile[FTIMES_XPA_ENCODED_PREFIX_LENGTH], acLocalError);
      if (pcDecodedName == NULL)
      {
        fprintf(stderr, "%s: Error='Unable to decode \"%s\" (%s).'\n", PROGRAM_NAME, pcTargetFile, acLocalError);
        continue;
      }
      iDecoded = 1;
    }
    else
    {
      pcDecodedName = pcTargetFile;
    }

    /*-
     *******************************************************************
     *
     * Process the file.
     *
     *******************************************************************
     */
    FTimesXpaAddMember(pcDecodedName, psProperties->pucData, psProperties->ui32Blocksize, acLocalError);
  }

  /*-
   *********************************************************************
   *
   * Clean up and go home.
   *
   *********************************************************************
   */
  FTimesXpaFreeProperties(psProperties);

  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaAbort
 *
 ***********************************************************************
 */
void
FTimesXpaAbort()
{
  exit(XER_Abort);
}


/*-
 ***********************************************************************
 *
 * FTimesXpaDecodeString
 *
 ***********************************************************************
 */
char *
FTimesXpaDecodeString(char *pcEncoded, char *pcError)
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
        snprintf(pcError, MESSAGE_SIZE, "Bad hex value in string");
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


/*-
 ***********************************************************************
 *
 * FTimesXpaEncodeString
 *
 ***********************************************************************
 */
char *
FTimesXpaEncodeString(char *pcData, int iLength, char *pcError)
{
  char               *pcEncoded = NULL;
  int                 i = 0;
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcEncoded = malloc((3 * iLength) + 1);
  if (pcEncoded == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return NULL;
  }
  pcEncoded[0] = 0;

  /*-
   *********************************************************************
   *
   * Encode non-printables and [|"'`%+#]. Conditionally encode '/' and
   * '\' depending on the target platform. Convert spaces to '+'. Avoid
   * isprint() here because it has led to unexpected results on Windows
   * platforms. In the past, isprint() on certain Windows systems has
   * decided that several characters in the range 0x7f - 0xff are
   * printable.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    if (pcData[i] > '~' || pcData[i] < ' ')
    {
      n += sprintf(&pcEncoded[n], "%%%02x", (unsigned char) pcData[i]);
    }
    else
    {
      switch (pcData[i])
      {
      case '|':
      case '"':
      case '\'':
      case '`':
      case '%':
      case '+':
      case '#':
#ifdef WINNT
      case '/':
#else
      case '\\':
#endif
        n += sprintf(&pcEncoded[n], "%%%02x", (unsigned char) pcData[i]);
        break;
      case ' ':
        pcEncoded[n++] = '+';
        break;
      default:
        pcEncoded[n++] = pcData[i];
        break;
      }
    }
  }
  pcEncoded[n] = 0;

  return pcEncoded;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesXpaFormatWinxError
 *
 ***********************************************************************
 */
void
FTimesXpaFormatWinxError(DWORD dwError, TCHAR **pptcMessage)
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
 * FTimesXpaFreeHandle
 *
 ***********************************************************************
 */
void
FTimesXpaFreeHandle(FTIMES_XPA_HANDLE *psHandle)
{
  if (psHandle)
  {
//FIXME What is the right way on WINX platforms to close a file handle
//      that trasitioned from hFile via CreateFileW() to iFile via
//      _open_osfhandle() to pFile via _fdopen()? When compiled using
//      Visual Studio 2005, the _close() was causing assertion
//      exceptions -- probably because the fclose() closed the
//      underlying file descriptor along with the file pointer, so the
//      _close() was attempting to close something that had already
//      been closed.
    if (psHandle->pFile)
    {
      fclose(psHandle->pFile);
    }
#ifdef WINNT
//FIXME See comment above.
//  if (psHandle->iFile != -1)
//  {
//    _close(psHandle->iFile);
//  }
    if (psHandle->pcFileA)
    {
      free(psHandle->pcFileA);
    }
    if (psHandle->pcFileW)
    {
      free(psHandle->pcFileW);
    }
#endif
    free(psHandle);
  }
}


/*-
 ***********************************************************************
 *
 * FTimesXpaGetHandle
 *
 ***********************************************************************
 */
FTIMES_XPA_HANDLE *
FTimesXpaGetHandle(char *pcDecodedName, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaGetHandle()";
#ifdef WINNT
  char                acLocalError[MESSAGE_SIZE] = "";
#endif
  FTIMES_XPA_HANDLE  *psHandle = NULL;
#ifdef WINNT
  int                 iSize = 0;
  TCHAR              *ptcWinxError = NULL;
#endif

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new handle structure.
   *
   *********************************************************************
   */
  psHandle = (FTIMES_XPA_HANDLE *) calloc(sizeof(FTIMES_XPA_HANDLE), 1);
  if (psHandle == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  psHandle->pcDecodedName = pcDecodedName;

  /*-
   *********************************************************************
   *
   * Open the specified file for binary read access. If the filename is
   * "-", use stdin.
   *
   *********************************************************************
   */
  if (strcmp(psHandle->pcDecodedName, "-") == 0)
  {
    psHandle->pFile = stdin;
    return psHandle;
  }

#ifdef WINNT
  iSize = strlen(psHandle->pcDecodedName) + strlen(FTIMES_XPA_EXTENDED_PATH_PREFIX) + 1;
  psHandle->pcFileA = (char *) calloc(iSize, 1);
  if (psHandle->pcFileA == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
  snprintf(psHandle->pcFileA, iSize, "%s%s",
    (isalpha((int) psHandle->pcDecodedName[0]) && psHandle->pcDecodedName[1] == ':') ? FTIMES_XPA_EXTENDED_PATH_PREFIX : "",
    psHandle->pcDecodedName
    );
  psHandle->pcFileW = FTimesXpaUtf8ToWide(psHandle->pcFileA, iSize, acLocalError);
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
    FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s: CreateFileW(): %s", acRoutine, ptcWinxError);
    goto ABORT;
  }
  psHandle->iFile = _open_osfhandle((intptr_t) psHandle->hFile, 0);
  if (psHandle->iFile == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: _open_osfhandle(): %s", acRoutine, strerror(errno));
    goto ABORT;
  }
  psHandle->pFile = _fdopen(psHandle->iFile, "rb");
  if (psHandle->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: _fdopen(): %s", acRoutine, strerror(errno));
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
  FTimesXpaFreeHandle(psHandle);
  return NULL;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaUsage
 *
 ***********************************************************************
 */
void
FTimesXpaUsage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s {-a|--archive} [{-b|--blocksize} bytes] [{-l|--list} {file|-}] -- [target [...]]\n", PROGRAM_NAME);
  fprintf(stderr, "       %s {-v|--version}\n", PROGRAM_NAME);
  fprintf(stderr, "\n");
  exit(XER_Usage);
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesXpaUtf8ToWide
 *
 ***********************************************************************
 */
wchar_t *
FTimesXpaUtf8ToWide(char *pcString, int iUtf8Size, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaUtf8ToWide()";
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
      FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
      snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, ptcWinxError);
      free(pwcString);
      return NULL;
    }
  }
  else
  {
    FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, ptcWinxError);
    return NULL;
  }

  return pwcString;
}
#endif


/*-
 ***********************************************************************
 *
 * FTimesXpaVersion
 *
 ***********************************************************************
 */
void
FTimesXpaVersion(void)
{
  fprintf(stdout, "%s %s %d-bit\n", PROGRAM_NAME, VERSION, (int) (sizeof(&FTimesXpaVersion) * 8));
  exit(XER_OK);
}


/*-
 ***********************************************************************
 *
 * FTimesXpaWriteData
 *
 ***********************************************************************
 */
int
FTimesXpaWriteData(FILE *pFile, unsigned char *pucData, int iLength, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaWriteData()";
  int                 iNWritten = 0;

  iNWritten = fwrite(pucData, 1, iLength, pFile);
  if (ferror(pFile))
  {
    if (iNWritten != iLength)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): NWritten = [%d] != [%d]: WriteLength mismatch!: %s",
        acRoutine,
        iNWritten,
        iLength,
        (errno == 0) ? "unexpected error -- check device for sufficient space" : strerror(errno)
        );
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): %s",
        acRoutine,
        (errno == 0) ? "unexpected error -- check device for sufficient space" : strerror(errno)
        );
    }
    return ER;
  }
  if (fflush(pFile) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fflush(): %s", acRoutine, strerror(errno));
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaWriteHeader
 *
 ***********************************************************************
 */
int
FTimesXpaWriteHeader(FILE *pFile, FTIMES_XPA_HEADER *psFileHeader, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaWriteHeader()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  int                 i = 0;
  int                 iIndex = 0;
  unsigned char       aucHeader[FTIMES_XPA_HEADER_SIZE];

  /*-
   *********************************************************************
   *
   * Initialize the header buffer in an endian-agnostic way.
   *
   *********************************************************************
   */
  iIndex = 0;
  for (i = 0; i < sizeof(psFileHeader->ui32Magic); i++)
  {
    aucHeader[iIndex++] = (unsigned char) ((psFileHeader->ui32Magic >> ((sizeof(psFileHeader->ui32Magic) - (i + 1)) * 8)) & 0xff);
  }
  for (i = 0; i < sizeof(psFileHeader->ui32Version); i++)
  {
    aucHeader[iIndex++] = (unsigned char) ((psFileHeader->ui32Version >> ((sizeof(psFileHeader->ui32Version) - (i + 1)) * 8)) & 0xff);
  }
  for (i = 0; i < sizeof(psFileHeader->ui32ThisChunkSize); i++)
  {
    aucHeader[iIndex++] = (unsigned char) ((psFileHeader->ui32ThisChunkSize >> ((sizeof(psFileHeader->ui32ThisChunkSize) - (i + 1)) * 8)) & 0xff);
  }
  for (i = 0; i < sizeof(psFileHeader->ui32LastChunkSize); i++)
  {
    aucHeader[iIndex++] = (unsigned char) ((psFileHeader->ui32LastChunkSize >> ((sizeof(psFileHeader->ui32LastChunkSize) - (i + 1)) * 8)) & 0xff);
  }
  for (i = 0; i < sizeof(psFileHeader->ui32ChunkId); i++)
  {
    aucHeader[iIndex++] = (unsigned char) ((psFileHeader->ui32ChunkId >> ((sizeof(psFileHeader->ui32ChunkId) - (i + 1)) * 8)) & 0xff);
  }

  /*-
   *********************************************************************
   *
   * Write the header.
   *
   *********************************************************************
   */
  iError = FTimesXpaWriteData(pFile, aucHeader, FTIMES_XPA_HEADER_SIZE, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaWriteKvp
 *
 ***********************************************************************
 */
int
FTimesXpaWriteKvp(FILE *pFile, int iKeyId, void *pvValue, int iValueLength, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaWriteKvp()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  int                 i = 0;
  int                 iIndex = 0;
  int                 iKvpLength = FTIMES_XPA_KIVL_SIZE + iValueLength;
  unsigned char      *pucKvp = NULL;
  unsigned int        uiKivl = 0;
  APP_UI32            ui32Value = 0;
  APP_UI64            ui64Value = 0;

  /*-
   *********************************************************************
   *
   * Make sure the value length is in the proper range.
   *
   *********************************************************************
   */
  if (iValueLength < 1 || iValueLength > FTIMES_XPA_MAX_VALUE_LENGTH)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: invalid value length (%d)", acRoutine, iValueLength);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Allocate the key/value pair buffer.
   *
   *********************************************************************
   */
  pucKvp = (unsigned char *) malloc(iKvpLength);
  if (pucKvp == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: unable to allocate key/value pair buffer (%s)", acRoutine, strerror(errno));
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Initialize the key/value pair buffer in an endian-agnostic way.
   *
   *********************************************************************
   */
  iIndex = 0;
  uiKivl = (iKeyId << 20) | iValueLength;
  for (i = 0; i < sizeof(uiKivl); i++)
  {
    pucKvp[iIndex++] = (unsigned char) ((uiKivl >> ((sizeof(uiKivl) - (i + 1)) * 8)) & 0xff);
  }

  switch (iKeyId)
  {
  case FTIMES_XPA_KEY_ID_NAME:
  case FTIMES_XPA_KEY_ID_MD5:
  case FTIMES_XPA_KEY_ID_SHA1:
  case FTIMES_XPA_KEY_ID_FILE_OSID:
  case FTIMES_XPA_KEY_ID_FILE_GSID:
  case FTIMES_XPA_KEY_ID_FILE_OWNER:
  case FTIMES_XPA_KEY_ID_FILE_GROUP:
  case FTIMES_XPA_KEY_ID_PATH_SEPARATOR:
    memcpy(&pucKvp[iIndex], (unsigned char *) pvValue, iValueLength);
    break;
  case FTIMES_XPA_KEY_ID_DATA_SIZE:
  case FTIMES_XPA_KEY_ID_HEAD_FLAGS:
  case FTIMES_XPA_KEY_ID_TAIL_FLAGS:
  case FTIMES_XPA_KEY_ID_FILE_UID:
  case FTIMES_XPA_KEY_ID_FILE_GID:
    ui32Value = *((APP_UI32 *) pvValue);
    for (i = 0; i < sizeof(ui32Value); i++)
    {
      pucKvp[iIndex++] = (unsigned char) ((ui32Value >> ((sizeof(ui32Value) - (i + 1)) * 8)) & 0xff);
    }
    break;
  case FTIMES_XPA_KEY_ID_FILE_REAL_SIZE:
  case FTIMES_XPA_KEY_ID_FILE_READ_SIZE:
  case FTIMES_XPA_KEY_ID_FILE_ATIME_RTUE:
  case FTIMES_XPA_KEY_ID_FILE_MTIME_RTUE:
  case FTIMES_XPA_KEY_ID_FILE_CTIME_RTUE:
    ui64Value = *((APP_UI64 *) pvValue);
    for (i = 0; i < sizeof(ui64Value); i++)
    {
      pucKvp[iIndex++] = (unsigned char) ((ui64Value >> ((sizeof(ui64Value) - (i + 1)) * 8)) & 0xff);
    }
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: invalid key ID (%d)", acRoutine, iKeyId);
    free(pucKvp);
    return ER;
    break;
  }

  /*-
   *********************************************************************
   *
   * Write the key/value pair buffer.
   *
   *********************************************************************
   */
  iError = FTimesXpaWriteData(pFile, pucKvp, iKvpLength, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    free(pucKvp);
    return ER;
  }

  free(pucKvp);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaAddMember
 *
 ***********************************************************************
 */
void
FTimesXpaAddMember(char *pcDecodedName, unsigned char *pucData, APP_UI32 ui32Blocksize, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaAddMember()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcEncodedName = NULL;
  FILE               *pFile = stdout;
  FTIMES_XPA_HANDLE  *psHandle = NULL;
  FTIMES_XPA_HEADER   sFileHeader = { 0 };
  unsigned char       aucChunkMd5[MD5_HASH_SIZE] = { 0 };
  unsigned char       aucChunkSha1[SHA1_HASH_SIZE] = { 0 };
  unsigned char       aucFinalMd5[MD5_HASH_SIZE] = { 0 };
  unsigned char       aucFinalSha1[SHA1_HASH_SIZE] = { 0 };
  MD5_CONTEXT         sChunkMd5 = { 0 };
  SHA1_CONTEXT        sChunkSha1 = { 0 };
  MD5_CONTEXT         sFinalMd5 = { 0 };
  SHA1_CONTEXT        sFinalSha1 = { 0 };
  int                 iError = 0;
  int                 iNameLength = strlen(pcDecodedName);
  int                 iNRead = 0;
  APP_UI32            ui32HeadFlags = 0;
  APP_UI32            ui32TailFlags = 0;
  APP_UI32            ui32LowerChunkId = 0;
  APP_UI32            ui32UpperChunkId = 0;
  APP_UI64            ui64FileRealSize = 0;
  APP_UI64            ui64FileReadSize = 0;
  APP_UI64            ui64FileATimeRtue = 0;
  APP_UI64            ui64FileMTimeRtue = 0;
  APP_UI64            ui64FileCTimeRtue = 0;

#ifdef WINNT
  BY_HANDLE_FILE_INFORMATION sFileInfo = {};
  char               *pcSidGroup = NULL;
  char               *pcSidOwner = NULL;
  DWORD               dwSize1 = 0;
  DWORD               dwSize2 = 0;
  DWORD               dwStatus = 0;
  SECURITY_DESCRIPTOR *psSd;
  SID                *psSidGroup;
  SID                *psSidOwner;
  SID_NAME_USE        sSidType;
  TCHAR               ptcGroupDomain[FTIMES_XPA_OWNER_NAME_SIZE] = {};
  TCHAR               ptcGroup[FTIMES_XPA_OWNER_NAME_SIZE] = {};
  TCHAR               ptcOwnerDomain[FTIMES_XPA_OWNER_NAME_SIZE] = {};
  TCHAR               ptcOwner[FTIMES_XPA_OWNER_NAME_SIZE] = {};
  TCHAR              *ptcWinxError = NULL;
#else
  APP_UI32            ui32FileUid = 0;
  APP_UI32            ui32FileGid = 0;
  struct passwd      *psOwner = NULL;
  struct group       *psGroup = NULL;
  struct stat         sStatEntry = {};
#endif

  /*-
   *********************************************************************
   *
   * Encode the filename.
   *
   *********************************************************************
   */
  pcEncodedName = FTimesXpaEncodeString(pcDecodedName, iNameLength, acLocalError);
  if (pcEncodedName == NULL)
  {
    fprintf(stderr, "%s: Error='Unable to encode \"%s\" (%s).'\n", PROGRAM_NAME, pcDecodedName, acLocalError);
    FTimesXpaAbort();
  }
  iNameLength = strlen(pcEncodedName);

  /*-
   *********************************************************************
   *
   * Open the file, initialize hashes, and get selected attributes.
   *
   *********************************************************************
   */
  psHandle = FTimesXpaGetHandle(pcDecodedName, acLocalError);
  if (psHandle == NULL)
  {
    fprintf(stderr, "%s: Error='Unable to obtain a handle for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_OPEN_ERROR;
  }
  else
  {
    MD5Alpha(&sFinalMd5);
    SHA1Alpha(&sFinalSha1);
#ifdef WINNT
    if (!GetFileInformationByHandle(psHandle->hFile, &sFileInfo))
    {
      FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
      fprintf(stderr, "%s: Error='Unable to obtain one or more attributes for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ptcWinxError);
      ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
    }
    else
    {
      ui64FileATimeRtue = (((APP_UI64)sFileInfo.ftLastAccessTime.dwHighDateTime) << 32) | (APP_UI64)sFileInfo.ftLastAccessTime.dwLowDateTime;
      ui64FileATimeRtue -= UNIX_EPOCH_IN_NT_TIME;
      ui64FileATimeRtue /= 10;
      ui64FileATimeRtue /= 1000000;
      ui64FileMTimeRtue = (((APP_UI64)sFileInfo.ftLastWriteTime.dwHighDateTime) << 32) | (APP_UI64)sFileInfo.ftLastWriteTime.dwLowDateTime;
      ui64FileMTimeRtue -= UNIX_EPOCH_IN_NT_TIME;
      ui64FileMTimeRtue /= 10;
      ui64FileMTimeRtue /= 1000000;
      ui64FileCTimeRtue = (((APP_UI64)sFileInfo.ftCreationTime.dwHighDateTime) << 32) | (APP_UI64)sFileInfo.ftCreationTime.dwLowDateTime;
      ui64FileCTimeRtue -= UNIX_EPOCH_IN_NT_TIME;
      ui64FileCTimeRtue /= 10;
      ui64FileCTimeRtue /= 1000000;
      ui64FileRealSize = (((APP_UI64)sFileInfo.nFileSizeHigh) << 32) | (APP_UI64)sFileInfo.nFileSizeLow;
    }
    dwStatus = GetSecurityInfo
    (
      psHandle->hFile,
      SE_FILE_OBJECT,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
      (PSID) &psSidOwner,
      (PSID) &psSidGroup,
      NULL, /* This pointer is not required to obtain DACL information. */
      NULL,
#if (WINVER <= 0x500)
      &psSd
#else
      (PSECURITY_DESCRIPTOR) &psSd
#endif
    );
    if (dwStatus != ERROR_SUCCESS || !IsValidSecurityDescriptor(psSd))
    {
      FTimesXpaFormatWinxError(dwStatus, &ptcWinxError); /* According to MSDN, GetSecurityInfo() returns a nonzero error code defined in WinError.h when it fails. */
      fprintf(stderr, "%s: Error='Unable to obtain one or more attributes for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ptcWinxError);
      ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
    }
    else
    {
      ConvertSidToStringSidA(psSidOwner, &pcSidOwner);
      ConvertSidToStringSidA(psSidGroup, &pcSidGroup);
      dwSize1 = FTIMES_XPA_OWNER_NAME_SIZE;
      dwSize2 = FTIMES_XPA_OWNER_NAME_SIZE;
      if (!LookupAccountSid(NULL, psSidOwner, ptcOwner, &dwSize1, ptcOwnerDomain, &dwSize2, &sSidType))
      {
        FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
        fprintf(stderr, "%s: Error='Unable to resolve owner SID for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ptcWinxError);
        ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
      }
      dwSize1 = FTIMES_XPA_OWNER_NAME_SIZE;
      dwSize2 = FTIMES_XPA_OWNER_NAME_SIZE;
      if (!LookupAccountSid(NULL, psSidGroup, ptcGroup, &dwSize1, ptcGroupDomain, &dwSize2, &sSidType))
      {
        FTimesXpaFormatWinxError(GetLastError(), &ptcWinxError);
        fprintf(stderr, "%s: Error='Unable to resolve group SID for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ptcWinxError);
        ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
      }
    }
#else
    if (fstat(fileno(psHandle->pFile), &sStatEntry) != ER_OK)
    {
      fprintf(stderr, "%s: Error='Unable to obtain one or more attributes for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, strerror(errno));
      ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
    }
    else
    {
      ui64FileATimeRtue = (APP_UI64)sStatEntry.st_atime;
      ui64FileMTimeRtue = (APP_UI64)sStatEntry.st_mtime;
      ui64FileCTimeRtue = (APP_UI64)sStatEntry.st_ctime;
      ui64FileRealSize = (APP_UI64)sStatEntry.st_size;
      ui32FileUid = (APP_UI32)sStatEntry.st_uid;
      ui32FileGid = (APP_UI32)sStatEntry.st_gid;
      errno = 0;
      psOwner = getpwuid(sStatEntry.st_uid);
      if (psOwner == NULL)
      {
        fprintf(stderr, "%s: Error='Unable to resolve owner name for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ((errno == 0) ? "Owner name is not mapped" : strerror(errno)));
        ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
      }
      errno = 0;
      psGroup = getgrgid(sStatEntry.st_gid);
      if (psGroup == NULL)
      {
        fprintf(stderr, "%s: Error='Unable to resolve group name for \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, ((errno == 0) ? "Group name is not mapped" : strerror(errno)));
        ui32HeadFlags |= FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR;
      }
    }
#endif
  }

  /*-
   *********************************************************************
   *
   * Write the HEAD chunk.
   *
   *********************************************************************
   */
  sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_HEAD;
  sFileHeader.ui32Version = FTIMES_XPA_VERSION;
  sFileHeader.ui32LastChunkSize = 0;
  sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE;
  sFileHeader.ui32ChunkId = ui32UpperChunkId;
  iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }

  /*-
   *********************************************************************
   *
   * Write the HKVP chunk.
   *
   *********************************************************************
   */
  sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_HKVP;
  sFileHeader.ui32Version = FTIMES_XPA_VERSION;
  sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
  sFileHeader.ui32ThisChunkSize =
      FTIMES_XPA_HEADER_SIZE
    + (4 + iNameLength)
    + (4 + 8) // FileRealSize
    + (4 + 8) // FileATimeRtue
    + (4 + 8) // FileMTimeRtue
    + (4 + 8) // FileCTimeRtue
    + (4 + 4) // HeadFlags
#ifdef WINNT
    + ((pcSidOwner) ? (4 + strlen(pcSidOwner)) : 0) // FileSidOwner
    + ((pcSidGroup) ? (4 + strlen(pcSidGroup)) : 0) // FileSidGroup
    + ((ptcOwner[0]) ? (4 + strlen(ptcOwner)) : 0) // FileOwner
    + ((ptcGroup[0]) ? (4 + strlen(ptcGroup)) : 0) // FileGroup
#else
    + (4 + 4) // FileUid
    + (4 + 4) // FileGid
    + ((psOwner) ? (4 + strlen(psOwner->pw_name)) : 0) // FileOwner
    + ((psGroup) ? (4 + strlen(psGroup->gr_name)) : 0) // FileGroup
#endif
    + (4 + FTIMES_XPA_PATH_SEPARATOR_LENGTH)
    ;
  sFileHeader.ui32ChunkId = ui32UpperChunkId;
  iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_NAME, (void *) pcEncodedName, iNameLength, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_REAL_SIZE, (void *) &ui64FileRealSize, sizeof(ui64FileRealSize), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_ATIME_RTUE, (void *) &ui64FileATimeRtue, sizeof(ui64FileATimeRtue), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_MTIME_RTUE, (void *) &ui64FileMTimeRtue, sizeof(ui64FileMTimeRtue), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_CTIME_RTUE, (void *) &ui64FileCTimeRtue, sizeof(ui64FileCTimeRtue), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_HEAD_FLAGS, (void *) &ui32HeadFlags, sizeof(ui32HeadFlags), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
#ifdef WINNT
  if (pcSidOwner)
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_OSID, (void *) pcSidOwner, strlen(pcSidOwner), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
    LocalFree(pcSidOwner);
  }
  if (pcSidGroup)
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_GSID, (void *) pcSidGroup, strlen(pcSidGroup), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
    LocalFree(pcSidGroup);
  }
//FIXME TCHAR/char mismatch and we'll probably need to utf8 encode the string.
  if (ptcOwner[0])
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_OWNER, (void *) ptcOwner, strlen(ptcOwner), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
  }
//FIXME TCHAR/char mismatch and we'll probably need to utf8 encode the string.
  if (ptcGroup[0])
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_GROUP, (void *) ptcGroup, strlen(ptcGroup), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
  }
#else
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_UID, (void *) &ui32FileUid, sizeof(ui32FileUid), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_GID, (void *) &ui32FileGid, sizeof(ui32FileGid), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  if (psOwner)
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_OWNER, (void *) psOwner->pw_name, strlen(psOwner->pw_name), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
  }
  if (psGroup)
  {
    iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_GROUP, (void *) psGroup->gr_name, strlen(psGroup->gr_name), acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
      FTimesXpaAbort();
    }
  }
#endif
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_PATH_SEPARATOR, (void *) FTIMES_XPA_PATH_SEPARATOR, FTIMES_XPA_PATH_SEPARATOR_LENGTH, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }

  /*-
   *********************************************************************
   *
   * Conditionally read the file and write DATA/DKVP chunks.
   *
   *********************************************************************
   */
  if (psHandle)
  {
    while (!feof(psHandle->pFile))
    {
      /*-
       *****************************************************************
       *
       * Read a block of data. If there's an error, set a flag, close
       * the current member, and move on. We do this to preserve any
       * data that's already be archived and to ensure that the next
       * member gets a proper start.
       *
       *****************************************************************
       */
      iNRead = fread(pucData, 1, ui32Blocksize, psHandle->pFile);
      if (ferror(psHandle->pFile))
      {
        fprintf(stderr, "%s: Error='Read error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, strerror(errno));
        ui32TailFlags |= FTIMES_XPA_MEMBER_TAIL_FLAG_READ_ERROR;
        break;
      }
      if (iNRead == 0)
      {
        break; /* We're done. */
      }
      ui64FileReadSize += (APP_UI64) iNRead;

      /*-
       *****************************************************************
       *
       * Create and/or update hashes.
       *
       *****************************************************************
       */
      MD5Alpha(&sChunkMd5);
      MD5Cycle(&sChunkMd5, pucData, iNRead);
      MD5Omega(&sChunkMd5, aucChunkMd5);
      MD5Cycle(&sFinalMd5, pucData, iNRead);

      SHA1Alpha(&sChunkSha1);
      SHA1Cycle(&sChunkSha1, pucData, iNRead);
      SHA1Omega(&sChunkSha1, aucChunkSha1);
      SHA1Cycle(&sFinalSha1, pucData, iNRead);

      /*-
       *****************************************************************
       *
       * Write the DATA chunk. If there's an error, abort.
       *
       *****************************************************************
       */
      sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_DATA;
      sFileHeader.ui32Version = FTIMES_XPA_VERSION;
      sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
      sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE + iNRead;
      sFileHeader.ui32ChunkId = ui32LowerChunkId;
      iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }
      iError = FTimesXpaWriteData(pFile, pucData, iNRead, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }

      /*-
       *****************************************************************
       *
       * Write the DKVP chunk.
       *
       *****************************************************************
       */
      sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_DKVP;
      sFileHeader.ui32Version = FTIMES_XPA_VERSION;
      sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
      sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE + 4 + 16 + 4 + 20 + 4 + 4;
      sFileHeader.ui32ChunkId = ui32LowerChunkId;
      iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }
      iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_MD5, (void *) aucChunkMd5, MD5_HASH_SIZE, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }
      iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_SHA1, (void *) aucChunkSha1, SHA1_HASH_SIZE, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }
      iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_DATA_SIZE, (void *) &iNRead, sizeof(iNRead), acLocalError);
      if (iError != ER_OK)
      {
        fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
        FTimesXpaAbort();
      }

      /*-
       *****************************************************************
       *
       * Update the books.
       *
       *****************************************************************
       */
      if (ui32LowerChunkId == 0xffffffff)
      {
        ui32UpperChunkId++;
        sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_JOIN;
        sFileHeader.ui32Version = FTIMES_XPA_VERSION;
        sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
        sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE;
        sFileHeader.ui32ChunkId = ui32UpperChunkId;
        iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
        if (iError != ER_OK)
        {
          fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
          FTimesXpaAbort();
        }
        ui32LowerChunkId = 0;
      }
      else
      {
        ui32LowerChunkId++;
      }
    }
    FTimesXpaFreeHandle(psHandle);
    MD5Omega(&sFinalMd5, aucFinalMd5);
    SHA1Omega(&sFinalSha1, aucFinalSha1);
  }
  else
  {
    memset(aucFinalMd5, 0, MD5_HASH_SIZE);
    memset(aucFinalSha1, 0, SHA1_HASH_SIZE);
  }

  /*-
   *********************************************************************
   *
   * Write the TKVP chunk.
   *
   *********************************************************************
   */
  sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_TKVP;
  sFileHeader.ui32Version = FTIMES_XPA_VERSION;
  sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
  sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE + 4 + 16 + 4 + 20 + 4 + 8 + 4 + 4;
  sFileHeader.ui32ChunkId = ui32UpperChunkId;
  iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_FILE_READ_SIZE, (void *) &ui64FileReadSize, sizeof(ui64FileReadSize), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_MD5, (void *) aucFinalMd5, MD5_HASH_SIZE, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_SHA1, (void *) aucFinalSha1, SHA1_HASH_SIZE, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }
  iError = FTimesXpaWriteKvp(pFile, FTIMES_XPA_KEY_ID_TAIL_FLAGS, (void *) &ui32TailFlags, sizeof(ui32TailFlags), acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }

  /*-
   *********************************************************************
   *
   * Write the TAIL chunk.
   *
   *********************************************************************
   */
  sFileHeader.ui32Magic = FTIMES_XPA_CHUNK_TYPE_TAIL;
  sFileHeader.ui32Version = FTIMES_XPA_VERSION;
  sFileHeader.ui32LastChunkSize = sFileHeader.ui32ThisChunkSize;
  sFileHeader.ui32ThisChunkSize = FTIMES_XPA_HEADER_SIZE;
  sFileHeader.ui32ChunkId = ui32UpperChunkId;
  iError = FTimesXpaWriteHeader(pFile, &sFileHeader, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Write error while processing \"%s\" (%s: %s).'\n", PROGRAM_NAME, pcEncodedName, acRoutine, acLocalError);
    FTimesXpaAbort();
  }

  if (pcEncodedName != NULL)
  {
    free(pcEncodedName);
  }

  return;
}

/*-
 ***********************************************************************
 *
 * FTimesXpaProcessArguments
 *
 ***********************************************************************
 */
int
FTimesXpaProcessArguments(int iArgumentCount, char *ppcArgumentVector[], FTIMES_XPA_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaProcessArguments()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcMode = NULL;
  int                 iError = 0;
  OPTIONS_CONTEXT    *psOptionsContext = NULL;
  OPTIONS_TABLE       asArchiveOptions[] =
  {
    { OPT_Blocksize, "-b", "--blocksize",  0, 0, 1, 0, FTimesXpaOptionHandler },
    { OPT_ListFile,  "-l", "--list",       0, 0, 1, 0, FTimesXpaOptionHandler },
  };

  /*-
   *********************************************************************
   *
   * Initialize the options context.
   *
   *********************************************************************
   */
  psProperties->psOptionsContext = psOptionsContext = OptionsNewOptionsContext(iArgumentCount, ppcArgumentVector, acLocalError);
  if (psOptionsContext == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Determine the run mode.
   *
   *********************************************************************
   */
  pcMode = OptionsGetFirstArgument(psOptionsContext);
  if (pcMode == NULL)
  {
    FTimesXpaUsage();
  }
  else
  {
    if (strcmp(pcMode, "-a") == 0 || strcmp(pcMode, "--archive") == 0)
    {
      psProperties->iRunMode = FTIMES_XPA_ARCHIVE_MODE;
      OptionsSetOptions(psOptionsContext, asArchiveOptions, (sizeof(asArchiveOptions) / sizeof(asArchiveOptions[0])));
    }
    else if (strcmp(pcMode, "-v") == 0 || strcmp(pcMode, "--version") == 0)
    {
      FTimesXpaVersion();
    }
    else
    {
      FTimesXpaUsage();
    }
  }

  /*-
   *********************************************************************
   *
   * Process options.
   *
   *********************************************************************
   */
  iError = OptionsProcessOptions2(psOptionsContext, (void *) psProperties, acLocalError);
  switch (iError)
  {
  case OPTIONS_ER:
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
    break;
  case OPTIONS_OK:
    break;
  case OPTIONS_USAGE:
  default:
    FTimesXpaUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * Handle any special cases.
   *
   *********************************************************************
   */
  switch (psProperties->iRunMode)
  {
  case FTIMES_XPA_ARCHIVE_MODE:
    break;
  default:
    FTimesXpaUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * If any required arguments are missing, it's an error.
   *
   *********************************************************************
   */
  if (OptionsHaveRequiredOptions(psOptionsContext) == 0)
  {
    FTimesXpaUsage();
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaBootStrap
 *
 ***********************************************************************
 */
int
FTimesXpaBootStrap(char *pcError)
{
  const char          acRoutine[] = "FTimesXpaBootStrap()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FTIMES_XPA_PROPERTIES *psProperties;

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Suppress critical-error-handler message boxes.
   *
   *********************************************************************
   */
  SetErrorMode(SEM_FAILCRITICALERRORS);

  /*-
   *********************************************************************
   *
   * Attempt to elevate privileges. If this fails, it's not fatal.
   *
   *********************************************************************
   */
  if (FTimesXpaSetPrivileges(acLocalError) != ER_OK)
  {
//FIXME Perhaps this should be fatal under certain conditions.
//  snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
//  return ER;
  }

  /*-
   *********************************************************************
   *
   * Put stdout in binary mode.
   *
   *********************************************************************
   */
  if (_setmode(_fileno(stdout), _O_BINARY) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unable to put stdout in binary mode (%s).", acRoutine, strerror(errno));
    return ER;
  }
#endif

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new properties structure.
   *
   *********************************************************************
   */
  psProperties = FTimesXpaNewProperties(acLocalError);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  FTimesXpaSetPropertiesReference(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaGetPropertiesReference
 *
 ***********************************************************************
 */
FTIMES_XPA_PROPERTIES *
FTimesXpaGetPropertiesReference(void)
{
  return gpsProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaSetPropertiesReference
 *
 ***********************************************************************
 */
void
FTimesXpaSetPropertiesReference(FTIMES_XPA_PROPERTIES *psProperties)
{
  gpsProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaOptionHandler
 *
 ***********************************************************************
 */
int
FTimesXpaOptionHandler(void *pvOption, char *pcValue, void *pvProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesXpaOptionHandler()";
  FTIMES_XPA_PROPERTIES *psProperties = (FTIMES_XPA_PROPERTIES *)pvProperties;
  int                 iLength = 0;
  OPTIONS_TABLE      *psOption = (OPTIONS_TABLE *)pvOption;

  iLength = (pcValue == NULL) ? 0 : strlen(pcValue);

  switch (psOption->iId)
  {
  case OPT_Blocksize:
    if (iLength < 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s], value=[%s]: value is null", acRoutine, psOption->atcFullName, pcValue);
      return ER;
    }
    while (iLength > 0)
    {
      if (!isdigit((int) pcValue[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s], value=[%s]: value must be a positive integer (digits only)", acRoutine, psOption->atcFullName, pcValue);
        return ER;
      }
      iLength--;
    }
    psProperties->ui32Blocksize = (APP_UI32) strtoul(pcValue, NULL, 10);
    if (errno == ERANGE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s], value=[%s]: %s", acRoutine, psOption->atcFullName, pcValue, strerror(errno));
      return ER;
    }
    if (psProperties->ui32Blocksize < FTIMES_XPA_MIN_BLOCKSIZE || psProperties->ui32Blocksize > FTIMES_XPA_MAX_BLOCKSIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s], value=[%s]: value must be in the range [%d-%d]", acRoutine, psOption->atcFullName, pcValue, FTIMES_XPA_MIN_BLOCKSIZE, FTIMES_XPA_MAX_BLOCKSIZE);
      return ER;
    }
    break;
  case OPT_ListFile:
    psProperties->pcListFile = pcValue;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: invalid option (%d)", acRoutine, psOption->iId);
    return ER;
    break;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaNewProperties
 *
 ***********************************************************************
 */
FTIMES_XPA_PROPERTIES *
FTimesXpaNewProperties(char *ptcError)
{
  const char          acRoutine[] = "FTimesXpaNewProperties()";
  FTIMES_XPA_PROPERTIES *psProperties = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (FTIMES_XPA_PROPERTIES *) calloc(sizeof(FTIMES_XPA_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    snprintf(ptcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */
  psProperties->ui32Blocksize = FTIMES_XPA_DEFAULT_BLOCKSIZE;

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaFreeProperties
 *
 ***********************************************************************
 */
void
FTimesXpaFreeProperties(FTIMES_XPA_PROPERTIES *psProperties)
{
  if (psProperties)
  {
    if (psProperties->pucData)
    {
      free(psProperties->pucData);
    }
    if (psProperties->psOptionsContext != NULL)
    {
      OptionsFreeOptionsContext(psProperties->psOptionsContext);
    }
    free(psProperties);
  }
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesXpaAdjustPrivileges
 *
 ***********************************************************************
 */
BOOL
FTimesXpaAdjustPrivileges(LPCTSTR lpcPrivilege)
{
  HANDLE              hToken;
  TOKEN_PRIVILEGES    sTokenPrivileges;
  BOOL                bResult;

  /*-
   *********************************************************************
   *
   * Open the access token associated with this process.
   *
   *********************************************************************
   */
  bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
  if (bResult == FALSE)
  {
    return FALSE;
  }

  /*-
   *********************************************************************
   *
   * Retrieve the locally unique identifier (LUID) used on this system
   * that represents the specified privilege.
   *
   *********************************************************************
   */
  bResult = LookupPrivilegeValue(NULL, lpcPrivilege, &sTokenPrivileges.Privileges[0].Luid);
  if (bResult == FALSE)
  {
    return FALSE;
  }
  sTokenPrivileges.PrivilegeCount = 1;

  /*-
   *********************************************************************
   *
   * One privilege to set.
   *
   *********************************************************************
   */
  sTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  /*-
   *********************************************************************
   *
   * Enable the specified privilege. Note: Enabling and/or disabling
   * privileges requires TOKEN_ADJUST_PRIVILEGES access.
   *
   *********************************************************************
   */
  bResult = AdjustTokenPrivileges(hToken, FALSE, &sTokenPrivileges, 0, (PTOKEN_PRIVILEGES) NULL, 0);
  if (bResult == FALSE || GetLastError() != ERROR_SUCCESS)
  {
    return FALSE;
  }

  return TRUE;
}


/*-
 ***********************************************************************
 *
 * FTimesXpaSetPrivileges
 *
 ***********************************************************************
 */
int
FTimesXpaSetPrivileges(char *pcError)
{
  const char          acRoutine[] = "FTimesXpaSetPrivileges()";

  /*-
   *********************************************************************
   *
   * Attempt to obtain backup user rights.
   *
   *********************************************************************
   */
  if (FTimesXpaAdjustPrivileges(SE_BACKUP_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unable to set SE_BACKUP_NAME privilege.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Attempt to obtain restore user rights.
   *
   *********************************************************************
   */
  if (FTimesXpaAdjustPrivileges(SE_RESTORE_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unable to set SE_RESTORE_NAME privilege.", acRoutine);
    return ER;
  }

  return ER_OK;
}
#endif
