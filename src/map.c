/*
 ***********************************************************************
 *
 * $Id: map.c,v 1.8 2002/09/09 05:38:58 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "all-includes.h"

static int giDirectories;
static int giFiles;
static int giSpecial;
#ifdef FTimes_WINNT
static int giStreams;
#endif

static int giRecords;
static int giIncompleteRecords;

/*-
 ***********************************************************************
 *
 * MapGetDirectoryCount
 *
 ***********************************************************************
 */
int
MapGetDirectoryCount()
{
  return giDirectories;
}


/*-
 ***********************************************************************
 *
 * MapGetFileCount
 *
 ***********************************************************************
 */
int
MapGetFileCount()
{
  return giFiles;
}


/*-
 ***********************************************************************
 *
 * MapGetSpecialCount
 *
 ***********************************************************************
 */
int
MapGetSpecialCount()
{
  return giSpecial;
}


#ifdef FTimes_WINNT
/*-
 ***********************************************************************
 *
 * MapGetStreamCount
 *
 ***********************************************************************
 */
int
MapGetStreamCount()
{
  return giStreams;
}
#endif


/*-
 ***********************************************************************
 *
 * MapGetRecordCount
 *
 ***********************************************************************
 */
int
MapGetRecordCount()
{
  return giRecords;
}


/*-
 ***********************************************************************
 *
 * MapGetIncompleteRecordCount
 *
 ***********************************************************************
 */
int
MapGetIncompleteRecordCount()
{
  return giIncompleteRecords;
}


#ifdef FTimes_UNIX
/*-
 ***********************************************************************
 *
 * MapTree
 *
 ***********************************************************************
 */
int
MapTree(FTIMES_PROPERTIES *psProperties, char *pcPath, int iFSType, unsigned char *pucTreeHash, char *pcError)
{
  const char          cRoutine[] = "MapTree()";
  char                cLocalError[ERRBUF_SIZE];
  char                cLinkData[FTIMES_MAX_PATH];
  char                cMessage[MESSAGE_SIZE];
  char                cNewRawPath[FTIMES_MAX_PATH];
  char                cParentPath[FTIMES_MAX_PATH];
  char               *pc;
  char               *pcNeuteredPath;
  DIR                *pDir;
  FTIMES_FILE_DATA    sFTFileData;
  int                 iError;
  int                 iNewFSType;
  int                 iNameLength;
  int                 iPathLength;
  int                 iParentPathLength;
  struct dirent      *pDirEntry;
  struct hash_block   dirHashBlock;
  struct stat         sStatPDirectory, *psStatPDirectory;
  struct stat         sStatCDirectory, *psStatCDirectory;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Abort if the path length is too long.
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  if (iPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 1);
    ErrorHandler(ER_Length, pcError, ERROR_CRITICAL);
  }

  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(cMessage, MESSAGE_SIZE, "FS=%s Directory=%s", FSType[iFSType], pcPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, cMessage);
  }

  /*-
   *********************************************************************
   *
   * If directory hashing is enabled, initialize dirHashBlock.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    md5_begin(&dirHashBlock);
  }

  /*-
   *********************************************************************
   *
   * Chop off trailing slashes for non root directories.
   *
   *********************************************************************
   */
  iParentPathLength = iPathLength;
  pc = strncpy(cParentPath, pcPath, FTIMES_MAX_PATH);
  if (strcmp(cParentPath, FTIMES_SLASH) != 0)
  {
    while (pc[iParentPathLength - 1] == FTIMES_SLASHCHAR)
    {
      pc[--iParentPathLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * Chop off the trailing directory name for non root directories.
   *
   *********************************************************************
   */
  if (strcmp(cParentPath, FTIMES_SLASH) != 0)
  {
    while (pc[iParentPathLength - 1] != FTIMES_SLASHCHAR)
    {
      pc[--iParentPathLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * Determine current and parent path attributes. These are used later
   * to check that "." and ".." are really hard links to the current
   * and parent paths, respectively. This is done to ensure that "."
   * or ".." don't point to a hidden file or directory. The attacker
   * would necessarily have to modify the directory structure for this
   * to be the case.
   *
   *********************************************************************
   */
  if (lstat(cParentPath, &sStatPDirectory) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: ParentPathR = [%s]: %s", cRoutine, cParentPath, strerror(errno));
    ErrorHandler(ER_lstat, pcError, ERROR_FAILURE);
    psStatPDirectory = NULL;
  }
  else
  {
    psStatPDirectory = &sStatPDirectory;
  }

  if (lstat(pcPath, &sStatCDirectory) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: CurrentPathR = [%s]: %s", cRoutine, pcPath, strerror(errno));
    ErrorHandler(ER_lstat, pcError, ERROR_FAILURE);
    psStatCDirectory = NULL;
  }
  else
  {
    psStatCDirectory = &sStatCDirectory;
  }

  /*-
   *********************************************************************
   *
   * Open up the directory to be scanned.
   *
   *********************************************************************
   */
  if ((pDir = opendir(pcPath)) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, pcPath, strerror(errno));
    ErrorHandler(ER_opendir, pcError, ERROR_FAILURE);
    return ER_opendir;
  }

  /*-
   *********************************************************************
   *
   * Note: errno is cleared before each readdir() call so that it's
   * value can be checked after the function returns. Read the comment
   * that follows this loop for more details.
   *
   *********************************************************************
   */
  errno = 0;

  /*-
   *********************************************************************
   *
   * Loop through the list of directory entries. Each time through this
   * loop, clear the contents of sFTFileData -- subsequent logic
   * relies on the assertion that ucFileMD5 has been initialized to all
   * zeros.
   *
   *********************************************************************
   */
  while ((pDirEntry = readdir(pDir)) != NULL)
  {
    memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
    sFTFileData.iFSType = iFSType;

    /*-
     *******************************************************************
     *
     * Figure out if the new path length will be too long. If yes, warn
     * the user, clear errno, and continue with the next file.
     *
     *******************************************************************
     */
    iNameLength = iPathLength + 1 + strlen(pDirEntry->d_name);
    if (iPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Path = [%s/%s], Length = [%d]: Length would exceed %d bytes.", cRoutine, pcPath, pDirEntry->d_name, iNameLength, FTIMES_MAX_PATH - 1);
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      errno = 0;
      continue;
    }

    /*-
     *******************************************************************
     *
     * Create the new path. If pcPath has a trailing slash, subtract
     * one from the total path length. In general, this should only
     * happen when pcPath = "/".
     *
     *******************************************************************
     */
    strncpy(cNewRawPath, pcPath, FTIMES_MAX_PATH);
    if (pcPath[iPathLength - 1] != FTIMES_SLASHCHAR)
    {
      cNewRawPath[iPathLength] = FTIMES_SLASHCHAR;
      strcat(&cNewRawPath[iPathLength + 1], pDirEntry->d_name);
    }
    else
    {
      iNameLength--;
      strcat(&cNewRawPath[iPathLength], pDirEntry->d_name);
    }
    sFTFileData.pcRawPath = cNewRawPath;

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, then continue with the
     * next entry. Remember to clear errno.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->ptExcludeList, cNewRawPath) != NULL)
    {
      errno = 0;
      continue;
    }

    /*-
     *******************************************************************
     *
     * Neuter the given path. In other words replace funky chars with
     * their hex value (e.g., backspace becomes %08).
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(cNewRawPath, iNameLength, cLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, cNewRawPath, cLocalError);
      ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
      errno = 0;
      continue;
    }
    sFTFileData.pcNeuteredPath = pcNeuteredPath;

    /*-
     *******************************************************************
     *
     * Collect attributes, but don't follow symbolic links.
     *
     *******************************************************************
     */
    iError = MapGetAttributes(&sFTFileData, cLocalError);
    if (iError == Have_Nothing)
    {
      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, UPDATE THE DIRECTORY HASH WITH A HASH OF ALL
       * ZEROS.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
      {
        md5_middle(&dirHashBlock, sFTFileData.ucFileMD5, MD5_HASH_LENGTH);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      /*-
       *****************************************************************
       *
       * Free the neutered path.
       *
       *****************************************************************
       */
      SupportFreeData(pcNeuteredPath);

      errno = 0;
      continue;
    }

    /*-
     *******************************************************************
     *
     * If this is "." and it has the same inode as the current path,
     * fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * If this is ".." and it has the same inode as the parent path,
     * fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * Otherwise, attempt to process the record.
     *
     *******************************************************************
     */
    if (strcmp(pDirEntry->d_name, FTIMES_DOT) == 0)
    {
      if (psStatCDirectory != NULL)
      {
        if (sFTFileData.statEntry.st_ino != sStatCDirectory.st_ino)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Inode mismatch between '.' and current directory.", cRoutine, pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on current directory to compare it to '.'.", cRoutine, pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else if (strcmp(pDirEntry->d_name, FTIMES_DOTDOT) == 0)
    {
      if (psStatPDirectory != NULL)
      {
        if (sFTFileData.statEntry.st_ino != sStatPDirectory.st_ino)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Inode mismatch between '..' and parent directory.", cRoutine, pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on parent directory to compare it to '..'.", cRoutine, pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else
    {
      /*-
       *****************************************************************
       *
       * Map directories, files, and links.
       *
       *****************************************************************
       */
      if (S_ISDIR(sFTFileData.statEntry.st_mode))
      {
        giDirectories++;
#ifdef USE_XMAGIC
        if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
        {
          snprintf(sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, "directory");
        }
#endif
        /*-
         ***************************************************************
         *
         * Check for a device crossing. Process new file systems if
         * they are supported, and process remote file systems when
         * MapRemoteFiles is enabled.
         *
         ***************************************************************
         */
        if (psStatCDirectory != NULL)
        {
          if (sFTFileData.statEntry.st_dev != sStatCDirectory.st_dev)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Crossing a device boundary.", cRoutine, pcNeuteredPath);
            ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
            /*-
             *************************************************************
             *
             * WHEN AN ERROR OCCURS OR THE FILE SYSTEM IS UNSUPPORTED,
             * WARN THE USER AND CONTINUE. IF THE FILE SYSTEM IS NFS AND
             * REMOTE SCANNING IS NOT ENABLED, WARN THE USER AND CONTINUE.
             * IF ONE OF THESE CASES SHOULD ARISE, DO NOT WRITE A ENTRY
             * IN THE OUTPUT FILE, AND DO NOT UPDATE THE DIRECTORY HASH.
             *
             *************************************************************
             */
            iNewFSType = GetFileSystemType(cNewRawPath, cLocalError);
            if (iNewFSType == ER)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              SupportFreeData(pcNeuteredPath);
              errno = 0;
              continue;
            }
            if (iNewFSType == FSTYPE_UNSUPPORTED)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding unsupported file system.", cRoutine, pcNeuteredPath);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              SupportFreeData(pcNeuteredPath);
              errno = 0;
              continue;
            }
            if (iNewFSType == FSTYPE_NFS && !psProperties->bMapRemoteFiles)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding remote file system.", cRoutine, pcNeuteredPath);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              SupportFreeData(pcNeuteredPath);
              errno = 0;
              continue;
            }
            iFSType = iNewFSType;
          }
        }
        else
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on current directory to determine a device boundary crossing.", cRoutine, pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
          iFSType = FSTYPE_NA;
        }
        MapTree(psProperties, cNewRawPath, iFSType, sFTFileData.ucFileMD5, cLocalError);
      }
      else if (S_ISREG(sFTFileData.statEntry.st_mode))
      {
        giFiles++;
        if (psProperties->iLastAnalysisStage > 0)
        {
          iError = AnalyzeFile(psProperties, &sFTFileData, cLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
      }
      else if (S_ISLNK(sFTFileData.statEntry.st_mode))
      {
        giSpecial++;
        if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
        {
          iError = readlink(cNewRawPath, cLinkData, FTIMES_MAX_PATH - 1);
          if (iError == ER)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Unreadable Symbolic Link: %s", cRoutine, pcNeuteredPath, strerror(errno));
            ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
          }
          else
          {
            cLinkData[iError] = 0; /* Readlink does not append a NULL. */
            md5_string((unsigned char *) cLinkData, strlen(cLinkData), sFTFileData.ucFileMD5);
          }
        }
#ifdef USE_XMAGIC
        if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
        {
          iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.statEntry, sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, cLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }
      else
      {
        giSpecial++;
#ifdef USE_XMAGIC
        if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
        {
          iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.statEntry, sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, cLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, UPDATE THE DIRECTORY HASH WITH A HASH OF ALL
       * ZEROS.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
      {
        md5_middle(&dirHashBlock, sFTFileData.ucFileMD5, MD5_HASH_LENGTH);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path.
     *
     *******************************************************************
     */
    SupportFreeData(pcNeuteredPath);

    errno = 0;
  }

  /*-
   *********************************************************************
   *
   * A NULL could mean EOD or error. The question is whether or not
   * errno is set by readdir(). We explicitly set errno to 0 before
   * each call to readdir(). So, in theory if readdir() actually
   * fails, then errno should be something other than 0. Unfortunately,
   * linux and freebsd man pages aren't explicit about their return
   * values for readdir().
   *
   *********************************************************************
   */
  if (pDirEntry == NULL && errno != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, pcPath, strerror(errno));
    ErrorHandler(ER_readdir, pcError, ERROR_FAILURE);
  }

  /*-
   *********************************************************************
   *
   * Complete the directory hash. Store the result in pucTreeHash.
   * NOTE: pucTreeHash must be initialized to zero by the caller.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    md5_end(&dirHashBlock, pucTreeHash);
  }

  closedir(pDir);
  return ER_OK;
}
#endif


#ifdef FTimes_WIN32
/*-
 ***********************************************************************
 *
 * MapGetFileHandle
 *
 ***********************************************************************
 */
HANDLE
MapGetFileHandle(char *pcPath)
{
  /*-
   *********************************************************************
   *
   * This is just a wrapper for CreateFile(). The caller must check
   * the return value to ensure that no error has occurred. Directories
   * specifically require the FILE_FLAG_BACKUP_SEMANTICS flag. This
   * does not seem to affect the opening of regular files.
   *
   *********************************************************************
   */
  return CreateFile(
                     pcPath,
                     0 /* 0 = device query access */,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                     NULL
                   );
}


/*-
 ***********************************************************************
 *
 * MapTree
 *
 ***********************************************************************
 */
int
MapTree(FTIMES_PROPERTIES *psProperties, char *pcPath, int iFSType, unsigned char *pucTreeHash, char *pcError)
{
  const char          cRoutine[] = "MapTree()";
  BOOL                bResult;
  BY_HANDLE_FILE_INFORMATION
                      fileInfoCurrent;
  BY_HANDLE_FILE_INFORMATION
                      fileInfoParent;
  char                cLocalError[ERRBUF_SIZE];
  char                cMessage[MESSAGE_SIZE];
  char                cNewRawPath[FTIMES_MAX_PATH];
  char                cParentPath[FTIMES_MAX_PATH];
  char                cSearchPath[FTIMES_MAX_PATH];
  char               *pc;
  char               *pcNeuteredPath;
  char               *pcMessage;
  FTIMES_FILE_DATA    sFTFileData;
  HANDLE              hFileCurrent;
  HANDLE              hFileParent;
  HANDLE              hSearch;
  int                 iError;
  int                 iNameLength;
  int                 iParentPathLength;
  int                 iPathLength;
  struct hash_block   dirHashBlock;
  WIN32_FIND_DATA     findData;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Abort if the path length is too long.
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  if (iPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 1);
    ErrorHandler(ER_Length, pcError, ERROR_CRITICAL);
  }

  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(cMessage, MESSAGE_SIZE, "FS=%s Directory=%s", FSType[iFSType], pcPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, cMessage);
  }

  /*-
   *********************************************************************
   *
   * If directory hashing is enabled, initialize dirHashBlock.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    md5_begin(&dirHashBlock);
  }

  /*-
   *********************************************************************
   *
   * Chop off trailing slashes for non root directories.
   *
   *********************************************************************
   */
  iParentPathLength = iPathLength;
  pc = strncpy(cParentPath, pcPath, FTIMES_MAX_PATH);
  if (!(iParentPathLength == 3 && pc[1] == ':' && pc[2] == FTIMES_SLASHCHAR && isalpha(pc[0])))
  {
    while (pc[iParentPathLength - 1] == FTIMES_SLASHCHAR)
    {
      pc[--iParentPathLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * Chop off the trailing directory name for non root directories.
   *
   *********************************************************************
   */
  if (!(iParentPathLength == 2 && pc[1] == ':' && isalpha(pc[0])))
  {
    while (pc[iParentPathLength - 1] != FTIMES_SLASHCHAR)
    {
      pc[--iParentPathLength] = 0;
    }
  }

  /*-
   *********************************************************************
   *
   * Create the search path. Append a backslash and asterisk to match
   * all files (i.e. "\*").
   *
   *********************************************************************
   */
  if (iPathLength > FTIMES_MAX_PATH - 3)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: SearchPath = [%s\\*], Length = [%d]: Length would exceed %d bytes.", cRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 3);
    ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
    return ER_Length;
  }

  strncpy(cSearchPath, pcPath, FTIMES_MAX_PATH);
  cSearchPath[iPathLength] = FTIMES_SLASHCHAR;
  cSearchPath[iPathLength + 1] = '*';
  cSearchPath[iPathLength + 2] = 0;

  /*-
   *********************************************************************
   *
   * Begin the search.
   *
   *********************************************************************
   */
  hSearch = FindFirstFile(cSearchPath, &findData);

  if (hSearch == INVALID_HANDLE_VALUE)
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, pcPath, pcMessage);
    ErrorHandler(ER_FindFirstFile, pcError, ERROR_FAILURE);
    return ER_FindFirstFile;
  }

  /*-
   *********************************************************************
   *
   * Loop through the list of directory entries. Each time through this
   * loop, clear the contents of sFTFileData -- subsequent logic
   * relies on the assertion that ucFileMD5 has been initialized to all
   * zeros.
   *
   *********************************************************************
   */
  bResult = TRUE;
  while (bResult == TRUE)
  {
    memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
    sFTFileData.dwVolumeSerialNumber = -1;
    sFTFileData.nFileIndexHigh = -1;
    sFTFileData.nFileIndexLow = -1;
    sFTFileData.iStreamCount = -1; /* Develop routines check for this value. */
    sFTFileData.iFSType = iFSType;

#ifdef FTimes_WIN98

    /*-
     *******************************************************************
     *
     * Break out early for WIN98 as it can't open "." or ".."
     *
     *******************************************************************
     */
    if (strcmp(findData.cFileName, FTIMES_DOT) == 0)
    {
      bResult = FindNextFile(hSearch, &findData);
      continue;
    }

    if (strcmp(findData.cFileName, FTIMES_DOTDOT) == 0)
    {
      bResult = FindNextFile(hSearch, &findData);
      continue;
    }
#endif

    /*-
     *******************************************************************
     *
     * Figure out if the new path length will be too long. If yes, warn
     * the user, and continue with the next file.
     *
     *******************************************************************
     */
    iNameLength = iPathLength + 1 + (int) strlen(findData.cFileName);
    if (iPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Path = [%s\\%s], Length = [%d]: Length would exceed %d bytes.", cRoutine, pcPath, findData.cFileName, iNameLength, FTIMES_MAX_PATH - 1);
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      bResult = FindNextFile(hSearch, &findData);
      continue;
    }

    strncpy(cNewRawPath, pcPath, FTIMES_MAX_PATH);
    cNewRawPath[iPathLength] = FTIMES_SLASHCHAR;
    strcpy(&cNewRawPath[iPathLength + 1], findData.cFileName);
    sFTFileData.pcRawPath = cNewRawPath;

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, then continue with the
     * next entry.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->ptExcludeList, cNewRawPath) != NULL)
    {
      bResult = FindNextFile(hSearch, &findData);
      continue;
    }

    /*-
     *******************************************************************
     *
     * Neuter the given path. In other words, replace funky chars with
     * their hex value (e.g., backspace becomes %08).
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(cNewRawPath, iNameLength, cLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, cNewRawPath, cLocalError);
      ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
      bResult = FindNextFile(hSearch, &findData);
      continue;
    }
    sFTFileData.pcNeuteredPath = pcNeuteredPath;

    /*-
     *******************************************************************
     *
     * Collect attributes.
     *
     *******************************************************************
     */
    iError = MapGetAttributes(&sFTFileData, cLocalError);
    if (iError == Have_Nothing)
    {
      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, UPDATE THE DIRECTORY HASH WITH A HASH OF ALL
       * ZEROS.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
      {
        md5_middle(&dirHashBlock, sFTFileData.ucFileMD5, MD5_HASH_LENGTH);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      /*-
       *****************************************************************
       *
       * Free the neutered path.
       *
       *****************************************************************
       */
      SupportFreeData(pcNeuteredPath);

      bResult = FindNextFile(hSearch, &findData);
      continue;
    }

    /*-
     *******************************************************************
     *
     * If this is "." and it has the same volume and file index as the
     * current path, fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * If this is ".." and it has the same volume and file index as the
     * parent path, fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * Otherwise, attempt to process the record.
     *
     *******************************************************************
     */
    if (strcmp(findData.cFileName, FTIMES_DOT) == 0)
    {
      if (sFTFileData.iFileFlags >= Have_GetFileInformationByHandle)
      {
        hFileCurrent = MapGetFileHandle(pcPath);
        if (hFileCurrent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileCurrent, &fileInfoCurrent))
        {
          if (fileInfoCurrent.dwVolumeSerialNumber != sFTFileData.dwVolumeSerialNumber ||
              fileInfoCurrent.nFileIndexHigh != sFTFileData.nFileIndexHigh ||
              fileInfoCurrent.nFileIndexLow != sFTFileData.nFileIndexLow)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Volume/FileIndex mismatch between '.' and current directory.", cRoutine, pcNeuteredPath);
            ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
          }
          CloseHandle(hFileCurrent);
        }
        else
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on current directory to compare it to '.'.", cRoutine, pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on '.' to compare it to current directory.", cRoutine, pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else if (strcmp(findData.cFileName, FTIMES_DOTDOT) == 0)
    {
      /*-
       *****************************************************************
       *
       * If the file system is remote, skip this check. This is done
       * because, in testing, the file index for '..' is different than
       * the parent directory. This was found to be true with NTFS and
       * Samba shares which, by-the-way, show up as NTFS_REMOTE. For
       * now these quirks remain unexplained.
       *
       *****************************************************************
       */
      if (iFSType != FSTYPE_NTFS_REMOTE && iFSType != FSTYPE_FAT_REMOTE)
      {
        if (sFTFileData.iFileFlags >= Have_GetFileInformationByHandle)
        {
          hFileParent = MapGetFileHandle(cParentPath);
          if (hFileParent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileParent, &fileInfoParent))
          {
            if (fileInfoParent.dwVolumeSerialNumber != sFTFileData.dwVolumeSerialNumber ||
                fileInfoParent.nFileIndexHigh != sFTFileData.nFileIndexHigh ||
                fileInfoParent.nFileIndexLow != sFTFileData.nFileIndexLow)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Volume/FileIndex mismatch between '..' and parent directory.", cRoutine, pcNeuteredPath);
              ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
            }
            CloseHandle(hFileParent);
          }
          else
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on parent directory to compare it to '..'.", cRoutine, pcNeuteredPath);
            ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
          }
        }
        else
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Not enough information on '..' to compare it to parent directory.", cRoutine, pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
        }
      }
    }
    else
    {
      /*-
       *****************************************************************
       *
       * Map directories and files.
       *
       *****************************************************************
       */
      if ((sFTFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
      {
        giDirectories++;
#ifdef USE_XMAGIC
        if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
        {
          snprintf(sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, "directory");
        }
#endif
        MapTree(psProperties, cNewRawPath, iFSType, sFTFileData.ucFileMD5, cLocalError);
      }
      else
      {
        giFiles++;
        if (sFTFileData.iFileFlags >= Have_MapGetFileHandle)
        {
          if (psProperties->iLastAnalysisStage > 0)
          {
            iError = AnalyzeFile(psProperties, &sFTFileData, cLocalError);
            if (iError != ER_OK)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
              ErrorHandler(iError, pcError, ERROR_FAILURE);
            }
          }
        }
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, sixteen zeros will be folded into the aggregate
       * hash.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
      {
        md5_middle(&dirHashBlock, sFTFileData.ucFileMD5, MD5_HASH_LENGTH);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

#ifdef FTimes_WINNT
      /*-
       *****************************************************************
       *
       * Process any alternate streams. This applies only to NTFS.
       *
       *****************************************************************
       */
      if (sFTFileData.iStreamCount > 0)
      {
        MapStream(psProperties, &sFTFileData, &dirHashBlock, cLocalError);
      }
      if (sFTFileData.pucStreamInfo)
      {
        free(sFTFileData.pucStreamInfo);
      }
#endif

      /*-
       *****************************************************************
       *
       * Free the neutered path.
       *
       *****************************************************************
       */
      SupportFreeData(pcNeuteredPath);

    }
    bResult = FindNextFile(hSearch, &findData);
  }

  /*-
   *********************************************************************
   *
   * Complete the directory hash. Store the result in pucTreeHash.
   * NOTE: pucTreeHash must be initialized to zero by the caller.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    md5_end(&dirHashBlock, pucTreeHash);
  }

  FindClose(hSearch);
  return ER_OK;
}


#ifdef FTimes_WINNT
/*-
 ***********************************************************************
 *
 * MapStream
 *
 ***********************************************************************
 */
void
MapStream(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTData, struct hash_block *pDirHashBlock, char *pcError)
{
  const char          cRoutine[] = "MapStream()";
  char                cLocalError[ERRBUF_SIZE];
  char                cNewRawPath[FTIMES_MAX_PATH];
  char                cStreamName[FTIMES_MAX_PATH];
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  FILE_STREAM_INFORMATION
                     *pFSI;
  int                 i;
  int                 iError;
  int                 iLength;
  int                 iNameLength;
  int                 iNextEntryOffset;
  unsigned short      usUnicode;

  cLocalError[0] = 0;

  pFSI = (FILE_STREAM_INFORMATION *) psFTData->pucStreamInfo;

  giStreams += psFTData->iStreamCount;

  /*-
   *********************************************************************
   *
   * Make a local copy of the file data. If the stream belongs to
   * a directory, clear the attributes field. If this isn't done,
   * the output routine, will overwrite the MD5 field with "DIRECTORY"
   * or "D" respectively.
   *
   *********************************************************************
   */
  memcpy(&sFTFileData, psFTData, sizeof(FTIMES_FILE_DATA));

  sFTFileData.pcRawPath = cNewRawPath;

  sFTFileData.iStreamCount = 0;

  if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
  {
    sFTFileData.dwFileAttributes = 0;
  }

  /*-
   *********************************************************************
   *
   * Process each stream, but skip the default stream.
   *
   *********************************************************************
   */
  for (iNextEntryOffset = 0; pFSI->NextEntryOffset; iNextEntryOffset = pFSI->NextEntryOffset)
  {
    pFSI = (FILE_STREAM_INFORMATION *) ((byte *) pFSI + iNextEntryOffset);

    /*-
     *******************************************************************
     *
     * The string :$DATA isn't part of the the stream's name as stored
     * on disk in the MFT. For this reason, it is chopped off here. If
     * is not chopped off, normal filenames (i.e. not the ones prefixed
     * with '\\?\') can exceed MAX_PATH.
     *
     *******************************************************************
     */
    iLength = pFSI->StreamNameLength / sizeof(WCHAR);
    if (
         pFSI->StreamName[iLength - 6] == (WCHAR) ':' &&
         pFSI->StreamName[iLength - 5] == (WCHAR) '$' &&
         pFSI->StreamName[iLength - 4] == (WCHAR) 'D' &&
         pFSI->StreamName[iLength - 3] == (WCHAR) 'A' &&
         pFSI->StreamName[iLength - 2] == (WCHAR) 'T' &&
         pFSI->StreamName[iLength - 1] == (WCHAR) 'A'
       )
    {
      iLength -= 6;
      if (pFSI->StreamName[iLength - 1] == (WCHAR) ':')
      {
        continue;
      }
    }

    for (i = 0; i < iLength; i++)
    {
      usUnicode = (unsigned short)pFSI->StreamName[i];
      if (usUnicode < 0x0020 || usUnicode > 0x007e)
      {
        break;
      }
      cStreamName[i] = (char)(usUnicode & 0xff);
    }
    if (i != iLength)
    {
      char *pcStream = SupportNeuterStringW(pFSI->StreamName, iLength, cLocalError);
      snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s], StreamWN = [%s]: Stream skipped because it's name contains Unicode or Unsafe characters.", cRoutine, psFTData->pcNeuteredPath, (pcStream == NULL) ? "" : pcStream);
      if (pcStream)
      {
        free(pcStream);
      }
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      continue;
    }
    cStreamName[i] = 0;

    /*-
     *******************************************************************
     *
     * Figure out if the new path length will be too long. If yes, warn
     * the user, and continue with the next stream.
     *
     *******************************************************************
     */
    iNameLength = strlen(psFTData->pcRawPath) + iLength;
    if (iNameLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(
                pcError,
                ERRBUF_SIZE,
                "%s: PathN = [%s], Length = [%d]: Length including alternate stream name would exceed %d bytes.",
                cRoutine,
                psFTData->pcNeuteredPath,
                iNameLength,
                FTIMES_MAX_PATH - 1
              );
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      continue;
    }
    sprintf(cNewRawPath, "%s%s", psFTData->pcRawPath, cStreamName);
    sFTFileData.pcRawPath = cNewRawPath;

    /*-
     *******************************************************************
     *
     * Neuter the new given path.
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(cNewRawPath, iNameLength, cLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, cNewRawPath, cLocalError);
      ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
      continue;
    }
    sFTFileData.pcNeuteredPath = pcNeuteredPath;

    /*-
     *******************************************************************
     *
     * Set the file attributes that are unique to this stream.
     *
     *******************************************************************
     */
    sFTFileData.nFileSizeHigh = (DWORD) (pFSI->StreamSize.QuadPart >> 32);
    sFTFileData.nFileSizeLow = (DWORD) pFSI->StreamSize.QuadPart;

    /*-
     *******************************************************************
     *
     * Analyze the stream's content.
     *
     *******************************************************************
     */
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }

    /*-
     *******************************************************************
     *
     * Update the directory hash. If the file was special or could
     * not be hashed, sixteen zeros will be folded into the aggregate
     * hash.
     *
     *******************************************************************
     */
    if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
    {
      md5_middle(pDirHashBlock, sFTFileData.ucFileMD5, MD5_HASH_LENGTH);
    }

    /*-
     *******************************************************************
     *
     * Record the collected data.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path.
     *
     *******************************************************************
     */
    SupportFreeData(pcNeuteredPath);
  }
}


/*-
 ***********************************************************************
 *
 * MapCountNamedStreams
 *
 ***********************************************************************
 */
int
MapCountNamedStreams(HANDLE hFile, int *piStreamCount, unsigned char **ppucStreamInfo, char *pcError)
{
  const char          cRoutine[] = "MapCountNamedStreams()";
  char               *pcMessage;
  DWORD               dwStatus;
  FILE_STREAM_INFORMATION
                     *pLastFSI;
  FILE_STREAM_INFORMATION
                     *pThisFSI;
  int                 i;
  int                 iStatus;
  int                 iStreamCount;
  int                 iNextEntryOffset;
  IO_STATUS_BLOCK     ioStatusBlock;
  unsigned long       ulSize;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x00000000
#endif
#ifndef STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW 0x80000005
#endif

  *piStreamCount = -1; /* Develop routines check for this value. */
  *ppucStreamInfo = NULL;

  if (hFile == INVALID_HANDLE_VALUE)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Invalid File Handle", cRoutine);
    return FTIMES_EMPTY_STREAM_COUNT;
  }

  /*-
   *********************************************************************
   *
   * Query up the stream information.
   *
   *********************************************************************
   */
  pLastFSI = pThisFSI = NULL;
  iStatus = FTIMES_FULL_STREAM_COUNT; /* Plan for success. */
  i = 0;
  do
  {
    pLastFSI = pThisFSI;
    ulSize = FTIMES_STREAM_INFO_SIZE << i;
    pThisFSI = (FILE_STREAM_INFORMATION *)calloc(ulSize, 1);
    if (pThisFSI == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: calloc(): %s", cRoutine, strerror(errno));
      if (i > 0) /* Restore the last good set of information, and break. */
      {
        pThisFSI = pLastFSI;
        iStatus = FTIMES_PARTIAL_STREAM_COUNT;
        break;
      }
      else
      {
        return FTIMES_EMPTY_STREAM_COUNT;
      }
    }
    else
    {
      dwStatus = NtdllNQIF(hFile, &ioStatusBlock, pThisFSI, ulSize, FileStreamInformation);
      if (dwStatus == STATUS_SUCCESS || dwStatus == STATUS_BUFFER_OVERFLOW)
      {
        if (i > 0)
        {
          free(pLastFSI);
        }
      }
      else
      {
        free(pThisFSI);
        SetLastError(LsaNtStatusToWinError(dwStatus));
        ErrorFormatWin32Error(&pcMessage);
        snprintf(pcError, ERRBUF_SIZE, "%s: NtQueryInformationFile(): %s", cRoutine, pcMessage);
        if (i > 0) /* Restore the last good set of information, and break. */
        {
          pThisFSI = pLastFSI;
          iStatus = FTIMES_PARTIAL_STREAM_COUNT;
          break;
        }
        else
        {
          return FTIMES_EMPTY_STREAM_COUNT;
        }
      }
    }
    i++;
  } while (dwStatus == STATUS_BUFFER_OVERFLOW);

  *ppucStreamInfo = (unsigned char *) pThisFSI;

  /*-
   *********************************************************************
   *
   * Count all but the default stream. This logic works even when
   * NtQueryInformationFile() returns no data. This is due to the fact
   * that pThisFSI is large enough to contain at least one struct, and
   * it was initialized to zeros by to by calloc(). In that particular
   * case, NextEntryOffset will be zero, and the loop will terminate.
   *
   *********************************************************************
   */
  for (iStreamCount = iNextEntryOffset = 0; pThisFSI->NextEntryOffset; iNextEntryOffset = pThisFSI->NextEntryOffset)
  {
    pThisFSI = (FILE_STREAM_INFORMATION *) ((byte *) pThisFSI + iNextEntryOffset);
    if (pThisFSI->StreamNameLength && wcscmp(pThisFSI->StreamName, DEFAULT_STREAM_NAME_W) != 0)
    {
      iStreamCount++;
    }
  }
  *piStreamCount = iStreamCount;

  return iStatus;
}
#endif /* WINNT */
#endif /* WIN32 */


#ifdef FTimes_UNIX
int
MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError)
{
  const char          cRoutine[] = "MapFile()";
  char                cLocalError[ERRBUF_SIZE];
  char                cLinkData[FTIMES_MAX_PATH];
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  int                 iError;
  int                 iFSType;
  int                 iPathLength;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Initialize the sFTFileData structure. Subsequent logic relies
   * on the assertion that ucFileMD5 has been initialized to all zeros.
   *
   *********************************************************************
   */
  memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
  sFTFileData.pcRawPath = pcPath;

  /*-
   *********************************************************************
   *
   * Neuter the given path. In other words, replace funky chars with
   * their hex value (e.g., backspace becomes %08).
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  pcNeuteredPath = SupportNeuterString(pcPath, iPathLength, cLocalError);
  if (pcNeuteredPath == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, pcPath, cLocalError);
    ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
    return ER_NeuterPathname;
  }
  sFTFileData.pcNeuteredPath = pcNeuteredPath;

  /*-
   *********************************************************************
   *
   * Get the file system type.
   *
   *********************************************************************
   */
  iFSType = GetFileSystemType(pcPath, cLocalError);
  if (iFSType == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  if (iFSType == FSTYPE_UNSUPPORTED)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding unsupported file system.", cRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  if (iFSType == FSTYPE_NFS && !psProperties->bMapRemoteFiles)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding remote file system.", cRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  sFTFileData.iFSType = iFSType;

  /*-
   *********************************************************************
   *
   * Collect attributes, but don't follow symbolic links.
   *
   *********************************************************************
   */
  iError = MapGetAttributes(&sFTFileData, cLocalError);
  if (iError == Have_Nothing)
  {
    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path.
     *
     *******************************************************************
     */
    SupportFreeData(pcNeuteredPath);

    return ER;
  }


  /*-
   *********************************************************************
   *
   * Map directories, files, and links. Everything else is considered
   * a special file. In that case write out any attributes have been
   * collected.
   *
   *********************************************************************
   */
  if (S_ISDIR(sFTFileData.statEntry.st_mode))
  {
    giDirectories++;
#ifdef USE_XMAGIC
    if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
    {
      snprintf(sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, "directory");
    }
#endif
    MapTree(psProperties, pcPath, iFSType, sFTFileData.ucFileMD5, cLocalError);
  }
#ifdef ENABLE_BLK_CHR_DEVICE_INCLUDES
  else if (S_ISREG(sFTFileData.statEntry.st_mode) || S_ISBLK(sFTFileData.statEntry.st_mode) || S_ISCHR(sFTFileData.statEntry.st_mode))
#else
  else if (S_ISREG(sFTFileData.statEntry.st_mode))
#endif
  {
    giFiles++;
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, &sFTFileData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
  }
  else if (S_ISLNK(sFTFileData.statEntry.st_mode))
  {
    giSpecial++;
    if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
    {
      iError = readlink(pcPath, cLinkData, FTIMES_MAX_PATH - 1);
      if (iError == ER)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Unreadable Symbolic Link: %s", cRoutine, pcNeuteredPath, strerror(errno));
        ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
      }
      else
      {
        cLinkData[iError] = 0; /* Readlink does not append a NULL. */
        md5_string((unsigned char *) cLinkData, strlen(cLinkData), sFTFileData.ucFileMD5);
      }
    }
#ifdef USE_XMAGIC
    if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
    {
      iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.statEntry, sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
#endif
  }
  else
  {
    giSpecial++;
#ifdef USE_XMAGIC
    if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
    {
      iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.statEntry, sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
#endif
  }

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

  /*-
   *********************************************************************
   *
   * Free the neutered path.
   *
   *********************************************************************
   */
  SupportFreeData(pcNeuteredPath);

  return ER_OK;
}
#endif


#ifdef FTimes_WIN32
/*-
 ***********************************************************************
 *
 * MapFile
 *
 ***********************************************************************
 */
int
MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError)
{
  const char          cRoutine[] = "MapFile()";
  char                cLocalError[ERRBUF_SIZE];
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  int                 iError;
  int                 iFSType;
  int                 iPathLength;

#ifdef FTimes_WINNT
  struct hash_block   unusedHashBlock;
#endif

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Initialize the file data structure.
   *
   *********************************************************************
   */
  memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
  sFTFileData.pcRawPath = pcPath;
  sFTFileData.dwVolumeSerialNumber = -1;
  sFTFileData.nFileIndexHigh = -1;
  sFTFileData.nFileIndexLow = -1;
  sFTFileData.iStreamCount = -1; /* Develop routines check for this value. */

  /*-
   *********************************************************************
   *
   * Neuter the given path. In other words, replace funky chars with
   * their hex value (e.g., backspace becomes %08).
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  pcNeuteredPath = SupportNeuterString(pcPath, iPathLength, cLocalError);
  if (pcNeuteredPath == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathR = [%s]: %s", cRoutine, pcPath, cLocalError);
    ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
    return ER_NeuterPathname;
  }
  sFTFileData.pcNeuteredPath = pcNeuteredPath;

  /*-
   *********************************************************************
   *
   * Get the file system type.
   *
   *********************************************************************
   */
  iFSType = GetFileSystemType(pcPath, cLocalError);
  if (iFSType == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  if (iFSType == FSTYPE_UNSUPPORTED)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding unsupported file system.", cRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  if ((iFSType == FSTYPE_NTFS_REMOTE || iFSType == FSTYPE_FAT_REMOTE) && !psProperties->bMapRemoteFiles)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Excluding remote file system.", cRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    SupportFreeData(pcNeuteredPath);
    return ER;
  }
  sFTFileData.iFSType = iFSType;

  /*-
   *********************************************************************
   *
   * Collect attributes.
   *
   *********************************************************************
   */
  iError = MapGetAttributes(&sFTFileData, cLocalError);
  if (iError == Have_Nothing)
  {
    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path.
     *
     *******************************************************************
     */
    SupportFreeData(pcNeuteredPath);

    return ER;
  }

  /*-
   *********************************************************************
   *
   * Map directories and files.
   *
   *********************************************************************
   */
  if ((sFTFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
  {
    giDirectories++;
#ifdef USE_XMAGIC
    if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
    {
      snprintf(sFTFileData.cType, FTIMES_FILETYPE_BUFSIZE, "directory");
    }
#endif
    MapTree(psProperties, pcPath, iFSType, sFTFileData.ucFileMD5, cLocalError);
  }
  else
  {
    giFiles++;
    if (sFTFileData.iFileFlags >= Have_MapGetFileHandle)
    {
      if (psProperties->iLastAnalysisStage > 0)
      {
        iError = AnalyzeFile(psProperties, &sFTFileData, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
          ErrorHandler(iError, pcError, ERROR_FAILURE);
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = MapWriteRecord(psProperties, &sFTFileData, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: %s", cRoutine, pcNeuteredPath, cLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

#ifdef FTimes_WINNT
  /*-
   *********************************************************************
   *
   * Process any alternate streams. This applies only to NTFS.
   *
   *********************************************************************
   */
  if (sFTFileData.iStreamCount > 0)
  {
    if (psProperties->bHashDirectories && (psProperties->ulFieldMask & MD5_SET) == MD5_SET)
    {
      md5_begin(&unusedHashBlock);
    }
    MapStream(psProperties, &sFTFileData, &unusedHashBlock, cLocalError);
  }
  if (sFTFileData.pucStreamInfo)
  {
    free(sFTFileData.pucStreamInfo);
  }
#endif

  /*-
   *********************************************************************
   *
   * Free the neutered path.
   *
   *********************************************************************
   */
  SupportFreeData(pcNeuteredPath);

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * MapWriteRecord
 *
 ***********************************************************************
 */
int
MapWriteRecord(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          cRoutine[] = "MapWriteRecord()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  int                 iWriteCount;

#ifdef FTimes_UNIX
  /*-
   *********************************************************************
   *
   * name          (4 * FTIMES_MAX_PATH) + 2 (+2 for quotes)
   * dev           10 (32 bits in decimal)
   * inode         10 (32 bits in decimal)
   * mode          10 (32 bits in decimal)
   * nlink         10 (32 bits in decimal)
   * uid           10 (32 bits in decimal)
   * gid           10 (32 bits in decimal)
   * rdev          10 (32 bits in decimal)
   * atime         FTIMES_TIME_FORMAT_SIZE
   * mtime         FTIMES_TIME_FORMAT_SIZE
   * ctime         FTIMES_TIME_FORMAT_SIZE
   * size          20 (64 bits in decimal)
   * md5           MD5_HASH_STRING_LENGTH
   * |'s           13 (not counting those embedded in time)
   * ------------------------------------------------------------------
   * Total         ((4 * FTIMES_MAX_PATH) + 2) + MD5_HASH_STRING_LENGTH + (3 * FTIMES_TIME_FORMAT_SIZE) + 103
   *
   *********************************************************************
   */
  static char         cOutput[((4 * FTIMES_MAX_PATH) + 2) + MD5_HASH_STRING_LENGTH + (3 * FTIMES_TIME_FORMAT_SIZE) + 103];
#endif

#ifdef FTimes_WIN32
  /*-
   *********************************************************************
   *
   * name          (4 * FTIMES_MAX_PATH) + 2 (+2 for quotes)
   * volume        10 (32 bits in decimal)
   * findex        20 (64 bits in decimal)
   * attributes    10 (32 bits in decimal)
   * atime|ams     FTIMES_TIME_FORMAT_SIZE
   * mtime|mms     FTIMES_TIME_FORMAT_SIZE
   * ctime|cms     FTIMES_TIME_FORMAT_SIZE
   * chtime|chms   FTIMES_TIME_FORMAT_SIZE
   * size          20 (64 bits in decimal)
   * altstreams    10 (32 bits in decimal)
   * md5           MD5_HASH_STRING_LENGTH
   * |'s           11 (not counting those embedded in time)
   * -----------------------------------------------------------------
   * Total         ((4 * FTIMES_MAX_PATH) + 2) + MD5_HASH_STRING_LENGTH + (4 * FTIMES_TIME_FORMAT_SIZE) + 81
   *
   *********************************************************************
   */
  static char         cOutput[((4 * FTIMES_MAX_PATH) + 2) + MD5_HASH_STRING_LENGTH + (4 * FTIMES_TIME_FORMAT_SIZE) + 81];
#endif

  /*-
   *********************************************************************
   *
   * Initialize the write count. Format or compress the output.
   *
   *********************************************************************
   */
  iWriteCount = 0;
  iError = psProperties->piDevelopMapOutput(psProperties, cOutput, &iWriteCount, psFTData, cLocalError);

  /*-
   *********************************************************************
   *
   * Update global counts. The values for incomplete, complete,
   * and total records specifically refer to the number of records
   * in the output stream. Inform the user if a record has one or
   * more null fields.
   *
   *********************************************************************
   */
  if (iError == ER_NullFields)
  {
    giIncompleteRecords++;
    snprintf(pcError, ERRBUF_SIZE, "%s: Null Fields: %s", cRoutine, cLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
  giRecords++;

  /*-
   *********************************************************************
   *
   * Write the output data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, cOutput, iWriteCount, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output file hash.
   *
   *********************************************************************
   */
  md5_middle(&psProperties->sOutFileHashContext, cOutput, iWriteCount);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapWriteHeader
 *
 ***********************************************************************
 */
int
MapWriteHeader(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "MapWriteHeader()";
  char                cLocalError[ERRBUF_SIZE];
  char                cHeaderData[FTIMES_MAX_LINE];
  int                 i;
  int                 iError;
  int                 iIndex;
  unsigned long       ulTempMask;
  unsigned long       ulFieldMask;

  /*-
   *********************************************************************
   *
   * Initialize the output's MD5 hash.
   *
   *********************************************************************
   */
  md5_begin(&psProperties->sOutFileHashContext);

  /*-
   *********************************************************************
   *
   * Build the output's header.
   *
   *********************************************************************
   */
  ulFieldMask = psProperties->ulFieldMask & ALL_MASK;
  iIndex = sprintf(cHeaderData, (psProperties->bCompress) ? "zname" : "name");
  for (i = 0; i < psProperties->iMaskTableLength; i++)
  {
    ulTempMask = (1 << i);
    if ((ulFieldMask & ulTempMask) == ulTempMask)
    {
      iIndex += sprintf(&cHeaderData[iIndex], "|%s", (char *) psProperties->ptMaskTable[i].HeaderName);
    }
  }
  iIndex += sprintf(&cHeaderData[iIndex], "%s", psProperties->cNewLine);

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, cHeaderData, iIndex, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output's MD5 hash.
   *
   *********************************************************************
   */
  md5_middle(&psProperties->sOutFileHashContext, cHeaderData, iIndex);

  return ER_OK;
}


#ifdef FTimes_UNIX
/*-
 ***********************************************************************
 *
 * MapGetAttributes
 *
 ***********************************************************************
 */
int
MapGetAttributes(FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          cRoutine[] = "MapGetAttributes()";

  psFTData->iFileFlags = Have_Nothing;

  /*-
   *********************************************************************
   *
   * Collect attributes. Use lstat() so links aren't followed.
   *
   *********************************************************************
   */
  if (lstat(psFTData->pcRawPath, &psFTData->statEntry) != ER)
  {
    psFTData->iFileFlags = Have_lstat;
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: lstat(): %s", cRoutine, psFTData->pcNeuteredPath, strerror(errno));
    ErrorHandler(ER_lstat, pcError, ERROR_FAILURE);
    pcError[0] = 0;
  }
  return psFTData->iFileFlags;
}
#endif


#ifdef FTimes_WIN32
/*-
 ***********************************************************************
 *
 * MapGetAttributes
 *
 ***********************************************************************
 */
int
MapGetAttributes(FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          cRoutine[] = "MapGetAttributes()";
  BY_HANDLE_FILE_INFORMATION fileInfo;
  char                cLocalError[ERRBUF_SIZE];
  char               *pcMessage;
  HANDLE              hFile;
  WIN32_FILE_ATTRIBUTE_DATA fileAttributeData;

#ifdef FTimes_WIN98
  DWORD               dwFileAttributes;
#endif

#ifdef FTimes_WINNT
  DWORD               dwStatus;
  FILE_BASIC_INFORMATION fileBasicInfo;
  IO_STATUS_BLOCK     ioStatusBlock;
  int                 iStatus;
#endif

  cLocalError[0] = 0;

  psFTData->iFileFlags = Have_Nothing;

#ifdef FTimes_WIN98
  /*-
   *********************************************************************
   *
   * Windows 98 can't open directories the way we need them to, so we
   * must rely on GetFileAttributesEx().
   *
   *********************************************************************
   */
  dwFileAttributes = GetFileAttributes(psFTData->pcRawPath);
  if (dwFileAttributes != 0xffffffff)
  {
    psFTData->iFileFlags = Have_GetFileAttributes;
    if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (GetFileAttributesEx(psFTData->pcRawPath, GetFileExInfoStandard, &fileAttributeData))
      {
        psFTData->iFileFlags                      = Have_GetFileAttributesEx;
        psFTData->dwFileAttributes                = fileAttributeData.dwFileAttributes;
        psFTData->ftLastAccessTime.dwLowDateTime  = fileAttributeData.ftLastAccessTime.dwLowDateTime;
        psFTData->ftLastAccessTime.dwHighDateTime = fileAttributeData.ftLastAccessTime.dwHighDateTime;
        psFTData->ftLastWriteTime.dwLowDateTime   = fileAttributeData.ftLastWriteTime.dwLowDateTime;
        psFTData->ftLastWriteTime.dwHighDateTime  = fileAttributeData.ftLastWriteTime.dwHighDateTime;
        psFTData->ftCreationTime.dwLowDateTime    = fileAttributeData.ftCreationTime.dwLowDateTime;
        psFTData->ftCreationTime.dwHighDateTime   = fileAttributeData.ftCreationTime.dwHighDateTime;
        psFTData->nFileSizeHigh                   = fileAttributeData.nFileSizeHigh;
        psFTData->nFileSizeLow                    = fileAttributeData.nFileSizeLow;
      }
      else
      {
        ErrorFormatWin32Error(&pcMessage);
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: GetFileAttributesEx(): %s", cRoutine, psFTData->pcNeuteredPath, pcMessage);
        ErrorHandler(ER_GetFileAttrsEx, pcError, ERROR_FAILURE);
        pcError[0] = 0;
      }
      return psFTData->iFileFlags;
    }
  }
  else
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: GetFileAttributes(): %s", cRoutine, psFTData->pcNeuteredPath, pcMessage);
    ErrorHandler(ER_GetFileAttrs, pcError, ERROR_FAILURE);
    pcError[0] = 0;
    return psFTData->iFileFlags;
  }
#endif

  /*-
   *********************************************************************
   *
   * Collect attributes. Use GetFileInformationByHandle() if the file
   * handle is valid. Otherwise, fall back to GetFileAttributesEx().
   * If this is NT and the file handle is valid, additionally invoke
   * the native call NTQueryInformationFile() to get ChangeTime.
   *
   *********************************************************************
   */
  hFile = MapGetFileHandle(psFTData->pcRawPath);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    psFTData->iFileFlags = Have_MapGetFileHandle;
    if (GetFileInformationByHandle(hFile, &fileInfo))
    {
      psFTData->iFileFlags                      = Have_GetFileInformationByHandle;
      psFTData->dwVolumeSerialNumber            = fileInfo.dwVolumeSerialNumber;
      psFTData->nFileIndexHigh                  = fileInfo.nFileIndexHigh;
      psFTData->nFileIndexLow                   = fileInfo.nFileIndexLow;
      psFTData->dwFileAttributes                = fileInfo.dwFileAttributes;
      psFTData->ftLastAccessTime.dwLowDateTime  = fileInfo.ftLastAccessTime.dwLowDateTime;
      psFTData->ftLastAccessTime.dwHighDateTime = fileInfo.ftLastAccessTime.dwHighDateTime;
      psFTData->ftLastWriteTime.dwLowDateTime   = fileInfo.ftLastWriteTime.dwLowDateTime;
      psFTData->ftLastWriteTime.dwHighDateTime  = fileInfo.ftLastWriteTime.dwHighDateTime;
      psFTData->ftCreationTime.dwLowDateTime    = fileInfo.ftCreationTime.dwLowDateTime;
      psFTData->ftCreationTime.dwHighDateTime   = fileInfo.ftCreationTime.dwHighDateTime;
      psFTData->ftChangeTime.dwLowDateTime      = 0;
      psFTData->ftChangeTime.dwHighDateTime     = 0;
      psFTData->nFileSizeHigh                   = fileInfo.nFileSizeHigh;
      psFTData->nFileSizeLow                    = fileInfo.nFileSizeLow;
    }
    else
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: GetFileInformationByHandle(): %s", cRoutine, psFTData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_GetFileInfo, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }

#ifdef FTimes_WINNT
    memset(&fileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
    dwStatus = NtdllNQIF(hFile, &ioStatusBlock, &fileBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    if (dwStatus == 0)
    {
      psFTData->iFileFlags                      = Have_NTQueryInformationFile;
      psFTData->dwFileAttributes                = fileBasicInfo.FileAttributes;
      psFTData->ftLastAccessTime.dwLowDateTime  = fileBasicInfo.LastAccessTime.LowPart;
      psFTData->ftLastAccessTime.dwHighDateTime = fileBasicInfo.LastAccessTime.HighPart;
      psFTData->ftLastWriteTime.dwLowDateTime   = fileBasicInfo.LastWriteTime.LowPart;
      psFTData->ftLastWriteTime.dwHighDateTime  = fileBasicInfo.LastWriteTime.HighPart;
      psFTData->ftCreationTime.dwLowDateTime    = fileBasicInfo.CreationTime.LowPart;
      psFTData->ftCreationTime.dwHighDateTime   = fileBasicInfo.CreationTime.HighPart;
      psFTData->ftChangeTime.dwLowDateTime      = fileBasicInfo.ChangeTime.LowPart;
      psFTData->ftChangeTime.dwHighDateTime     = fileBasicInfo.ChangeTime.HighPart;
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: NtdllNQIF(): %08x", cRoutine, psFTData->pcNeuteredPath, dwStatus);
      ErrorHandler(ER_NQIF, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }

    /*-
     *********************************************************************
     *
     * Determine the number of alternate streams. This check applies to
     * files and directories, and is specific to NTFS. A valid handle
     * is required to perform the check.
     *
     *********************************************************************
     */
    if (psFTData->iFSType == FSTYPE_NTFS)
    {
      iStatus = MapCountNamedStreams(hFile, &psFTData->iStreamCount, &psFTData->pucStreamInfo, cLocalError);
      switch (iStatus)
      {
      case FTIMES_FULL_STREAM_COUNT:
        psFTData->iFileFlags = Have_MapCountNamedStreams;
        break;
      case FTIMES_PARTIAL_STREAM_COUNT:
        psFTData->iFileFlags = Have_MapCountNamedStreams;
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Stream Count Incomplete: %s", cRoutine, psFTData->pcNeuteredPath, cLocalError);
        ErrorHandler(ER_MapCountNamedStreams, pcError, ERROR_FAILURE);
        pcError[0] = 0;
        break;
      default:
        snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: Stream Count Failed: %s", cRoutine, psFTData->pcNeuteredPath, cLocalError);
        ErrorHandler(ER_MapCountNamedStreams, pcError, ERROR_FAILURE);
        pcError[0] = 0;
        break;
      }
    }
#endif
    CloseHandle(hFile);
  }
  else
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: MapGetFileHandle(): %s", cRoutine, psFTData->pcNeuteredPath, pcMessage);
    ErrorHandler(ER_CreateFile, pcError, ERROR_FAILURE);
    pcError[0] = 0;

    if (GetFileAttributesEx(psFTData->pcRawPath, GetFileExInfoStandard, &fileAttributeData))
    {
      psFTData->iFileFlags                      = Have_GetFileAttributesEx;
      psFTData->dwFileAttributes                = fileAttributeData.dwFileAttributes;
      psFTData->ftLastAccessTime.dwLowDateTime  = fileAttributeData.ftLastAccessTime.dwLowDateTime;
      psFTData->ftLastAccessTime.dwHighDateTime = fileAttributeData.ftLastAccessTime.dwHighDateTime;
      psFTData->ftLastWriteTime.dwLowDateTime   = fileAttributeData.ftLastWriteTime.dwLowDateTime;
      psFTData->ftLastWriteTime.dwHighDateTime  = fileAttributeData.ftLastWriteTime.dwHighDateTime;
      psFTData->ftCreationTime.dwLowDateTime    = fileAttributeData.ftCreationTime.dwLowDateTime;
      psFTData->ftCreationTime.dwHighDateTime   = fileAttributeData.ftCreationTime.dwHighDateTime;
      psFTData->nFileSizeHigh                   = fileAttributeData.nFileSizeHigh;
      psFTData->nFileSizeLow                    = fileAttributeData.nFileSizeLow;
    }
    else
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, ERRBUF_SIZE, "%s: PathN = [%s]: GetFileAttributesEx(): %s", cRoutine, psFTData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_GetFileAttrsEx, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }
  }
  return psFTData->iFileFlags;
}
#endif
