/*
 ***********************************************************************
 *
 * $Id: support.c,v 1.3 2002/01/29 15:20:06 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "all-includes.h"

#ifdef FTimes_WIN32
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
SupportAddListItem(char *pcPath, FILE_LIST *pHead, char *pcError)
{
  const char          cRoutine[] = "SupportAddListItem()";
  int                 iLength;
  FILE_LIST          *pNewLink,
                     *pCurrent,
                     *pTail;

  iLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length less than 1 byte.", cRoutine, iLength);
    return NULL;
  }
   
  if (iLength > FTIMES_MAX_PATH - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", cRoutine, iLength, FTIMES_MAX_PATH - 1);
    return NULL;
  }

  pTail = pNewLink = (FILE_LIST *) malloc(sizeof(FILE_LIST)); /* The caller must free this storage. */
  if (pNewLink == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * If the head is NULL, create a new link and insert path.
   *
   *********************************************************************
   */
  if ((pCurrent = pHead) == NULL)
  {
    pHead = pNewLink;
  }
  else
  {
    while (pCurrent != NULL)
    {
      if (pCurrent->pNext == NULL)
      {
        pTail = pCurrent;
      }
      pCurrent = pCurrent->pNext;
    }
    pTail->pNext = pNewLink;
  }
  pTail = pNewLink;
  pTail->pNext = NULL;
  strncpy((char *) &pTail->cPath, (char *) pcPath, FTIMES_MAX_PATH);
  return pHead;
}


/*-
 ***********************************************************************
 *
 * SupportAddToList
 *
 ***********************************************************************
 */
int
SupportAddToList(char *pcPath, FILE_LIST **ppList, char *pcError)
{
  const char          cRoutine[] = "SupportAddToList()";
  char                cLocalError[ERRBUF_SIZE];
  char                cPathLocal[FTIMES_MAX_PATH];
  int                 i;
  int                 iLength;
  FILE_LIST           *pHead;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Copy pcPath into cPathLocal removing extra slashes along the way.
   *
   *********************************************************************
   */
  for (i = 0, iLength = 0, memset(cPathLocal, 0, FTIMES_MAX_PATH); i < (int) strlen(pcPath); i++)
  {
    if (i > 0 && pcPath[i] == FTIMES_SLASHCHAR && pcPath[i - 1] == FTIMES_SLASHCHAR)
    {
      continue;
    }
    cPathLocal[iLength++] = pcPath[i];
  }

  /*-
   *********************************************************************
   *
   * If this is not the root directory chop off any trailing slashes.
   *
   *********************************************************************
   */
  if (strcmp(cPathLocal, FTIMES_SLASH) != 0)
  {
    while (cPathLocal[iLength - 1] == FTIMES_SLASHCHAR && iLength > 1)
    {
      cPathLocal[--iLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * If this is not a duplicate item, add it to the list.
   *
   *********************************************************************
   */
  if (SupportMatchExclude(*ppList, cPathLocal) == NULL)
  {
    pHead = SupportAddListItem(cPathLocal, *ppList, cLocalError);
    if (pHead == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER_SupportAddListItem;
    }

    if (*ppList == NULL)
    {
      *ppList = pHead;
    }
  }

  return ER_OK;
}


#ifdef FTimes_WINNT
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
  TOKEN_PRIVILEGES    tokenPrivileges;
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
                                &tokenPrivileges.Privileges[0].Luid
    );

  if (bResult == FALSE)
  {
    return FALSE;
  }

  tokenPrivileges.PrivilegeCount = 1;

  /*-
   *********************************************************************
   *
   * One privilege to set.
   *
   *********************************************************************
   */
  tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

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
                                 &tokenPrivileges,
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
 * SupportDisplayRunStatistics
 *
 ***********************************************************************
 */
void
SupportDisplayRunStatistics(FTIMES_PROPERTIES *psProperties)
{
  char                cMessage[MESSAGE_SIZE];
  time_t              stopTime;

  /*-
   *********************************************************************
   *
   * Stop the RunTime clock. Report Warnings, Failures, and RunTime.
   *
   *********************************************************************
   */
  stopTime = time(NULL);

  snprintf(cMessage, MESSAGE_SIZE, "Warnings=%d", ErrorGetWarnings());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "Failures=%d", ErrorGetFailures());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

  if (psProperties->tStartTime == ER || stopTime == ER)
  {
    snprintf(cMessage, MESSAGE_SIZE, "RunEpoch=NA");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

    snprintf(cMessage, MESSAGE_SIZE, "Duration=NA");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);
  }
  else
  {
    snprintf(cMessage, MESSAGE_SIZE, "RunEpoch=%s %s %s", psProperties->cStartDate, psProperties->cStartTime, psProperties->cStartZone);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

    snprintf(cMessage, MESSAGE_SIZE, "Duration=%d", (int) (stopTime - psProperties->tStartTime));
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);
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
SupportDropListItem(FILE_LIST *pHead, FILE_LIST *pDrop)
{
  FILE_LIST           *pList,
                     *pNewHead;

  pNewHead = pHead;

  if (pDrop == pHead)
  {
    pNewHead = pDrop->pNext;
    free(pDrop);
  }
  else
  {
    for (pList = pHead; pList != NULL; pList = pList->pNext)
    {
      if (pDrop == pList->pNext)
      {
        pList->pNext = pDrop->pNext;
        free(pDrop);
      }
    }
  }
  return pNewHead;
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
  const char          cRoutine[] = "SupportEraseFile()";
#define WIPE_BUFSIZE 0x8000
  char               *cWipe[WIPE_BUFSIZE];
  long                lFileLength,
                      lNWiped;
  FILE               *pFile;

  if ((pFile = fopen(pcName, "rb+")) != NULL)
  {
    fseek(pFile, 0, SEEK_END);
    lFileLength = ftell(pFile);
    rewind(pFile);
    lNWiped = 0;
    memset(cWipe, 0xa5, WIPE_BUFSIZE);
    while (((lNWiped += fwrite(cWipe, 1, WIPE_BUFSIZE, pFile)) <= lFileLength) && !ferror(pFile));
    fclose(pFile);
  }

  if (unlink(pcName) != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER_unlink;
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
  const char          cRoutine[] = "SupportExpandDirectoryPath()";
  char               *pcCwdDir,
                     *pcNewDir,
                     *pcTempPath;
  int                 iError,
                      iLength;

  iLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s], Length = [%d]: Length less than 1 byte.", cRoutine, pcPath, iLength);
    return ER_Length;
  }

  if (iLength > iFullPathSize - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, pcPath, iLength, iFullPathSize - 1);
    return ER_Length;
  }

  pcTempPath = malloc(iLength + 2); /* +1 for a FTIMES_SLASHCHAR, +1 for a NULL */
  if (pcTempPath == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s]: %s", cRoutine, pcPath, strerror(errno));
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s]: %s", cRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    return ER_getcwd;
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s]: %s", cRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER_chdir;
  }

  pcNewDir = getcwd(pcFullPath, iFullPathSize);
  if (pcNewDir == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s]: %s", cRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER_getcwd;
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s]: %s", cRoutine, pcPath, strerror(errno));
    free(pcTempPath);
    free(pcCwdDir); /* Created by getcwd() */
    return ER_chdir;
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
  const char          cRoutine[] = "SupportExpandPath()";
  int                 iError,
                      iLength,
                      iTempLength;
  char                cLocalError[ERRBUF_SIZE],
                     *pcTempFile,
                     *pcTempPath;

  cLocalError[0] = 0;

  iLength = iTempLength = strlen(pcPath);
  if (iLength < 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Length = [%d]: Length less than 1 byte.", cRoutine, pcPath, iLength);
    return ER_Length;
  }

  if (iLength > iFullPathSize - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, pcPath, iLength, iFullPathSize - 1);
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
#ifdef FTimes_WIN32
    if (
         (iLength == 2 && isalpha((int) pcPath[0]) && pcPath[1] == ':') ||
         (iLength >= 3 && isalpha((int) pcPath[0]) && pcPath[1] == ':' && pcPath[2] == FTIMES_SLASHCHAR)
       )
    {
      strncpy(pcFullPath, pcPath, iFullPathSize);
      return ER_OK;
    } 
#endif
#ifdef FTimes_UNIX
    if (pcPath[0] == FTIMES_SLASHCHAR)   
    { 
      strncpy(pcFullPath, pcPath, iFullPathSize);
      return ER_OK;
    }
#endif
  }

  switch (SupportGetFileType(pcPath))
  {
  case FTIMES_FILETYPE_ERROR:
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcPath, strerror(errno));
    return ER_BadValue;
    break;
  
  case FTIMES_FILETYPE_DIRECTORY:
    iError = SupportExpandDirectoryPath(pcPath, pcFullPath, iFullPathSize, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Path = [%s]: %s", cRoutine, pcPath, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcPath, strerror(errno));
      return ER_BadHandle;
    }
  
    pcTempPath = malloc(iLength + 1); /* +1 for a NULL */
    if (pcTempPath == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcPath, strerror(errno));
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
      iError = SupportExpandDirectoryPath(FTIMES_DOT, pcFullPath, iFullPathSize, cLocalError);
    }
    else
    {  
      iError = SupportExpandDirectoryPath(pcTempPath, pcFullPath, iFullPathSize, cLocalError);
    }
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcPath, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: Length would exceed %d bytes.", cRoutine, pcPath, iFullPathSize - 1);
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
 * SupportGetFileType
 *
 ***********************************************************************
 */
int
SupportGetFileType(char *pcPath)
{
  struct stat         statEntry;

#ifdef FTimes_UNIX
  if (lstat(pcPath, &statEntry) == ER)
  {
    return FTIMES_FILETYPE_ERROR;
  }

  switch (statEntry.st_mode & S_IFMT)
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

#ifdef FTimes_WIN32
  char                cWorkingPath[4];

  if ((isalpha((int) pcPath[0]) && pcPath[1] == ':' && pcPath[2] == 0))
  {
    cWorkingPath[0] = pcPath[0];
    cWorkingPath[1] = pcPath[1];
    cWorkingPath[2] = FTIMES_SLASHCHAR;
    cWorkingPath[3] = 0;

    if (stat(cWorkingPath, &statEntry) == ER)
    {
      return FTIMES_FILETYPE_ERROR;
    }
  }
  else
  {
    if (stat(pcPath, &statEntry) == ER)
    {
      return FTIMES_FILETYPE_ERROR;
    }
  }

  switch (statEntry.st_mode & _S_IFMT)
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
  static char         cHostname[MAX_HOSTNAME_LENGTH] = "NA";
#ifdef FTimes_UNIX
  struct utsname      utsName;

  memset(&utsName, 0, sizeof(struct utsname));
  if (uname(&utsName) != -1)
  {
    snprintf(cHostname, MAX_HOSTNAME_LENGTH, "%s", (utsName.nodename[0]) ? utsName.nodename : "NA");
  }
#endif
#ifdef FTimes_WIN32
  char                cTempname[MAX_HOSTNAME_LENGTH];
  DWORD               dwTempNameLength = sizeof(cTempname);

  if (GetComputerName(cTempname, &dwTempNameLength) == TRUE)
  {
    snprintf(cHostname, MAX_HOSTNAME_LENGTH, "%s", cTempname);
  }
#endif
  return cHostname;
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
  static char         cSystemOS[MAX_SYSTEMOS_LENGTH] = "NA";
#ifdef FTimes_UNIX
  struct utsname      utsName;

  memset(&utsName, 0, sizeof(struct utsname));
  if (uname(&utsName) != -1)
  {
#ifdef FTimes_AIX
    snprintf(cSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %s.%s", utsName.machine, utsName.sysname, utsName.version, utsName.release);
#else
    snprintf(cSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %s", utsName.machine, utsName.sysname, utsName.release);
#endif
  }
#endif
#ifdef FTimes_WIN32
  char                cOS[16];
  char                cPlatform[16];
  OSVERSIONINFO       osvi;
  SYSTEM_INFO         si;

  memset(&si, 0, sizeof(SYSTEM_INFO));
  GetSystemInfo(&si);
  switch (si.wProcessorArchitecture)
  {
  case PROCESSOR_ARCHITECTURE_INTEL:
    strncpy(cPlatform, "INTEL", sizeof(cPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_MIPS:
    strncpy(cPlatform, "MIPS", sizeof(cPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_ALPHA:
    strncpy(cPlatform, "ALPHA", sizeof(cPlatform));
    break;
  case PROCESSOR_ARCHITECTURE_PPC:
    strncpy(cPlatform, "PPC", sizeof(cPlatform));
    break;
  default:
    strncpy(cPlatform, "NA", sizeof(cPlatform));
    break;
  }

  memset(&osvi, 0, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (GetVersionEx(&osvi) == TRUE)
  {
    switch (osvi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32s:
      strncpy(cOS, "Windows 3.1", sizeof(cOS));
      break;
    case VER_PLATFORM_WIN32_WINDOWS:
      strncpy(cOS, "Windows 98", sizeof(cOS));
      break;
    case VER_PLATFORM_WIN32_NT:
      strncpy(cOS, "Windows NT", sizeof(cOS));
      break;
    default:
      strncpy(cOS, "NA", sizeof(cOS));
      break;
    }

    snprintf(
              cSystemOS, MAX_SYSTEMOS_LENGTH, "%s %s %u.%u Build %u %s",
              cPlatform,
              cOS,
              osvi.dwMajorVersion,
              osvi.dwMinorVersion,
              osvi.dwBuildNumber,
              osvi.szCSDVersion
            );
  }
#endif
  return cSystemOS;
}


/*-
 ***********************************************************************
 *
 * SupportIncludeEverything
 *
 ***********************************************************************
 */
FILE_LIST *
SupportIncludeEverything(BOOL allowremote, char *pcError)
{
  const char          cRoutine[] = "SupportIncludeEverything()";
  char                cLocalError[ERRBUF_SIZE];
  FILE_LIST           *pHead;

#ifdef FTimes_WIN32
  char                cDriveList[26 * 4 + 2];
  char               *pcDrive;
  int                 iLength;
  int                 iTempLength;

  cLocalError[0] = 0;

  pHead = NULL;

  if (GetLogicalDriveStrings(26 * 4 + 2, cDriveList) == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: GetLogicalDriveStrings: %u", cRoutine, GetLastError());
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
  for (pcDrive = cDriveList; *pcDrive; pcDrive += iLength + 1)
  {
    iLength = iTempLength = strlen(pcDrive);
    while (pcDrive[iTempLength - 1] == FTIMES_SLASHCHAR)
    {
      pcDrive[--iTempLength] = 0;
    }

    pHead = SupportAddListItem(pcDrive, pHead, cLocalError);
    if (pHead == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Include = [%s]: %s", cRoutine, pcDrive, cLocalError);
      return NULL;
    }
  }

  if (pHead == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: No supported drives found.", cRoutine);
  }
#endif
#ifdef FTimes_UNIX

  cLocalError[0] = 0;

  pHead = SupportAddListItem(FTIMES_ROOT_PATH, NULL, cLocalError);
  if (pHead == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Include = [%s]: %s", cRoutine, FTIMES_ROOT_PATH, cLocalError);
    return NULL;
  }
#endif
  return pHead;
}


/*-
 ***********************************************************************
 *
 * SupportMakeName
 *
 ***********************************************************************
 */
int
SupportMakeName(char *pcDir, char *pcBaseName, char *pcDateTime, char *pcExtension, char *pcFilename, char *pcError)
{
  const char          cRoutine[] = "SupportMakeName()";
  int                 iLength;

  iLength  = strlen(pcDir);
  iLength += 1; /* FTIMES_SLASH */
  iLength += strlen(pcBaseName);
  iLength += 1; /* "_" */
  iLength += strlen(pcDateTime);
  iLength += strlen(pcExtension);

  if (iLength > FTIMES_MAX_PATH - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Filename would exceed %d bytes", cRoutine, (FTIMES_MAX_PATH - 1));
    return ER_Length;
  }
  snprintf(pcFilename, FTIMES_MAX_PATH, "%s%s%s_%s%s", pcDir, FTIMES_SLASH, pcBaseName, pcDateTime, pcExtension);

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
SupportMatchExclude(FILE_LIST *pHead, char *pcPath)
{
  FILE_LIST           *pList;

  for (pList = pHead; pList != NULL; pList = pList->pNext)
  {
    if (CompareFunction(pList->cPath, pcPath) == 0)
    {
      return pList;
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
SupportMatchSubTree(FILE_LIST *pHead, FILE_LIST *pTarget)
{
  int                 x,
                      y;
  FILE_LIST           *pList;

  x = strlen(pTarget->cPath);
  for (pList = pHead; pList != NULL; pList = pList->pNext)
  {
    y = strlen(pList->cPath);
    if (NCompareFunction(pTarget->cPath, pList->cPath, MIN(x, y)) == 0)
    {
      if (x <= y)
      {
        if ((pList->cPath[x - 1] == FTIMES_SLASHCHAR && pList->cPath[x] != FTIMES_SLASHCHAR) ||
            (pList->cPath[x - 1] != FTIMES_SLASHCHAR && pList->cPath[x] == FTIMES_SLASHCHAR) ||
            (pList->cPath[x - 1] == FTIMES_SLASHCHAR && pList->cPath[x] == FTIMES_SLASHCHAR))
        {
          return pList;
        }
      }
      else
      {
        if ((pTarget->cPath[y - 1] == FTIMES_SLASHCHAR && pTarget->cPath[y] != FTIMES_SLASHCHAR) ||
            (pTarget->cPath[y - 1] != FTIMES_SLASHCHAR && pTarget->cPath[y] == FTIMES_SLASHCHAR) ||
            (pTarget->cPath[y - 1] == FTIMES_SLASHCHAR && pTarget->cPath[y] == FTIMES_SLASHCHAR))
        {
          return pTarget;
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
  const char          cRoutine[] = "SupportNeuterString()";
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  pcNeutered[0] = 0;
         
  /*-
   *********************************************************************
   *
   * Neuter !isprint and [|"'`%+]. Convert spaces to '+'.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    if (isprint((int) pcData[i]))
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
    else
    {
      n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
    }
  }
  pcNeutered[n] = 0;

  return pcNeutered;
}


#ifdef FTimes_WIN32
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
  const char          cRoutine[] = "SupportNeuterStringW()";
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
SupportPruneList(FILE_LIST *pList, BOOL bMapRemoteFiles)
{
  const char          cRoutine[] = "SupportPruneList()";
  char                cLocalError[ERRBUF_SIZE];
  FILE_LIST           *pListHead,
                     *pListTree,
                     *pListKill;

  if (pList == NULL)
  {
    return NULL;
  }
  else
  {
    pListHead = pList;
  }

  /*-
   *********************************************************************
   *
   * Eliminate any subtree components from the tree list. For example,
   * /usr/local would be eliminated from /usr. We also eliminate any
   * duplicate entries in the process.
   *
   *********************************************************************
   */
  for (pListTree = pListHead; pListTree->pNext != NULL;)
  {
    pListKill = SupportMatchSubTree(pListTree->pNext, pListTree);

    /*-
     *******************************************************************
     *
     * When a match is found, we have to perform another search to
     * ensure that there are no more matches further down in the list.
     * This is done implicitly by setting pListTree to pListHead after
     * the drop.
     *
     *******************************************************************
     */
    if (pListKill != NULL)
    {
      snprintf(cLocalError, ERRBUF_SIZE, "%s: Item = [%s]: Dropping redundant entry.", cRoutine, pListKill->cPath);
      ErrorHandler(ER_Warning, cLocalError, ERROR_WARNING);
      pListHead = SupportDropListItem(pListHead, pListKill);
      pListTree = pListHead;
    }
    else
    {
      pListTree = pListTree->pNext;
    }
  }
  return pListHead;
}


/*-
 ***********************************************************************
 *
 * RequirePrivilege
 *
 ***********************************************************************
 */
int
SupportRequirePrivilege(char *pcError)
{
  const char          cRoutine[] = "SupportRequirePrivilege()";

#ifdef FTimes_WINNT

  char                cLocalError[ERRBUF_SIZE];

  if (SupportSetPrivileges(cLocalError) != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER_NoPrivilege;
  }
#endif

#ifdef FTimes_UNIX
  if (getuid() != 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Need root privilege to continue.", cRoutine);
    return ER_NoPrivilege;
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
  const char          cRoutine[] = "SupportSetLogLevel()";

  if ((int) strlen(pcLevel) != 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: LogLevel must be %d-%d", cRoutine, MESSAGE_DEBUGGER, MESSAGE_CRITICAL);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: LogLevel must be %d-%d", cRoutine, MESSAGE_DEBUGGER, MESSAGE_CRITICAL);
    return ER_Length;
    break;
  }
  return ER_OK;
}


#ifdef FTimes_WINNT
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
  const char          cRoutine[] = "SupportSetPrivileges()";

  /*-
   *********************************************************************
   *
   * Attempt to obtain backup user rights.
   *
   *********************************************************************
   */
  if (SupportAdjustPrivileges(SE_BACKUP_NAME) == FALSE)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Can't set SE_BACKUP_NAME privilege.", cRoutine);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Can't set SE_RESTORE_NAME privilege.", cRoutine);
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
  const char          cRoutine[] = "SupportWriteData()";
  int                 iNWritten;

  iNWritten = fwrite(pcData, 1, iLength, pFile);
  if (iNWritten != iLength)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER_fwrite;
  }
  fflush(pFile);
  return ER_OK;
}
