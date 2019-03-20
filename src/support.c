/*-
 ***********************************************************************
 *
 * $Id: support.c,v 1.30 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static int (*CompareFunction)  () = strcasecmp;
static int (*NCompareFunction) () = strncasecmp;
#else
static int (*CompareFunction)  () = strcmp;
static int (*NCompareFunction) () = strncmp;
#endif

/*-
 ***********************************************************************
 *
 * SupportAddListItem
 *
 ***********************************************************************
 */
FILE_LIST *
SupportAddListItem(char *pcPath, FILE_LIST *psHead, char *pcError)
{
  const char          acRoutine[] = "SupportAddListItem()";
  int                 iLength;
  FILE_LIST          *psNewLink;
  FILE_LIST          *psCurrent;
  FILE_LIST          *psTail;

  iLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length less than 1 byte.", acRoutine, iLength);
    return NULL;
  }

  if (iLength > FTIMES_MAX_PATH - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", acRoutine, iLength, FTIMES_MAX_PATH - 1);
    return NULL;
  }

  psTail = psNewLink = (FILE_LIST *) malloc(sizeof(FILE_LIST)); /* The caller must free this storage. */
  if (psNewLink == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * If the head is NULL, create a new link and insert path.
   *
   *********************************************************************
   */
  if ((psCurrent = psHead) == NULL)
  {
    psHead = psNewLink;
  }
  else
  {
    while (psCurrent != NULL)
    {
      if (psCurrent->psNext == NULL)
      {
        psTail = psCurrent;
      }
      psCurrent = psCurrent->psNext;
    }
    psTail->psNext = psNewLink;
  }
  psTail = psNewLink;
  psTail->psNext = NULL;
  strncpy((char *) &psTail->acPath, (char *) pcPath, FTIMES_MAX_PATH);
  return psHead;
}


/*-
 ***********************************************************************
 *
 * SupportAddToList
 *
 ***********************************************************************
 */
int
SupportAddToList(char *pcPath, FILE_LIST **ppsList, char *pcListName, char *pcError)
{
  const char          acRoutine[] = "SupportAddToList()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acLocalPath[FTIMES_MAX_PATH];
  int                 i;
  int                 iLength;
  FILE_LIST          *psHead;

  /*-
   *********************************************************************
   *
   * Make sure that we have the start of a full path.
   *
   *********************************************************************
   */
#ifdef WIN32
    if (!(isalpha((int) pcPath[0]) && pcPath[1] == ':'))
#else
    if (pcPath[0] != FTIMES_SLASHCHAR)
#endif
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: List = [%s], Item = [%s]: A full path is required.", acRoutine, pcListName, pcPath);
      return ER;
    }

  /*-
   *********************************************************************
   *
   * Copy pcPath into acLocalPath removing extra slashes along the way.
   *
   *********************************************************************
   */
  for (i = 0, iLength = 0, memset(acLocalPath, 0, FTIMES_MAX_PATH); i < (int) strlen(pcPath); i++)
  {
    if (i > 0 && pcPath[i] == FTIMES_SLASHCHAR && pcPath[i - 1] == FTIMES_SLASHCHAR)
    {
      continue;
    }
    acLocalPath[iLength++] = pcPath[i];
  }

  /*-
   *********************************************************************
   *
   * If this is not the root directory chop off any trailing slashes.
   *
   *********************************************************************
   */
  if (strcmp(acLocalPath, FTIMES_SLASH) != 0)
  {
    while (acLocalPath[iLength - 1] == FTIMES_SLASHCHAR && iLength > 1)
    {
      acLocalPath[--iLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * If this is not a duplicate item, add it to the list.
   *
   *********************************************************************
   */
  if (SupportMatchExclude(*ppsList, acLocalPath) == NULL)
  {
    psHead = SupportAddListItem(acLocalPath, *ppsList, acLocalError);
    if (psHead == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: List = [%s], Item = [%s]: %s", acRoutine, pcListName, pcPath, acLocalError);
      return ER;
    }
    if (*ppsList == NULL)
    {
      *ppsList = psHead;
    }
  }
  else
  {
    snprintf(acLocalError, MESSAGE_SIZE, "List = [%s], Item = [%s]: Ignoring duplicate item.", pcListName, pcPath);
    ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
  }

  return ER_OK;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * SupportAdjustPrivileges
 *
 ***********************************************************************
 */
BOOL
SupportAdjustPrivileges(LPCTSTR lpcPrivilege)
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
  bResult = OpenProcessToken
              (
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &hToken
              );
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
  bResult = LookupPrivilegeValue(
                                NULL,
                                lpcPrivilege,
                                &sTokenPrivileges.Privileges[0].Luid
    );

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
  bResult = AdjustTokenPrivileges(
                                 hToken,
                                 FALSE,
                                 &sTokenPrivileges,
                                 0,
                                 (PTOKEN_PRIVILEGES) NULL,
                                 0
    );

  if (bResult == FALSE || GetLastError() != ERROR_SUCCESS)
  {
    return FALSE;
  }

  return TRUE;
}
#endif


/*-
 ***********************************************************************
 *
 * SupportCheckList
 *
 ***********************************************************************
 */
int
SupportCheckList(FILE_LIST *psHead, char *pcListName, char *pcError)
{
  const char          acRoutine[] = "SupportCheckList()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE_LIST          *psList;

  for (psList = psHead; psList != NULL; psList = psList->psNext)
  {
    if (SupportGetFileType(psList->acPath, acLocalError) == FTIMES_FILETYPE_ERROR)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: List = [%s], Item = [%s]: %s", acRoutine, pcListName, psList->acPath, acLocalError);
      return ER;
    }
  }
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportChopEOLs
 *
 ***********************************************************************
 */
int
SupportChopEOLs(char *pcLine, int iStrict, char *pcError)
{
  const char          acRoutine[] = "SupportChopEOLs()";
  int                 iLineLength;
  int                 iSaveLength;

  /*-
   *********************************************************************
   *
   * Calculate line length.
   *
   *********************************************************************
   */
  iLineLength = iSaveLength = strlen(pcLine);

  /*-
   *********************************************************************
   *
   * Scan backwards over EOL characters.
   *
   *********************************************************************
   */
  while (iLineLength > 0 && ((pcLine[iLineLength - 1] == '\r') || (pcLine[iLineLength - 1] == '\n')))
  {
    iLineLength--;
  }

  /*-
   *********************************************************************
   *
   * If strict checking is on and EOL was not found, it's an error.
   *
   *********************************************************************
   */
  if (iStrict && iLineLength == iSaveLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: EOL required but not found.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Terminate line excluding any EOL characters.
   *
   *********************************************************************
   */
  pcLine[iLineLength] = 0;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportDisplayRunStatistics
 *
 ***********************************************************************
 */
void
SupportDisplayRunStatistics(FTIMES_PROPERTIES *psProperties)
{
  char                acMessage[MESSAGE_SIZE];
  time_t              stopTime;

  /*-
   *********************************************************************
   *
   * Stop the RunTime clock. Report Warnings, Failures, and RunTime.
   *
   *********************************************************************
   */
  stopTime = time(NULL);

  snprintf(acMessage, MESSAGE_SIZE, "Warnings=%d", ErrorGetWarnings());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "Failures=%d", ErrorGetFailures());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  if (psProperties->tStartTime == ER || stopTime == ER)
  {
    snprintf(acMessage, MESSAGE_SIZE, "RunEpoch=NA");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

    snprintf(acMessage, MESSAGE_SIZE, "Duration=NA");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }
  else
  {
    snprintf(acMessage, MESSAGE_SIZE, "RunEpoch=%s %s %s", psProperties->acStartDate, psProperties->acStartTime, psProperties->acStartZone);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

    snprintf(acMessage, MESSAGE_SIZE, "Duration=%d", (int) (stopTime - psProperties->tStartTime));
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }
}


/*-
 ***********************************************************************
 *
 * SupportDropListItem
 *
 ***********************************************************************
 */
FILE_LIST *
SupportDropListItem(FILE_LIST *psHead, FILE_LIST *psDrop)
{
  FILE_LIST          *psList;
  FILE_LIST          *psNewHead;

  psNewHead = psHead;

  if (psDrop == psHead)
  {
    psNewHead = psDrop->psNext;
    free(psDrop);
  }
  else
  {
    for (psList = psHead; psList != NULL; psList = psList->psNext)
    {
      if (psDrop == psList->psNext)
      {
        psList->psNext = psDrop->psNext;
        free(psDrop);
      }
    }
  }
  return psNewHead;
}


/*-
 ***********************************************************************
 *
 * SupportEraseFile
 *
 ***********************************************************************
 */
int
SupportEraseFile(char *pcName, char *pcError)
{
  const char          acRoutine[] = "SupportEraseFile()";
#define WIPE_BUFSIZE 0x8000
  char               *apcWipe[WIPE_BUFSIZE];
  long                lFileLength;
  long                lNWiped;
  FILE               *pFile;

  if ((pFile = fopen(pcName, "rb+")) != NULL)
  {
    fseek(pFile, 0, SEEK_END);
    lFileLength = ftell(pFile);
    rewind(pFile);
    lNWiped = 0;
    memset(apcWipe, 0xa5, WIPE_BUFSIZE);
    while (((lNWiped += fwrite(apcWipe, 1, WIPE_BUFSIZE, pFile)) <= lFileLength) && !ferror(pFile));
    fclose(pFile);
  }

  if (unlink(pcName) != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportExpandDirectoryPath
 *
 ***********************************************************************
 */
int
SupportExpandDirectoryPath(char *pcPath, char *pcFullPath, int iFullPathSize, char *pcError)
{
  const char          acRoutine[] = "SupportExpandDirectoryPath()";
  char               *pcCwdDir;
  char               *pcNewDir;
  char               *pcTempPath;
  int                 iError;
  int                 iLength;

  iLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s], Length = [%d]: Length less than 1 byte.", acRoutine, pcPath, iLength);
    return ER_Length;
  }

  if (iLength > iFullPathSize - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, pcPath, iLength, iFullPathSize - 1);
    return ER_Length;
  }

  pcTempPath = malloc(iLength + 2); /* +1 for a FTIMES_SLASHCHAR, +1 for a NULL */
  if (pcTempPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s]: %s", acRoutine, pcPath, strerror(errno));
    return ER_BadHandle;
  }

  strcpy(pcTempPath, pcPath);

  /*-
   *********************************************************************
   *
   * We need to tack on a FTIMES_SLASHCHAR so that NT can figure out what we
   * want to do. If you are in c:\xyz and type "cd c:", NT will simply
   * print out the name of the directory that you are in (i.e. c:\xyz).
   * To avoid this problem, you need to type "cd c:\". The same seems
   * to be true for getwcd().
   *
   *********************************************************************
   */
  if (pcPath[iLength - 1] != FTIMES_SLASHCHAR)
  {
    pcTempPath[iLength] = FTIMES_SLASHCHAR;
    pcTempPath[iLength + 1] = 0;
  }

  /*-
   *********************************************************************
   *
   * Save the current directory entry after getting its full path.
   *
   *********************************************************************
   */
  pcCwdDir = getcwd(NULL, FTIMES_MAX_PATH);
  if (pcCwdDir == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s]: %s", acRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Change to the specified directory, and expand its path.
   *
   *********************************************************************
   */
  iError = chdir((const char *) pcTempPath);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s]: %s", acRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER;
  }

  pcNewDir = getcwd(pcFullPath, iFullPathSize);
  if (pcNewDir == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s]: %s", acRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Chop off any trailing slashes.
   *
   *********************************************************************
   */
  iLength = strlen(pcFullPath);
  while (pcFullPath[iLength - 1] == FTIMES_SLASHCHAR && iLength > 1)
  {
    pcFullPath[--iLength] = 0;
  }

  /*-
   *********************************************************************
   *
   * Change back to the starting directory.
   *
   *********************************************************************
   */
  iError = chdir((const char *) pcCwdDir);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s]: %s", acRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER;
  }

  free(pcTempPath);
  free(pcCwdDir); /* Created by getcwd() */

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportExpandPath
 *
 ***********************************************************************
 */
int
SupportExpandPath(char *pcPath, char *pcFullPath, int iFullPathSize, int iForceExpansion, char *pcError)
{
  const char          acRoutine[] = "SupportExpandPath()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcTempFile;
  char               *pcTempPath;
  int                 iError;
  int                 iLength;
  int                 iTempLength;

  iLength = iTempLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Length = [%d]: Length less than 1 byte.", acRoutine, pcPath, iLength);
    return ER_Length;
  }

  if (iLength > iFullPathSize - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, pcPath, iLength, iFullPathSize - 1);
    return ER_Length;
  }

  /*-
   *********************************************************************
   *
   * When forced expansion is off (i.e. 0), fully qualified paths will
   * be copied directly into the output buffer. Relative paths must be
   * expanded in any case.
   *
   *********************************************************************
   */
  if (!iForceExpansion)
  {
#ifdef WIN32
    if (
         (iLength == 2 && isalpha((int) pcPath[0]) && pcPath[1] == ':') ||
         (iLength >= 3 && isalpha((int) pcPath[0]) && pcPath[1] == ':' && pcPath[2] == FTIMES_SLASHCHAR)
       )
    {
      strncpy(pcFullPath, pcPath, iFullPathSize);
      return ER_OK;
    }
#endif
#ifdef UNIX
    if (pcPath[0] == FTIMES_SLASHCHAR)
    {
      strncpy(pcFullPath, pcPath, iFullPathSize);
      return ER_OK;
    }
#endif
  }

  switch (SupportGetFileType(pcPath, acLocalError))
  {
  case FTIMES_FILETYPE_ERROR:
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcPath, acLocalError);
    return ER_BadValue;
    break;

  case FTIMES_FILETYPE_DIRECTORY:
    iError = SupportExpandDirectoryPath(pcPath, pcFullPath, iFullPathSize, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Path = [%s]: %s", acRoutine, pcPath, acLocalError);
      return iError;
    }
    break;

  default:
    /*-
     *******************************************************************
     *
     * Create a working copy of the input path.
     *
     *******************************************************************
     */
    pcTempFile = malloc(iLength + 1); /* +1 for a NULL */
    if (pcTempFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcPath, strerror(errno));
      return ER_BadHandle;
    }

    pcTempPath = malloc(iLength + 1); /* +1 for a NULL */
    if (pcTempPath == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcPath, strerror(errno));
      free(pcTempFile);
      return ER_BadHandle;
    }

    strcpy(pcTempPath, pcPath);

    /*-
     *******************************************************************
     *
     * Scan backwards looking for a directory separator. If found, note
     * the location.
     *
     *******************************************************************
     */
    while (iTempLength > 0)
    {
      if (pcTempPath[iTempLength - 1] == FTIMES_SLASHCHAR)
      {
        break;
      }
      iTempLength--;
    }

    /*-
     *******************************************************************
     *
     * Copy off the filename portion of the path. It will be referenced
     * during construction of the full path. Insert a null after the
     * last directory separator to terminate the directory portion of
     * the path.
     *
     *******************************************************************
     */
    strcpy(pcTempFile, &pcTempPath[iTempLength]);
    pcTempPath[iTempLength] = 0;

    /*-
     *******************************************************************
     *
     * Expand the directory path. If length is zero, a valid directory
     * separator was not found. In that case, expand cwd (i.e. FTIMES_DOT).
     *
     *******************************************************************
     */
    if (iTempLength == 0)
    {
      iError = SupportExpandDirectoryPath(FTIMES_DOT, pcFullPath, iFullPathSize, acLocalError);
    }
    else
    {
      iError = SupportExpandDirectoryPath(pcTempPath, pcFullPath, iFullPathSize, acLocalError);
    }
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcPath, acLocalError);
      free(pcTempPath);
      free(pcTempFile);
      return iError;
    }

    /*-
     *******************************************************************
     *
     * Construct the full path. Abort, if the size limit is exceeded.
     *
     *******************************************************************
     */
    if ((int)(strlen(pcFullPath) + strlen(FTIMES_SLASH) + strlen(pcTempFile)) > iFullPathSize - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: Length would exceed %d bytes.", acRoutine, pcPath, iFullPathSize - 1);
      free(pcTempPath);
      free(pcTempFile);
      return ER_Length;
    }
    strcat(pcFullPath, FTIMES_SLASH);
    strcat(pcFullPath, pcTempFile);

    free(pcTempPath);
    free(pcTempFile);

    break;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportFreeData
 *
 ***********************************************************************
 */
void
SupportFreeData(void *pcData)
{
  if (pcData != NULL)
  {
    free(pcData);
  }
}


/*-
 ***********************************************************************
 *
 * SupportGetFileHandle
 *
 ***********************************************************************
 */
FILE *
SupportGetFileHandle(char *pcFile, char *pcError)
{
  const char          acRoutine[] = "SupportGetFileHandle()";
  static int          iStdinTaken = 0;
  FILE               *pFile = NULL;

  /*-
   *********************************************************************
   *
   * Open the specified file. If "-" was specified, bind the handle to
   * stdin, but do not do this more than once per invocation.
   *
   *********************************************************************
   */
  if (strcmp(pcFile, "-") == 0 && iStdinTaken == 0)
  {
    pFile = stdin;
    iStdinTaken = 1;
  }
  else
  {
    pFile = fopen(pcFile, "rb");
    if (pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): %s", acRoutine, strerror(errno));
      return NULL;
    }
  }

  return pFile;
}


/*-
 ***********************************************************************
 *
 * SupportGetFileType
 *
 ***********************************************************************
 */
int
SupportGetFileType(char *pcPath, char *pcError)
{
  const char          acRoutine[] = "SupportGetFileType()";
  struct stat         sStatEntry;

#ifdef UNIX
  if (lstat(pcPath, &sStatEntry) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: lstat(): %s", acRoutine, strerror(errno));
    return FTIMES_FILETYPE_ERROR;
  }

  switch (sStatEntry.st_mode & S_IFMT)
  {
  case S_IFBLK:
    return FTIMES_FILETYPE_BLOCK;
    break;
  case S_IFCHR:
    return FTIMES_FILETYPE_CHARACTER;
    break;
  case S_IFDIR:
    return FTIMES_FILETYPE_DIRECTORY;
    break;
#ifdef S_IFDOOR
  case S_IFDOOR:
    return FTIMES_FILETYPE_DOOR;
    break;
#endif
  case S_IFIFO:
    return FTIMES_FILETYPE_FIFO;
    break;
  case S_IFLNK:
    return FTIMES_FILETYPE_LINK;
    break;
  case S_IFREG:
    return FTIMES_FILETYPE_REGULAR;
    break;
  case S_IFSOCK:
    return FTIMES_FILETYPE_SOCKET;
    break;
#ifdef S_IFWHT
  case S_IFWHT:
    return FTIMES_FILETYPE_WHITEOUT;
    break;
#endif
  default:
    return FTIMES_FILETYPE_UNKNOWN;
    break;
  }
#endif

#ifdef WIN32
  char                acWorkingPath[4];

  if ((isalpha((int) pcPath[0]) && pcPath[1] == ':' && pcPath[2] == 0))
  {
    acWorkingPath[0] = pcPath[0];
    acWorkingPath[1] = pcPath[1];
    acWorkingPath[2] = FTIMES_SLASHCHAR;
    acWorkingPath[3] = 0;

    if (stat(acWorkingPath, &sStatEntry) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: stat(): %s", acRoutine, strerror(errno));
      return FTIMES_FILETYPE_ERROR;
    }
  }
  else
  {
    if (stat(pcPath, &sStatEntry) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: stat(): %s", acRoutine, strerror(errno));
      return FTIMES_FILETYPE_ERROR;
    }
  }

  switch (sStatEntry.st_mode & _S_IFMT)
  {
  case _S_IFCHR:
    return FTIMES_FILETYPE_CHARACTER;
    break;
  case _S_IFDIR:
    return FTIMES_FILETYPE_DIRECTORY;
    break;
  case _S_IFIFO:
    return FTIMES_FILETYPE_FIFO;
    break;
  case _S_IFREG:
    return FTIMES_FILETYPE_REGULAR;
    break;
  default:
    return FTIMES_FILETYPE_UNKNOWN;
    break;
  }
#endif
}


/*-
 ***********************************************************************
 *
 * SupportGetHostname
 *
 ***********************************************************************
 */
char *
SupportGetHostname(void)
{
#define MAX_HOSTNAME_LENGTH 256
  static char         acHostname[MAX_HOSTNAME_LENGTH] = "NA";
#ifdef UNIX
  struct utsname      sUTSName;

  memset(&sUTSName, 0, sizeof(struct utsname));
  if (uname(&sUTSName) != -1)
  {
    snprintf(acHostname, MAX_HOSTNAME_LENGTH, "%s", (sUTSName.nodename[0]) ? sUTSName.nodename : "NA");
  }
#endif
#ifdef WIN32
  char                acTempname[MAX_HOSTNAME_LENGTH];
  DWORD               dwTempNameLength = sizeof(acTempname);

  if (GetComputerName(acTempname, &dwTempNameLength) == TRUE)
  {
    snprintf(acHostname, MAX_HOSTNAME_LENGTH, "%s", acTempname);
  }
#endif
  return acHostname;
}


/*-
 ***********************************************************************
 *
 * SupportGetMyVersion
 *
 ***********************************************************************
 */
char *
SupportGetMyVersion(void)
{
#define MAX_VERSION_LENGTH 256
  static char         acMyVersion[MAX_VERSION_LENGTH] = "NA";
  int                 iIndex = 0;
  int                 iLength = MAX_VERSION_LENGTH;

  iIndex += snprintf(&acMyVersion[iIndex], iLength, "%s %s", PROGRAM_NAME, VERSION);
  iLength -= iIndex;
#ifdef USE_PCRE
  iIndex += snprintf(&acMyVersion[iIndex], iLength, " pcre");
  iLength -= strlen(" pcre");
#endif
#ifdef USE_SSL
  iIndex += snprintf(&acMyVersion[iIndex], iLength, " ssl");
  iLength -= strlen(" ssl");
#endif
#ifdef USE_XMAGIC
  iIndex += snprintf(&acMyVersion[iIndex], iLength, " xmagic");
  iLength -= strlen(" xmagic");
#endif
  iIndex += snprintf(&acMyVersion[iIndex], iLength, " %d-bit", (int) (sizeof(&SupportGetMyVersion) * 8));
  return acMyVersion;
}


/*-
 ***********************************************************************
 *
 * SupportGetSystemOS
 *
 ***********************************************************************
 */
char *
SupportGetSystemOS(void)
{
#define MAX_SYSTEMOS_LENGTH 256
  static char         acSystemOS[MAX_SYSTEMOS_LENGTH] = "NA";
#ifdef UNIX
  struct utsname      sUTSName;

  memset(&sUTSName, 0, sizeof(struct utsname));
  if (uname(&sUTSName) != -1)
  {
#ifdef FTimes_AIX
    snprintf(acSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %s.%s", sUTSName.machine, sUTSName.sysname, sUTSName.version, sUTSName.release);
#else
    snprintf(acSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %s", sUTSName.machine, sUTSName.sysname, sUTSName.release);
#endif
  }
#endif
#ifdef WIN32
  char                acOS[16];
  char                acPlatform[16];
  OSVERSIONINFO       sOSVersionInfo;
  SYSTEM_INFO         sSystemInfo;

  memset(&sSystemInfo, 0, sizeof(SYSTEM_INFO));
  GetSystemInfo(&sSystemInfo);
  switch (sSystemInfo.wProcessorArchitecture)
  {
  case PROCESSOR_ARCHITECTURE_INTEL:
    strncpy(acPlatform, "INTEL", sizeof(acPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_MIPS:
    strncpy(acPlatform, "MIPS", sizeof(acPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_ALPHA:
    strncpy(acPlatform, "ALPHA", sizeof(acPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_PPC:
    strncpy(acPlatform, "PPC", sizeof(acPlatform));
    break;
  default:
    strncpy(acPlatform, "NA", sizeof(acPlatform));
    break;
  }

  memset(&sOSVersionInfo, 0, sizeof(OSVERSIONINFO));
  sOSVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (GetVersionEx(&sOSVersionInfo) == TRUE)
  {
    switch (sOSVersionInfo.dwPlatformId)
    {
    case VER_PLATFORM_WIN32s:
      strncpy(acOS, "Windows 3.1", sizeof(acOS));
      break;
    case VER_PLATFORM_WIN32_WINDOWS:
      strncpy(acOS, "Windows 98", sizeof(acOS));
      break;
    case VER_PLATFORM_WIN32_NT:
      strncpy(acOS, "Windows NT", sizeof(acOS));
      break;
    default:
      strncpy(acOS, "NA", sizeof(acOS));
      break;
    }

    snprintf(
              acSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %u.%u Build %u %s",
              acPlatform,
              acOS,
              sOSVersionInfo.dwMajorVersion,
              sOSVersionInfo.dwMinorVersion,
              sOSVersionInfo.dwBuildNumber,
              sOSVersionInfo.szCSDVersion
            );
  }
#endif
  return acSystemOS;
}


/*-
 ***********************************************************************
 *
 * SupportIncludeEverything
 *
 ***********************************************************************
 */
FILE_LIST *
SupportIncludeEverything(char *pcError)
{
  const char          acRoutine[] = "SupportIncludeEverything()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE_LIST          *psHead;

#ifdef WIN32
  char                acDriveList[26 * 4 + 2];
  char               *pcDrive;
  int                 iLength;
  int                 iTempLength;

  psHead = NULL;

  if (GetLogicalDriveStrings(26 * 4 + 2, acDriveList) == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: GetLogicalDriveStrings: %u", acRoutine, GetLastError());
    return NULL;
  }

  /*-
   *******************************************************************
   *
   * Strip off backslash characters, and add the drive to the Include
   * list. Remember the total length of the original drive as it is
   * needed to update pcDrive next time through the loop.
   *
   *******************************************************************
   */
  for (pcDrive = acDriveList; *pcDrive; pcDrive += iLength + 1)
  {
    iLength = iTempLength = strlen(pcDrive);
    while (pcDrive[iTempLength - 1] == FTIMES_SLASHCHAR)
    {
      pcDrive[--iTempLength] = 0;
    }

    psHead = SupportAddListItem(pcDrive, psHead, acLocalError);
    if (psHead == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Include = [%s]: %s", acRoutine, pcDrive, acLocalError);
      return NULL;
    }
  }

  if (psHead == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: No supported drives found.", acRoutine);
  }
#endif
#ifdef UNIX

  psHead = SupportAddListItem(FTIMES_ROOT_PATH, NULL, acLocalError);
  if (psHead == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Include = [%s]: %s", acRoutine, FTIMES_ROOT_PATH, acLocalError);
    return NULL;
  }
#endif
  return psHead;
}


/*-
 ***********************************************************************
 *
 * SupportMakeName
 *
 ***********************************************************************
 */
int
SupportMakeName(char *pcDir, char *pcBaseName, char *pcBaseNameSuffix, char *pcExtension, char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "SupportMakeName()";
  int                 iLength;

  iLength  = strlen(pcDir);
  iLength += 1; /* FTIMES_SLASH */
  iLength += strlen(pcBaseName);
  iLength += (pcBaseNameSuffix[0] != 0) ? 1 : 0; /* "_" */
  iLength += strlen(pcBaseNameSuffix);
  iLength += strlen(pcExtension);

  if (iLength > FTIMES_MAX_PATH - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filename would exceed %d bytes", acRoutine, (FTIMES_MAX_PATH - 1));
    return ER_Length;
  }
  snprintf(pcFilename, FTIMES_MAX_PATH, "%s%s%s%s%s%s",
    pcDir,
    FTIMES_SLASH,
    pcBaseName,
    (pcBaseNameSuffix[0] != 0) ? "_" : "",
    pcBaseNameSuffix,
    pcExtension
    );

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportMatchExclude
 *
 ***********************************************************************
 */
FILE_LIST *
SupportMatchExclude(FILE_LIST *psHead, char *pcPath)
{
  FILE_LIST          *psList;

  for (psList = psHead; psList != NULL; psList = psList->psNext)
  {
    if (CompareFunction(psList->acPath, pcPath) == 0)
    {
      return psList;
    }
  }
  return NULL;
}


/*-
 ***********************************************************************
 *
 * SupportMatchSubTree
 *
 ***********************************************************************
 */
FILE_LIST *
SupportMatchSubTree(FILE_LIST *psHead, FILE_LIST *psTarget)
{
  int                 x;
  int                 y;
  FILE_LIST          *psList;

  x = strlen(psTarget->acPath);
  for (psList = psHead; psList != NULL; psList = psList->psNext)
  {
    y = strlen(psList->acPath);
    if (NCompareFunction(psTarget->acPath, psList->acPath, MIN(x, y)) == 0)
    {
      if (x <= y)
      {
        if ((psList->acPath[x - 1] == FTIMES_SLASHCHAR && psList->acPath[x] != FTIMES_SLASHCHAR) ||
            (psList->acPath[x - 1] != FTIMES_SLASHCHAR && psList->acPath[x] == FTIMES_SLASHCHAR) ||
            (psList->acPath[x - 1] == FTIMES_SLASHCHAR && psList->acPath[x] == FTIMES_SLASHCHAR))
        {
          return psList;
        }
      }
      else
      {
        if ((psTarget->acPath[y - 1] == FTIMES_SLASHCHAR && psTarget->acPath[y] != FTIMES_SLASHCHAR) ||
            (psTarget->acPath[y - 1] != FTIMES_SLASHCHAR && psTarget->acPath[y] == FTIMES_SLASHCHAR) ||
            (psTarget->acPath[y - 1] == FTIMES_SLASHCHAR && psTarget->acPath[y] == FTIMES_SLASHCHAR))
        {
          return psTarget;
        }
      }
    }
  }
  return NULL;
}


/*-
 ***********************************************************************
 *
 * SupportNeuterString
 *
 ***********************************************************************
 */
char *
SupportNeuterString(char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SupportNeuterString()";
  char               *pcNeutered;
  int                 i;
  int                 n;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcNeutered = malloc((3 * iLength) + 1);
  if (pcNeutered == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcNeutered[0] = 0;

  /*-
   *********************************************************************
   *
   * Neuter non-printables and [|"'`%+]. Convert spaces to '+'. Avoid
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
      n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
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
        n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
        break;
      case ' ':
        pcNeutered[n++] = '+';
        break;
      default:
        pcNeutered[n++] = pcData[i];
        break;
      }
    }
  }
  pcNeutered[n] = 0;

  return pcNeutered;
}


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * SupportNeuterStringW
 *
 ***********************************************************************
 */
char *
SupportNeuterStringW(unsigned short *pusData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SupportNeuterStringW()";
  char                cH;
  char                cL;
  char               *pcNeutered;
  int                 i;
  int                 n;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcNeutered = malloc((3 * (2 * iLength)) + 1);
  if (pcNeutered == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcNeutered[0] = 0;

  /*-
   *********************************************************************
   *
   * Neuter high byte, !isprint, and [|"'`%+]. Convert spaces to '+'.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    cH = (char) (pusData[i] >> 8);
    cL = (char) (pusData[i] & 0xff);
    n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) cH);
    if (isprint((int) cL))
    {
      switch (cL)
      {
      case '|':
      case '"':
      case '\'':
      case '`':
      case '%':
      case '+':
        n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) cL);
        break;
      case ' ':
        pcNeutered[n++] = '+';
        break;
      default:
        pcNeutered[n++] = cL;
        break;
      }
    }
    else
    {
      n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) cL);
    }
  }
  pcNeutered[n] = 0;

  return pcNeutered;
}
#endif


/*-
 ***********************************************************************
 *
 * SupportPruneList
 *
 ***********************************************************************
 */
FILE_LIST *
SupportPruneList(FILE_LIST *psList, char *pcListName)
{
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE_LIST          *psListHead;
  FILE_LIST          *psListTree;
  FILE_LIST          *psListKill;

  /*-
   *********************************************************************
   *
   * If there's nothing to prune, just return.
   *
   *********************************************************************
   */
  if (psList == NULL)
  {
    return psList;
  }

  /*-
   *********************************************************************
   *
   * Eliminate any subtree components from the tree list. For example,
   * /usr/local would be eliminated from /usr. We also eliminate any
   * duplicate entries in the process. However, there should not be any
   * duplicates because they should have been automatically pruned as
   * the list was being created.
   *
   *********************************************************************
   */
  for (psListTree = psListHead = psList; psListTree->psNext != NULL;)
  {
    psListKill = SupportMatchSubTree(psListTree->psNext, psListTree);

    /*-
     *******************************************************************
     *
     * When a match is found, another search is performed to ensure
     * that there are no more matches further down in the list. This is
     * done implicitly by setting psListTree to psListHead after the
     * drop.
     *
     *******************************************************************
     */
    if (psListKill != NULL)
    {
      snprintf(acLocalError, MESSAGE_SIZE, "List = [%s], Item = [%s]: Pruning item because it is part of a larger branch.", pcListName, psListKill->acPath);
      ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
      psListHead = SupportDropListItem(psListHead, psListKill);
      psListTree = psListHead;
    }
    else
    {
      psListTree = psListTree->psNext;
    }
  }
  return psListHead;
}


/*-
 ***********************************************************************
 *
 * SupportRequirePrivilege
 *
 ***********************************************************************
 */
int
SupportRequirePrivilege(char *pcError)
{
  const char          acRoutine[] = "SupportRequirePrivilege()";

#ifdef WINNT
  char                acLocalError[MESSAGE_SIZE] = { 0 };

  if (SupportSetPrivileges(acLocalError) != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
#endif

#ifdef UNIX
  if (getuid() != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Need root privilege to continue.", acRoutine);
    return ER;
  }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportSetLogLevel
 *
 ***********************************************************************
 */
int
SupportSetLogLevel(char *pcLevel, int *piLevel, char *pcError)
{
  const char          acRoutine[] = "SupportSetLogLevel()";

  if ((int) strlen(pcLevel) != 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: LogLevel must be %d-%d", acRoutine, MESSAGE_DEBUGGER, MESSAGE_CRITICAL);
    return ER_Length;
  }
  switch (pcLevel[0])
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
    *piLevel = pcLevel[0] - '0';
    MessageSetLogLevel(*piLevel);
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: LogLevel must be %d-%d", acRoutine, MESSAGE_DEBUGGER, MESSAGE_CRITICAL);
    return ER_Length;
    break;
  }
  return ER_OK;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * SupportSetPrivileges
 *
 ***********************************************************************
 */
int
SupportSetPrivileges(char *pcError)
{
  const char          acRoutine[] = "SupportSetPrivileges()";

  /*-
   *********************************************************************
   *
   * Attempt to obtain backup user rights.
   *
   *********************************************************************
   */
  if (SupportAdjustPrivileges(SE_BACKUP_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Can't set SE_BACKUP_NAME privilege.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Attempt to obtain restore user rights.
   *
   *********************************************************************
   */
  if (SupportAdjustPrivileges(SE_RESTORE_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Can't set SE_RESTORE_NAME privilege.", acRoutine);
    return ER;
  }

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * SupportWriteData
 *
 ***********************************************************************
 */
int
SupportWriteData(FILE *pFile, char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SupportWriteData()";
  int                 iNWritten;

  iNWritten = fwrite(pcData, 1, iLength, pFile);
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


#ifdef USE_PCRE
/*-
 ***********************************************************************
 *
 * SupportAddFilter
 *
 ***********************************************************************
 */
int
SupportAddFilter(char *pcFilter, FILTER_LIST **psHead, char *pcError)
{
  const char          acRoutine[] = "SupportAddFilter()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILTER_LIST        *psCurrent = NULL;
  FILTER_LIST        *psFilter = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new Filter.
   *
   *********************************************************************
   */
  psFilter = SupportNewFilter(pcFilter, acLocalError);
  if (psFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * If the head is NULL, insert the new filter and return. Otherwise,
   * append the new filter to the end of the list.
   *
   *********************************************************************
   */
  if (*psHead == NULL)
  {
    *psHead = psFilter;
  }
  else
  {
    psCurrent = *psHead;
    while (psCurrent != NULL)
    {
      if (psCurrent->psNext == NULL)
      {
        psCurrent->psNext = psFilter;
        break;
      }
      psCurrent = psCurrent->psNext;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportFreeFilter
 *
 ***********************************************************************
 */
void
SupportFreeFilter(FILTER_LIST *psFilter)
{
  if (psFilter != NULL)
  {
    if (psFilter->pcFilter != NULL)
    {
      free(psFilter->pcFilter);
    }
    if (psFilter->psPcre != NULL)
    {
      pcre_free(psFilter->psPcre);
    }
    if (psFilter->psPcreExtra != NULL)
    {
      pcre_free(psFilter->psPcreExtra);
    }
    free(psFilter);
  }
}


/*-
 ***********************************************************************
 *
 * SupportMatchFilter
 *
 ***********************************************************************
 */
FILTER_LIST *
SupportMatchFilter(FILTER_LIST *psFilterList, char *pcPath)
{
  FILTER_LIST        *psFilter;
#ifndef PCRE_OVECTOR_ARRAY_SIZE
#define PCRE_OVECTOR_ARRAY_SIZE 30
#endif
  int                 aiPcreOVector[PCRE_OVECTOR_ARRAY_SIZE];
  int                 iError = 0;

  for (psFilter = psFilterList; psFilter != NULL; psFilter = psFilter->psNext)
  {
    /*-
     *******************************************************************
     *
     * PCRE_NOTEMPTY is used here to squash any attempts to match
     * empty strings. Only unfettered matches will be accepted as
     * valid. In particular, a return value of zero will be ignored
     * eventhough it indicates there was a match. A value of zero
     * would mean that there was an overflow in ovector, and that
     * should never happen based on the restriction that no filter
     * contain capturing subpatterns.
     *
     *******************************************************************
     */
    iError = pcre_exec(psFilter->psPcre, psFilter->psPcreExtra, pcPath, strlen(pcPath), 0, PCRE_NOTEMPTY, aiPcreOVector, PCRE_OVECTOR_ARRAY_SIZE);
    if (iError > 0)
    {
      return psFilter;
    }
  }

  return NULL;
}


/*-
 ***********************************************************************
 *
 * SupportNewFilter
 *
 ***********************************************************************
 */
FILTER_LIST *
SupportNewFilter(char *pcFilter, char *pcError)
{
  const char          acRoutine[] = "SupportNewFilter()";
  const char         *pcPcreError = NULL;
  FILTER_LIST        *psFilter = NULL;
  int                 iError = 0;
  int                 iCaptureCount = 0;
  int                 iLength = 0;
  int                 iPcreErrorOffset = 0;

  /*-
   *********************************************************************
   *
   * Check that the input filter is not NULL and that it has length.
   *
   *********************************************************************
   */
  if (pcFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NULL input. That shouldn't happen.", acRoutine);
    return NULL;
  }

  iLength = strlen(pcFilter);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length must be greater than zero.", acRoutine, iLength);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory for a new Filter. The caller should free this
   * memory with SupportFreeFilter().
   *
   *********************************************************************
   */
  psFilter = calloc(sizeof(FILTER_LIST), 1);
  if (psFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  psFilter->pcFilter = calloc(iLength + 1, 1);
  if (psFilter->pcFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  strncpy(psFilter->pcFilter, pcFilter, iLength);

  /*-
   *********************************************************************
   *
   * Compile and study the regular expression. Then, make sure that
   * there are no capturing subpatterns. Compile-time options (?imsx)
   * are not set here because the user can specify them as needed in
   * the filter.
   *
   *********************************************************************
   */
  psFilter->psPcre = pcre_compile(pcFilter, 0, &pcPcreError, &iPcreErrorOffset, NULL);
  if (psFilter->psPcre == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: pcre_compile(): %s", acRoutine, pcPcreError);
    SupportFreeFilter(psFilter);
    return NULL;
  }
  psFilter->psPcreExtra = pcre_study(psFilter->psPcre, 0, &pcPcreError);
  if (pcPcreError != NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: pcre_study(): %s", acRoutine, pcPcreError);
    SupportFreeFilter(psFilter);
    return NULL;
  }
  iError = pcre_fullinfo(psFilter->psPcre, psFilter->psPcreExtra, PCRE_INFO_CAPTURECOUNT, (void *) &iCaptureCount);
  if (iError == ER_OK)
  {
    if (iCaptureCount != 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Invalid capture count [%d]. Capturing '()' subpatterns are not allowed in filters. Use '(?:)' if grouping is required.", acRoutine, iCaptureCount);
      SupportFreeFilter(psFilter);
      return NULL;
    }
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: pcre_fullinfo(): Unexpected return value [%d]. That shouldn't happen.", acRoutine, iError);
    SupportFreeFilter(psFilter);
    return NULL;
  }
  psFilter->psNext = NULL;

  return psFilter;
}
#endif
