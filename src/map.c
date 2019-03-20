/*-
 ***********************************************************************
 *
 * $Id: map.c,v 1.57 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static int giDirectories;
static int giFiles;
static int giSpecial;
#ifdef WINNT
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


#ifdef WINNT
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


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * MapTree
 *
 ***********************************************************************
 */
int
MapTree(FTIMES_PROPERTIES *psProperties, char *pcPath, int iFSType, FTIMES_FILE_DATA *psParentFTData, char *pcError)
{
  const char          acRoutine[] = "MapTree()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acLinkData[FTIMES_MAX_PATH];
  char                acMessage[MESSAGE_SIZE];
  char                acNewRawPath[FTIMES_MAX_PATH];
  char                acParentPath[FTIMES_MAX_PATH];
  char               *pc;
  char               *pcNeuteredPath;
  DIR                *psDir;
  FTIMES_FILE_DATA    sFTFileData;
  FTIMES_HASH_DATA    sDirFTHashData;
  int                 iError;
#ifdef USE_PCRE
  int                 iFiltered = 0;
#endif
  int                 iNewFSType;
  int                 iNameLength;
  int                 iPathLength;
  int                 iParentPathLength;
  struct dirent      *psDirEntry;
  struct stat         sStatPDirectory, *psStatPDirectory;
  struct stat         sStatCDirectory, *psStatCDirectory;

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
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 1);
    ErrorHandler(ER_Length, pcError, ERROR_CRITICAL);
  }

  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(acMessage, MESSAGE_SIZE, "FS=%s Directory=%s", gaacFSType[iFSType], pcPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, acMessage);
  }

  /*-
   *********************************************************************
   *
   * If directory hashing is enabled, initialize hash contexts.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      MD5Alpha(&sDirFTHashData.sMd5Context);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      SHA1Alpha(&sDirFTHashData.sSha1Context);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      SHA256Alpha(&sDirFTHashData.sSha256Context);
    }
  }

  /*-
   *********************************************************************
   *
   * Chop off trailing slashes for non root directories.
   *
   *********************************************************************
   */
  iParentPathLength = iPathLength;
  pc = strncpy(acParentPath, pcPath, FTIMES_MAX_PATH);
  if (strcmp(acParentPath, FTIMES_SLASH) != 0)
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
  if (strcmp(acParentPath, FTIMES_SLASH) != 0)
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
  if (lstat(acParentPath, &sStatPDirectory) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawParentPath = [%s]: %s", acRoutine, acParentPath, strerror(errno));
    ErrorHandler(ER_lstat, pcError, ERROR_FAILURE);
    psStatPDirectory = NULL;
  }
  else
  {
    psStatPDirectory = &sStatPDirectory;
  }

  if (lstat(pcPath, &sStatCDirectory) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawCurrentPath = [%s]: %s", acRoutine, pcPath, strerror(errno));
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
  if ((psDir = opendir(pcPath)) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, pcPath, strerror(errno));
    ErrorHandler(ER_opendir, pcError, ERROR_FAILURE);
    return ER_opendir;
  }

  /*-
   *********************************************************************
   *
   * Note: errno is cleared before each readdir() call so that its
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
   * relies on the assertion that each hash value has been initialized
   * to all zeros.
   *
   *********************************************************************
   */
  while ((psDirEntry = readdir(psDir)) != NULL)
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
    iNameLength = iPathLength + 1 + strlen(psDirEntry->d_name);
    if (iNameLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NewRawPath = [%s/%s], Length = [%d]: Length would exceed %d bytes.",
        acRoutine,
        pcPath,
        psDirEntry->d_name,
        iNameLength,
        FTIMES_MAX_PATH - 1
        );
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
    strncpy(acNewRawPath, pcPath, FTIMES_MAX_PATH);
    if (pcPath[iPathLength - 1] != FTIMES_SLASHCHAR)
    {
      acNewRawPath[iPathLength] = FTIMES_SLASHCHAR;
      strcat(&acNewRawPath[iPathLength + 1], psDirEntry->d_name);
    }
    else
    {
      iNameLength--;
      strcat(&acNewRawPath[iPathLength], psDirEntry->d_name);
    }
    sFTFileData.pcRawPath = acNewRawPath;

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, then continue with the
     * next entry. Remember to clear errno.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->psExcludeList, acNewRawPath) != NULL)
    {
      errno = 0;
      continue;
    }

#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If the new path is matched by an exclude filter, continue with
     * the next entry. If the new path is matched by an include
     * filter, set a flag, but keep going. Include filters do not get
     * applied until the file's type is known. This is because
     * directories must be traversed before they can be filtered.
     *
     *******************************************************************
     */
    if (psProperties->psExcludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, acNewRawPath);
      if (psFilter != NULL)
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, acNewRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
        errno = 0;
        continue;
      }
    }

    iFiltered = 0;
    if (psProperties->psIncludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, acNewRawPath);
      if (psFilter == NULL)
      {
        iFiltered = 1;
      }
      else
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, acNewRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
      }
    }
#endif

    /*-
     *******************************************************************
     *
     * Neuter the given path. In other words replace funky chars with
     * their hex value (e.g., backspace becomes %08).
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(acNewRawPath, iNameLength, acLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, acNewRawPath, acLocalError);
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
    iError = MapGetAttributes(&sFTFileData, acLocalError);
    if (iError == Have_Nothing)
    {
#ifdef USE_PCRE
      /*-
       *****************************************************************
       *
       * If this path has been filtered, we're done.
       *
       *****************************************************************
       */
      if (iFiltered)
      {
        MEMORY_FREE(pcNeuteredPath);
        errno = 0;
        continue;
      }
#endif

      /*-
       *****************************************************************
       *
       * If this is "." or "..", we're done.
       *
       *****************************************************************
       */
      if (strcmp(psDirEntry->d_name, FTIMES_DOT) == 0 || strcmp(psDirEntry->d_name, FTIMES_DOTDOT) == 0)
      {
        MEMORY_FREE(pcNeuteredPath);
        errno = 0;
        continue;
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, zeros will be folded into the aggregate hash.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5Cycle(&sDirFTHashData.sMd5Context, sFTFileData.aucFileMd5, MD5_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1Cycle(&sDirFTHashData.sSha1Context, sFTFileData.aucFileSha1, SHA1_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256Cycle(&sDirFTHashData.sSha256Context, sFTFileData.aucFileSha256, SHA256_HASH_SIZE);
        }
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      /*-
       *****************************************************************
       *
       * Free the neutered path, set errno, and continue.
       *
       *****************************************************************
       */
      MEMORY_FREE(pcNeuteredPath);
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
    if (strcmp(psDirEntry->d_name, FTIMES_DOT) == 0)
    {
      if (psStatCDirectory != NULL)
      {
        if (sFTFileData.sStatEntry.st_ino != sStatCDirectory.st_ino)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Inode mismatch between '.' and current directory.", acRoutine, pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to compare it to '.'.", acRoutine, pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else if (strcmp(psDirEntry->d_name, FTIMES_DOTDOT) == 0)
    {
      if (psStatPDirectory != NULL)
      {
        if (sFTFileData.sStatEntry.st_ino != sStatPDirectory.st_ino)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Inode mismatch between '..' and parent directory.", acRoutine, pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on parent directory to compare it to '..'.", acRoutine, pcNeuteredPath);
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
      if (S_ISDIR(sFTFileData.sStatEntry.st_mode))
      {
        giDirectories++;
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          snprintf(sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, "directory");
        }
#endif
        /*-
         ***************************************************************
         *
         * Check for a device crossing. Process new file systems if
         * they are supported, and process remote file systems when
         * AnalyzeRemoteFiles is enabled.
         *
         ***************************************************************
         */
        if (psStatCDirectory != NULL)
        {
          if (sFTFileData.sStatEntry.st_dev != sStatCDirectory.st_dev)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Crossing a device boundary.", acRoutine, pcNeuteredPath);
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
            iNewFSType = GetFileSystemType(acNewRawPath, acLocalError);
            if (iNewFSType == ER)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              MEMORY_FREE(pcNeuteredPath);
              errno = 0;
              continue;
            }
            if (iNewFSType == FSTYPE_UNSUPPORTED)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              MEMORY_FREE(pcNeuteredPath);
              errno = 0;
              continue;
            }
            if ((iNewFSType == FSTYPE_NFS || iNewFSType == FSTYPE_SMB) && !psProperties->bAnalyzeRemoteFiles)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, pcNeuteredPath);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              MEMORY_FREE(pcNeuteredPath);
              errno = 0;
              continue;
            }
            iFSType = iNewFSType;
          }
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to determine a device boundary crossing.", acRoutine, pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
          iFSType = FSTYPE_NA;
        }
        if (psProperties->bEnableRecursion)
        {
          MapTree(psProperties, acNewRawPath, iFSType, &sFTFileData, acLocalError);
        }
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
          MEMORY_FREE(pcNeuteredPath);
          errno = 0;
          continue;
        }
#endif
      }
      else if (S_ISREG(sFTFileData.sStatEntry.st_mode))
      {
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
          MEMORY_FREE(pcNeuteredPath);
          errno = 0;
          continue;
        }
#endif
        giFiles++;
        if (psProperties->iLastAnalysisStage > 0)
        {
          iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
      }
      else if (S_ISLNK(sFTFileData.sStatEntry.st_mode))
      {
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
          MEMORY_FREE(pcNeuteredPath);
          errno = 0;
          continue;
        }
#endif
        giSpecial++;
        if (psProperties->bHashSymbolicLinks)
        {
          iError = readlink(acNewRawPath, acLinkData, FTIMES_MAX_PATH - 1);
          if (iError == ER)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Unreadable Symbolic Link: %s", acRoutine, pcNeuteredPath, strerror(errno));
            ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
          }
          else
          {
            acLinkData[iError] = 0; /* Readlink does not append a NULL. */
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
            {
              MD5HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileMd5);
            }
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
            {
              SHA1HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileSha1);
            }
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
            {
              SHA256HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileSha256);
            }
          }
        }
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.sStatEntry, sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }
      else
      {
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
          MEMORY_FREE(pcNeuteredPath);
          errno = 0;
          continue;
        }
#endif
        giSpecial++;
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.sStatEntry, sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, zeros will be folded into the aggregate hash.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5Cycle(&sDirFTHashData.sMd5Context, sFTFileData.aucFileMd5, MD5_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1Cycle(&sDirFTHashData.sSha1Context, sFTFileData.aucFileSha1, SHA1_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256Cycle(&sDirFTHashData.sSha256Context, sFTFileData.aucFileSha256, SHA256_HASH_SIZE);
        }
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    MEMORY_FREE(pcNeuteredPath);

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
  if (psDirEntry == NULL && errno != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, pcPath, strerror(errno));
    ErrorHandler(ER_readdir, pcError, ERROR_FAILURE);
  }

  /*-
   *********************************************************************
   *
   * Complete the directory hash(es). NOTE: The caller is expected to
   * initialize each hash to all zeros.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      MD5Omega(&sDirFTHashData.sMd5Context, psParentFTData->aucFileMd5);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      SHA1Omega(&sDirFTHashData.sSha1Context, psParentFTData->aucFileSha1);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      SHA256Omega(&sDirFTHashData.sSha256Context, psParentFTData->aucFileSha256);
    }
  }

  closedir(psDir);
  return ER_OK;
}
#endif


#ifdef WIN32
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
MapTree(FTIMES_PROPERTIES *psProperties, char *pcPath, int iFSType, FTIMES_FILE_DATA *psParentFTData, char *pcError)
{
  const char          acRoutine[] = "MapTree()";
  BOOL                bResult;
  BY_HANDLE_FILE_INFORMATION sFileInfoCurrent;
  BY_HANDLE_FILE_INFORMATION sFileInfoParent;
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acMessage[MESSAGE_SIZE];
  char                acNewRawPath[FTIMES_MAX_PATH];
  char                acParentPath[FTIMES_MAX_PATH];
  char                acSearchPath[FTIMES_MAX_PATH];
  char               *pc;
  char               *pcNeuteredPath;
  char               *pcMessage;
  FTIMES_FILE_DATA    sFTFileData;
  FTIMES_HASH_DATA    sDirFTHashData;
  HANDLE              hFileCurrent;
  HANDLE              hFileParent;
  HANDLE              hSearch;
  int                 iError;
#ifdef USE_PCRE
  int                 iFiltered = 0;
#endif
  int                 iNameLength;
  int                 iParentPathLength;
  int                 iPathLength;
  WIN32_FIND_DATA     sFindData;

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
    snprintf(pcError, MESSAGE_SIZE, "%s: Directory = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 1);
    ErrorHandler(ER_Length, pcError, ERROR_CRITICAL);
  }

  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(acMessage, MESSAGE_SIZE, "FS=%s Directory=%s", gaacFSType[iFSType], pcPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, acMessage);
  }

  /*-
   *********************************************************************
   *
   * If directory hashing is enabled, initialize hash contexts.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      MD5Alpha(&sDirFTHashData.sMd5Context);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      SHA1Alpha(&sDirFTHashData.sSha1Context);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      SHA256Alpha(&sDirFTHashData.sSha256Context);
    }
  }

  /*-
   *********************************************************************
   *
   * Chop off trailing slashes for non root directories.
   *
   *********************************************************************
   */
  iParentPathLength = iPathLength;
  pc = strncpy(acParentPath, pcPath, FTIMES_MAX_PATH);
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
    snprintf(pcError, MESSAGE_SIZE, "%s: SearchPath = [%s\\*], Length = [%d]: Length would exceed %d bytes.", acRoutine, pcPath, iPathLength, FTIMES_MAX_PATH - 3);
    ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
    return ER_Length;
  }

  strncpy(acSearchPath, pcPath, FTIMES_MAX_PATH);
  acSearchPath[iPathLength] = FTIMES_SLASHCHAR;
  acSearchPath[iPathLength + 1] = '*';
  acSearchPath[iPathLength + 2] = 0;

  /*-
   *********************************************************************
   *
   * Begin the search.
   *
   *********************************************************************
   */
  hSearch = FindFirstFile(acSearchPath, &sFindData);

  if (hSearch == INVALID_HANDLE_VALUE)
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, pcPath, pcMessage);
    ErrorHandler(ER_FindFirstFile, pcError, ERROR_FAILURE);
    return ER_FindFirstFile;
  }

  /*-
   *********************************************************************
   *
   * Loop through the list of directory entries. Each time through this
   * loop, clear the contents of sFTFileData -- subsequent logic
   * relies on the assertion that each hash value has been initialized
   * to all zeros.
   *
   *********************************************************************
   */
  bResult = TRUE;
  while (bResult == TRUE)
  {
    memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
    sFTFileData.dwVolumeSerialNumber = -1;
    sFTFileData.dwFileIndexHigh = -1;
    sFTFileData.dwFileIndexLow = -1;
    sFTFileData.iStreamCount = FTIMES_INVALID_STREAM_COUNT; /* The Develop{Compressed,Normal}Output routines check for this value. */
    sFTFileData.iFSType = iFSType;

#ifdef WIN98

    /*-
     *******************************************************************
     *
     * Break out early for WIN98 as it can't open "." or ".."
     *
     *******************************************************************
     */
    if (strcmp(sFindData.cFileName, FTIMES_DOT) == 0)
    {
      bResult = FindNextFile(hSearch, &sFindData);
      continue;
    }

    if (strcmp(sFindData.cFileName, FTIMES_DOTDOT) == 0)
    {
      bResult = FindNextFile(hSearch, &sFindData);
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
    iNameLength = iPathLength + 1 + (int) strlen(sFindData.cFileName);
    if (iNameLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NewRawPath = [%s\\%s], Length = [%d]: Length would exceed %d bytes.",
        acRoutine,
        pcPath,
        sFindData.cFileName,
        iNameLength,
        FTIMES_MAX_PATH - 1
        );
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      bResult = FindNextFile(hSearch, &sFindData);
      continue;
    }

    strncpy(acNewRawPath, pcPath, FTIMES_MAX_PATH);
    acNewRawPath[iPathLength] = FTIMES_SLASHCHAR;
    strcpy(&acNewRawPath[iPathLength + 1], sFindData.cFileName);
    sFTFileData.pcRawPath = acNewRawPath;

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, then continue with the
     * next entry.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->psExcludeList, acNewRawPath) != NULL)
    {
      bResult = FindNextFile(hSearch, &sFindData);
      continue;
    }

#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If the new path is matched by an exclude filter, continue with
     * the next entry. If the new path is matched by an include
     * filter, set a flag, but keep going. Include filters do not get
     * applied until the file's type is known. This is because
     * directories must be traversed before they can be filtered.
     *
     *******************************************************************
     */
    if (psProperties->psExcludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, acNewRawPath);
      if (psFilter != NULL)
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, acNewRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
        bResult = FindNextFile(hSearch, &sFindData);
        continue;
      }
    }

    iFiltered = 0;
    if (psProperties->psIncludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, acNewRawPath);
      if (psFilter == NULL)
      {
        iFiltered = 1;
      }
      else
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, acNewRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
      }
    }
#endif

    /*-
     *******************************************************************
     *
     * Neuter the given path. In other words, replace funky chars with
     * their hex value (e.g., backspace becomes %08).
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(acNewRawPath, iNameLength, acLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, acNewRawPath, acLocalError);
      ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
      bResult = FindNextFile(hSearch, &sFindData);
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
    iError = MapGetAttributes(&sFTFileData, acLocalError);
    if (iError == Have_Nothing)
    {
#ifdef USE_PCRE
      /*-
       *****************************************************************
       *
       * If this path has been filtered, we're done.
       *
       *****************************************************************
       */
      if (iFiltered)
      {
        MEMORY_FREE(pcNeuteredPath);
        bResult = FindNextFile(hSearch, &sFindData);
        continue;
      }
#endif

      /*-
       *****************************************************************
       *
       * If this is "." or "..", we're done.
       *
       *****************************************************************
       */
      if (strcmp(sFindData.cFileName, FTIMES_DOT) == 0 || strcmp(sFindData.cFileName, FTIMES_DOTDOT) == 0)
      {
        MEMORY_FREE(pcNeuteredPath);
        bResult = FindNextFile(hSearch, &sFindData);
        continue;
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, zeros will be folded into the aggregate hash.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5Cycle(&sDirFTHashData.sMd5Context, sFTFileData.aucFileMd5, MD5_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1Cycle(&sDirFTHashData.sSha1Context, sFTFileData.aucFileSha1, SHA1_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256Cycle(&sDirFTHashData.sSha256Context, sFTFileData.aucFileSha256, SHA256_HASH_SIZE);
        }
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      /*-
       *****************************************************************
       *
       * Free the neutered path, find the next file, and continue.
       *
       *****************************************************************
       */
      MEMORY_FREE(pcNeuteredPath);
      bResult = FindNextFile(hSearch, &sFindData);
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
    if (strcmp(sFindData.cFileName, FTIMES_DOT) == 0)
    {
      if (sFTFileData.iFileFlags >= Have_GetFileInformationByHandle)
      {
        hFileCurrent = MapGetFileHandle(pcPath);
        if (hFileCurrent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileCurrent, &sFileInfoCurrent))
        {
          if (sFileInfoCurrent.dwVolumeSerialNumber != sFTFileData.dwVolumeSerialNumber ||
              sFileInfoCurrent.nFileIndexHigh != sFTFileData.dwFileIndexHigh ||
              sFileInfoCurrent.nFileIndexLow != sFTFileData.dwFileIndexLow)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Volume/FileIndex mismatch between '.' and current directory.", acRoutine, pcNeuteredPath);
            ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
          }
          CloseHandle(hFileCurrent);
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to compare it to '.'.", acRoutine, pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on '.' to compare it to current directory.", acRoutine, pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else if (strcmp(sFindData.cFileName, FTIMES_DOTDOT) == 0)
    {
      /*-
       *****************************************************************
       *
       * If the file system is remote, skip this check. This is done
       * because, in testing, the file index for '..' is different than
       * the parent directory. This was found to be true with NTFS and
       * Samba shares which, by-the-way, show up as NTFS_REMOTE. For
       * now, these quirks remain unexplained. NWFS_REMOTE was added
       * here to follow suit with the other file systems -- i.e., it
       * has not been tested to see if the '..' issue exists.
       *
       *****************************************************************
       */
      if (iFSType != FSTYPE_NTFS_REMOTE && iFSType != FSTYPE_FAT_REMOTE && iFSType != FSTYPE_NWFS_REMOTE)
      {
        if (sFTFileData.iFileFlags >= Have_GetFileInformationByHandle)
        {
          hFileParent = MapGetFileHandle(acParentPath);
          if (hFileParent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileParent, &sFileInfoParent))
          {
            if (sFileInfoParent.dwVolumeSerialNumber != sFTFileData.dwVolumeSerialNumber ||
                sFileInfoParent.nFileIndexHigh != sFTFileData.dwFileIndexHigh ||
                sFileInfoParent.nFileIndexLow != sFTFileData.dwFileIndexLow)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Volume/FileIndex mismatch between '..' and parent directory.", acRoutine, pcNeuteredPath);
              ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
            }
            CloseHandle(hFileParent);
          }
          else
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on parent directory to compare it to '..'.", acRoutine, pcNeuteredPath);
            ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
          }
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on '..' to compare it to parent directory.", acRoutine, pcNeuteredPath);
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
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          snprintf(sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, "directory");
        }
#endif
        if (psProperties->bEnableRecursion)
        {
          MapTree(psProperties, acNewRawPath, iFSType, &sFTFileData, acLocalError);
        }
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
#ifdef WINNT
          MEMORY_FREE(sFTFileData.pucStreamInfo);
#endif
          MEMORY_FREE(pcNeuteredPath);
          bResult = FindNextFile(hSearch, &sFindData);
          continue;
        }
#endif
      }
      else
      {
#ifdef USE_PCRE
        if (iFiltered) /* We're done. */
        {
#ifdef WINNT
          MEMORY_FREE(sFTFileData.pucStreamInfo);
#endif
          MEMORY_FREE(pcNeuteredPath);
          bResult = FindNextFile(hSearch, &sFindData);
          continue;
        }
#endif
        giFiles++;
        if (sFTFileData.iFileFlags >= Have_MapGetFileHandle)
        {
          if (psProperties->iLastAnalysisStage > 0)
          {
            iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
            if (iError != ER_OK)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
              ErrorHandler(iError, pcError, ERROR_FAILURE);
            }
          }
        }
      }

      /*-
       *****************************************************************
       *
       * Update the directory hash. If the file was special or could
       * not be hashed, zeros will be folded into the aggregate hash.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5Cycle(&sDirFTHashData.sMd5Context, sFTFileData.aucFileMd5, MD5_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1Cycle(&sDirFTHashData.sSha1Context, sFTFileData.aucFileSha1, SHA1_HASH_SIZE);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256Cycle(&sDirFTHashData.sSha256Context, sFTFileData.aucFileSha256, SHA256_HASH_SIZE);
        }
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

#ifdef WINNT
      /*-
       *****************************************************************
       *
       * Process alternate streams. This applies only to NTFS.
       *
       *****************************************************************
       */
      if (sFTFileData.iStreamCount > 0)
      {
        MapStream(psProperties, &sFTFileData, &sDirFTHashData, acLocalError);
      }
#endif
    }

#ifdef WINNT
    /*-
     *******************************************************************
     *
     * Free the alternate stream info (including DOT and DOTDOT).
     *
     *******************************************************************
     */
    MEMORY_FREE(sFTFileData.pucStreamInfo);
#endif

    /*-
     *******************************************************************
     *
     * Free the neutered path (including DOT and DOTDOT).
     *
     *******************************************************************
     */
    MEMORY_FREE(pcNeuteredPath);

    bResult = FindNextFile(hSearch, &sFindData);
  }

  /*-
   *********************************************************************
   *
   * Complete the directory hash(es). NOTE: The caller is expected to
   * initialize each hash to all zeros.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      MD5Omega(&sDirFTHashData.sMd5Context, psParentFTData->aucFileMd5);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      SHA1Omega(&sDirFTHashData.sSha1Context, psParentFTData->aucFileSha1);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      SHA256Omega(&sDirFTHashData.sSha256Context, psParentFTData->aucFileSha256);
    }
  }

  FindClose(hSearch);
  return ER_OK;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapStream
 *
 ***********************************************************************
 */
void
MapStream(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTData, FTIMES_HASH_DATA *psFTHashData, char *pcError)
{
  const char          acRoutine[] = "MapStream()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acNewRawPath[FTIMES_MAX_PATH];
  char                acStreamName[FTIMES_MAX_PATH];
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  FILE_STREAM_INFORMATION *psFSI;
  int                 i;
  int                 iDone;
  int                 iError;
  int                 iLength;
  int                 iNameLength;
  int                 iNextEntryOffset;
  unsigned short      usUnicode;

  psFSI = (FILE_STREAM_INFORMATION *) psFTData->pucStreamInfo;

  giStreams += psFTData->iStreamCount;

  /*-
   *********************************************************************
   *
   * Make a local copy of the file data. If the stream belongs to
   * a directory, clear the attributes field. If this isn't done,
   * the output routine, will overwrite the hash field with "DIRECTORY"
   * or "D" respectively.
   *
   *********************************************************************
   */
  memcpy(&sFTFileData, psFTData, sizeof(FTIMES_FILE_DATA));

  sFTFileData.pcRawPath = acNewRawPath;

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
  for (iDone = iNextEntryOffset = 0; !iDone; iNextEntryOffset = psFSI->NextEntryOffset)
  {
    psFSI = (FILE_STREAM_INFORMATION *) ((byte *) psFSI + iNextEntryOffset);
    if (psFSI->NextEntryOffset == 0)
    {
      iDone = 1; /* Instruct the loop to terminate after this pass. */
    }

    /*-
     *******************************************************************
     *
     * The string ":$DATA" is not part of the the stream's name as it's
     * stored on disk in the MFT. For this reason, it's removed here. If
     * this is not done, normal filenames (i.e., those not prefixed with
     * '\\?\') can exceed MAX_PATH.
     *
     *******************************************************************
     */
    iLength = psFSI->StreamNameLength / sizeof(WCHAR);
    if (
         psFSI->StreamName[iLength - 6] == (WCHAR) ':' &&
         psFSI->StreamName[iLength - 5] == (WCHAR) '$' &&
         psFSI->StreamName[iLength - 4] == (WCHAR) 'D' &&
         psFSI->StreamName[iLength - 3] == (WCHAR) 'A' &&
         psFSI->StreamName[iLength - 2] == (WCHAR) 'T' &&
         psFSI->StreamName[iLength - 1] == (WCHAR) 'A'
       )
    {
      iLength -= 6;
      if (psFSI->StreamName[iLength - 1] == (WCHAR) ':')
      {
        continue; /* No name means that this is the default stream. */
      }
    }

    for (i = 0; i < iLength; i++)
    {
      usUnicode = (unsigned short)psFSI->StreamName[i];
      if (usUnicode < 0x0020 || usUnicode > 0x007e)
      {
        break;
      }
      acStreamName[i] = (char)(usUnicode & 0xff);
    }
    if (i != iLength)
    {
      char *pcStream = SupportNeuterStringW(psFSI->StreamName, iLength, acLocalError);
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s], NeuteredWideStream = [%s]: Stream skipped because its name contains Unicode or Unsafe characters.", acRoutine, psFTData->pcNeuteredPath, (pcStream == NULL) ? "" : pcStream);
      MEMORY_FREE(pcStream);
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      continue;
    }
    acStreamName[i] = 0;

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
      snprintf(pcError, MESSAGE_SIZE, "%s: NewRawPath = [%s%s], Length = [%d]: Length would exceed %d bytes.",
        acRoutine,
        psFTData->pcRawPath,
        acStreamName,
        iNameLength,
        FTIMES_MAX_PATH - 1
        );
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      continue;
    }
    sprintf(acNewRawPath, "%s%s", psFTData->pcRawPath, acStreamName);
    sFTFileData.pcRawPath = acNewRawPath;

    /*-
     *******************************************************************
     *
     * Neuter the new given path.
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(acNewRawPath, iNameLength, acLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, acNewRawPath, acLocalError);
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
    sFTFileData.dwFileSizeHigh = (DWORD) (psFSI->StreamSize.QuadPart >> 32);
    sFTFileData.dwFileSizeLow = (DWORD) psFSI->StreamSize.QuadPart;

    /*-
     *******************************************************************
     *
     * Analyze the stream's content.
     *
     *******************************************************************
     */
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }

    /*-
     *******************************************************************
     *
     * Update the directory hash. If the stream could not be hashed,
     * zeros will be folded into the aggregate hash. When psFTHashData
     * is NULL, assume that the caller was MapFile(), and skip all
     * directory hashing -- individual files, by definition, cannot
     * have this kind of hash.
     *
     *******************************************************************
     */
    if (psProperties->bHashDirectories && psFTHashData != NULL)
    {
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
      {
        MD5Cycle(&psFTHashData->sMd5Context, sFTFileData.aucFileMd5, MD5_HASH_SIZE);
      }
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
      {
        SHA1Cycle(&psFTHashData->sSha1Context, sFTFileData.aucFileSha1, SHA1_HASH_SIZE);
      }
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
      {
        SHA256Cycle(&psFTHashData->sSha256Context, sFTFileData.aucFileSha256, SHA256_HASH_SIZE);
      }
    }

    /*-
     *******************************************************************
     *
     * Record the collected data.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path.
     *
     *******************************************************************
     */
    MEMORY_FREE(pcNeuteredPath);
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
  const char          acRoutine[] = "MapCountNamedStreams()";
  char               *pcMessage = NULL;
  DWORD               dwStatus = 0;
  FILE_STREAM_INFORMATION *psFSI = NULL;
  FILE_STREAM_INFORMATION *psTempFSI = NULL;
  int                 i = 0;
  int                 iDone = 0;
  int                 iNextEntryOffset = 0;
  IO_STATUS_BLOCK     sIOStatusBlock;
  unsigned long       ulSize = 0;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x00000000
#endif
#ifndef STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW 0x80000005
#endif

  /*-
   *********************************************************************
   *
   * Make sure the provided file handle is valid.
   *
   *********************************************************************
   */
  if (hFile == INVALID_HANDLE_VALUE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid File Handle", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Request the file's stream information. Loop until enough memory
   * has been allocated to hold the data. However, do not exceed the
   * maximum size limit.
   *
   *********************************************************************
   */
  i = 0; psFSI = psTempFSI = NULL;
  do
  {
    ulSize = FTIMES_STREAM_INFO_SIZE << i;
    if (ulSize > FTIMES_MAX_STREAM_INFO_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Requested buffer size would exceed the maximum size limit (%lu bytes).", acRoutine, FTIMES_MAX_STREAM_INFO_SIZE);
      MEMORY_FREE(psFSI);
      return ER;
    }
    psTempFSI = (FILE_STREAM_INFORMATION *) realloc(psFSI, ulSize);
    if (psTempFSI == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
      MEMORY_FREE(psFSI);
      return ER;
    }
    memset(psTempFSI, 0, ulSize);
    psFSI = psTempFSI;
    dwStatus = NtdllNQIF(hFile, &sIOStatusBlock, psFSI, ulSize, FileStreamInformation);
    if (dwStatus != STATUS_SUCCESS && dwStatus != STATUS_BUFFER_OVERFLOW)
    {
      SetLastError(LsaNtStatusToWinError(dwStatus));
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: NtQueryInformationFile(): %s", acRoutine, pcMessage);
      MEMORY_FREE(psFSI);
      return ER;
    }
    i++;
  } while (dwStatus == STATUS_BUFFER_OVERFLOW);

  /*-
   *********************************************************************
   *
   * Record the final FSI pointer.
   *
   *********************************************************************
   */
  *ppucStreamInfo = (unsigned char *) psFSI;

  /*-
   *********************************************************************
   *
   * Count all but the default stream. This logic is supposed to work
   * even if NtQueryInformationFile() returns no data. This is due to
   * the fact that psFSI should be pointing to a zero-initialized
   * block of memory that is large enough to contain at least one FSI
   * struct. In other words, NextEntryOffset will be set to zero, and
   * the loop will terminate after one pass. Note: At least one pass
   * through the loop is required to catch directories that have only
   * one named stream. This is necessary because directories do not
   * have an unnamed (i.e., default) stream -- see Data attribute in
   * Table 9-1 of Inside Windows NT Second Edition, page 412.
   *
   *********************************************************************
   */
  for (*piStreamCount = iDone = iNextEntryOffset = 0; !iDone; iNextEntryOffset = psFSI->NextEntryOffset)
  {
    psFSI = (FILE_STREAM_INFORMATION *) ((byte *) psFSI + iNextEntryOffset);
    if (psFSI->NextEntryOffset == 0)
    {
      iDone = 1; /* Instruct the loop to terminate after this pass. */
    }
    if (psFSI->StreamNameLength && wcscmp(psFSI->StreamName, DEFAULT_STREAM_NAME_W) != 0)
    {
      (*piStreamCount)++;
    }
  }

  return ER_OK;
}
#endif /* WINNT */
#endif /* WIN32 */


#ifdef UNIX
int
MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError)
{
  const char          acRoutine[] = "MapFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acLinkData[FTIMES_MAX_PATH];
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  int                 iError;
  int                 iFSType;
  int                 iPathLength;
#ifdef USE_PCRE
  char                acMessage[MESSAGE_SIZE] = "";
  int                 iFiltered = 0;
#endif

  /*-
   *********************************************************************
   *
   * Initialize the sFTFileData structure. Subsequent logic relies
   * on the assertion that each hash value has been initialized to all
   * zeros.
   *
   *********************************************************************
   */
  memset(&sFTFileData, 0, sizeof(FTIMES_FILE_DATA));
  sFTFileData.pcRawPath = pcPath;

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * If the path is matched by an exclude filter, just return. If the
   * path is matched by an include filter, set a flag, but keep going.
   * Include filters do not get applied until the file's type is
   * known. This is because directories must be traversed before they
   * can be filtered.
   *
   *********************************************************************
   */
  if (psProperties->psExcludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, pcPath);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, pcPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      return ER_OK;
    }
  }

  iFiltered = 0;
  if (psProperties->psIncludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, pcPath);
    if (psFilter == NULL)
    {
      iFiltered = 1;
    }
    else
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, pcPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * Neuter the given path. In other words, replace funky chars with
   * their hex value (e.g., backspace becomes %08).
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  pcNeuteredPath = SupportNeuterString(pcPath, iPathLength, acLocalError);
  if (pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, pcPath, acLocalError);
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
  iFSType = GetFileSystemType(pcPath, acLocalError);
  if (iFSType == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
    return ER;
  }
  if (iFSType == FSTYPE_UNSUPPORTED)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
    return ER;
  }
  if ((iFSType == FSTYPE_NFS || iFSType == FSTYPE_SMB) && !psProperties->bAnalyzeRemoteFiles)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
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
  iError = MapGetAttributes(&sFTFileData, acLocalError);
  if (iError == Have_Nothing)
  {
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If this path has been filtered, we're done.
     *
     *******************************************************************
     */
    if (iFiltered)
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif

    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path and return.
     *
     *******************************************************************
     */
    MEMORY_FREE(pcNeuteredPath);
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
  if (S_ISDIR(sFTFileData.sStatEntry.st_mode))
  {
    giDirectories++;
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      snprintf(sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, "directory");
    }
#endif
    if (psProperties->bEnableRecursion)
    {
      MapTree(psProperties, pcPath, iFSType, &sFTFileData, acLocalError);
    }
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
  }
  else if (S_ISREG(sFTFileData.sStatEntry.st_mode) || ((S_ISBLK(sFTFileData.sStatEntry.st_mode) || S_ISCHR(sFTFileData.sStatEntry.st_mode)) && psProperties->bAnalyzeDeviceFiles))
  {
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
    giFiles++;
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
  }
  else if (S_ISLNK(sFTFileData.sStatEntry.st_mode))
  {
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
    giSpecial++;
    if (psProperties->bHashSymbolicLinks)
    {
      iError = readlink(pcPath, acLinkData, FTIMES_MAX_PATH - 1);
      if (iError == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Unreadable Symbolic Link: %s", acRoutine, pcNeuteredPath, strerror(errno));
        ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
      }
      else
      {
        acLinkData[iError] = 0; /* Readlink does not append a NULL. */
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileMd5);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileSha1);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256HashString((unsigned char *) acLinkData, strlen(acLinkData), sFTFileData.aucFileSha256);
        }
      }
    }
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.sStatEntry, sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
#endif
  }
  else
  {
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
    giSpecial++;
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      iError = XMagicTestSpecial(sFTFileData.pcRawPath, &sFTFileData.sStatEntry, sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
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
  iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

  /*-
   *********************************************************************
   *
   * Free the neutered path.
   *
   *********************************************************************
   */
  MEMORY_FREE(pcNeuteredPath);

  return ER_OK;
}
#endif


#ifdef WIN32
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
  const char          acRoutine[] = "MapFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcNeuteredPath;
  FTIMES_FILE_DATA    sFTFileData;
  int                 iError;
  int                 iFSType;
  int                 iPathLength;
#ifdef USE_PCRE
  char                acMessage[MESSAGE_SIZE] = "";
  int                 iFiltered = 0;
#endif

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
  sFTFileData.dwFileIndexHigh = -1;
  sFTFileData.dwFileIndexLow = -1;
  sFTFileData.iStreamCount = FTIMES_INVALID_STREAM_COUNT; /* The Develop{Compressed,Normal}Output routines check for this value. */

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * If the path is matched by an exclude filter, just return. If the
   * path is matched by an include filter, set a flag, but keep going.
   * Include filters do not get applied until the file's type is
   * known. This is because directories must be traversed before they
   * can be filtered.
   *
   *********************************************************************
   */
  if (psProperties->psExcludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, pcPath);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, pcPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      return ER_OK;
    }
  }

  iFiltered = 0;
  if (psProperties->psIncludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, pcPath);
    if (psFilter == NULL)
    {
      iFiltered = 1;
    }
    else
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, pcPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * Neuter the given path. In other words, replace funky chars with
   * their hex value (e.g., backspace becomes %08).
   *
   *********************************************************************
   */
  iPathLength = strlen(pcPath);
  pcNeuteredPath = SupportNeuterString(pcPath, iPathLength, acLocalError);
  if (pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, pcPath, acLocalError);
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
  iFSType = GetFileSystemType(pcPath, acLocalError);
  if (iFSType == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
    return ER;
  }
  if (iFSType == FSTYPE_UNSUPPORTED)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
    return ER;
  }
  if ((iFSType == FSTYPE_NTFS_REMOTE || iFSType == FSTYPE_FAT_REMOTE || iFSType == FSTYPE_NWFS_REMOTE) && !psProperties->bAnalyzeRemoteFiles)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MEMORY_FREE(pcNeuteredPath);
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
  iError = MapGetAttributes(&sFTFileData, acLocalError);
  if (iError == Have_Nothing)
  {
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If this path has been filtered, we're done.
     *
     *******************************************************************
     */
    if (iFiltered)
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif

    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path and return.
     *
     *******************************************************************
     */
    MEMORY_FREE(pcNeuteredPath);
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
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      snprintf(sFTFileData.acType, FTIMES_FILETYPE_BUFSIZE, "directory");
    }
#endif
    if (psProperties->bEnableRecursion)
    {
      MapTree(psProperties, pcPath, iFSType, &sFTFileData, acLocalError);
    }
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
  }
  else
  {
#ifdef USE_PCRE
    if (iFiltered) /* We're done. */
    {
      MEMORY_FREE(pcNeuteredPath);
      return ER_OK;
    }
#endif
    giFiles++;
    if (sFTFileData.iFileFlags >= Have_MapGetFileHandle)
    {
      if (psProperties->iLastAnalysisStage > 0)
      {
        iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
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
  iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Process alternate streams. This applies only to NTFS. The NULL
   * argument causes MapStream to skip directory hashing -- even if
   * the user has enabled HashDirectories.
   *
   *********************************************************************
   */
  if (sFTFileData.iStreamCount > 0)
  {
    MapStream(psProperties, &sFTFileData, NULL, acLocalError);
  }

  /*-
   *********************************************************************
   *
   * Free the alternate stream info.
   *
   *********************************************************************
   */
  MEMORY_FREE(sFTFileData.pucStreamInfo);
#endif

  /*-
   *********************************************************************
   *
   * Free the neutered path.
   *
   *********************************************************************
   */
  MEMORY_FREE(pcNeuteredPath);

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
  const char          acRoutine[] = "MapWriteRecord()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  int                 iWriteCount;

#ifdef UNIX
  /*-
   *********************************************************************
   *
   * name          1 (for quote) + (3 * FTIMES_MAX_PATH) + 1 (for quote)
   * dev           FTIMES_MAX_32BIT_SIZE
   * inode         FTIMES_MAX_32BIT_SIZE
   * mode          FTIMES_MAX_32BIT_SIZE
   * nlink         FTIMES_MAX_32BIT_SIZE
   * uid           FTIMES_MAX_32BIT_SIZE
   * gid           FTIMES_MAX_32BIT_SIZE
   * rdev          FTIMES_MAX_32BIT_SIZE
   * atime         FTIMES_TIME_FORMAT_SIZE
   * mtime         FTIMES_TIME_FORMAT_SIZE
   * ctime         FTIMES_TIME_FORMAT_SIZE
   * size          FTIMES_MAX_64BIT_SIZE
   * md5           FTIMES_MAX_MD5_LENGTH
   * sha1          FTIMES_MAX_SHA1_LENGTH
   * sha256        FTIMES_MAX_SHA256_LENGTH
#ifdef USE_XMAGIC
   * magic         XMAGIC_DESCRIPTION_BUFSIZE
#endif
   * |'s           13
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[(3 * FTIMES_MAX_PATH) +
                (7 * FTIMES_MAX_32BIT_SIZE) +
                (3 * FTIMES_TIME_FORMAT_SIZE) +
                (1 * FTIMES_MAX_64BIT_SIZE) +
                (1 * FTIMES_MAX_MD5_LENGTH) +
                (1 * FTIMES_MAX_SHA1_LENGTH) +
                (1 * FTIMES_MAX_SHA256_LENGTH) +
#ifdef USE_XMAGIC
                (1 * XMAGIC_DESCRIPTION_BUFSIZE) +
#endif
                17
                ];
#endif

#ifdef WIN32
  /*-
   *********************************************************************
   *
   * name          1 (for quote) + (3 * FTIMES_MAX_PATH) + 1 (for quote)
   * volume        FTIMES_MAX_32BIT_SIZE
   * findex        FTIMES_MAX_64BIT_SIZE
   * attributes    FTIMES_MAX_32BIT_SIZE
   * atime|ams     FTIMES_TIME_FORMAT_SIZE
   * mtime|mms     FTIMES_TIME_FORMAT_SIZE
   * ctime|cms     FTIMES_TIME_FORMAT_SIZE
   * chtime|chms   FTIMES_TIME_FORMAT_SIZE
   * size          FTIMES_MAX_64BIT_SIZE
   * altstreams    FTIMES_MAX_32BIT_SIZE
   * md5           FTIMES_MAX_MD5_LENGTH
   * sha1          FTIMES_MAX_SHA1_LENGTH
   * sha256        FTIMES_MAX_SHA256_LENGTH
#ifdef USE_XMAGIC
   * magic         XMAGIC_DESCRIPTION_BUFSIZE
#endif
   * |'s           11 (not counting those embedded in time)
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[(3 * FTIMES_MAX_PATH) +
                (3 * FTIMES_MAX_32BIT_SIZE) +
                (4 * FTIMES_TIME_FORMAT_SIZE) +
                (2 * FTIMES_MAX_64BIT_SIZE) +
                (1 * FTIMES_MAX_MD5_LENGTH) +
                (1 * FTIMES_MAX_SHA1_LENGTH) +
                (1 * FTIMES_MAX_SHA256_LENGTH) +
#ifdef USE_XMAGIC
                (1 * XMAGIC_DESCRIPTION_BUFSIZE) +
#endif
                15
                ];
#endif

  /*-
   *********************************************************************
   *
   * Initialize the write count. Format or compress the output.
   *
   *********************************************************************
   */
  iWriteCount = 0;
  iError = psProperties->piDevelopMapOutput(psProperties, acOutput, &iWriteCount, psFTData, acLocalError);

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
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s], NullFields = [%s]", acRoutine, psFTData->pcNeuteredPath, acLocalError);
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
  iError = SupportWriteData(psProperties->pFileOut, acOutput, iWriteCount, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output file hash.
   *
   *********************************************************************
   */
  MD5Cycle(&psProperties->sOutFileHashContext, acOutput, iWriteCount);

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
  const char          acRoutine[] = "MapWriteHeader()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acHeaderData[FTIMES_MAX_LINE] = { 0 };
  int                 i = 0;
  int                 iError = 0;
  int                 iIndex = 0;
  int                 iMaskTableLength = MaskGetTableLength(MASK_RUNMODE_TYPE_MAP);
  MASK_B2S_TABLE     *psMaskTable = MaskGetTableReference(MASK_RUNMODE_TYPE_MAP);
  unsigned long       ul = 0;

  /*-
   *********************************************************************
   *
   * Initialize the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Alpha(&psProperties->sOutFileHashContext);

  /*-
   *********************************************************************
   *
   * Build the output's header.
   *
   *********************************************************************
   */
  if (psProperties->bCompress)
  {
    iIndex = sprintf(acHeaderData, "z_name");
    for (i = 0; i < iMaskTableLength; i++)
    {
      ul = (1 << i);
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, ul))
      {
#ifdef WIN32
        switch (ul)
        {
        case MAP_ATIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_atime|z_ams");
          break;
        case MAP_MTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_mtime|z_mms");
          break;
        case MAP_CTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_ctime|z_cms");
          break;
        case MAP_CHTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_chtime|z_chms");
          break;
        default:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_%s", (char *) psMaskTable[i].acName);
          break;
        }
#else
        iIndex += sprintf(&acHeaderData[iIndex], "|z_%s", (char *) psMaskTable[i].acName);
#endif
      }
    }
  }
  else
  {
    iIndex = sprintf(acHeaderData, "name");
    for (i = 0; i < iMaskTableLength; i++)
    {
      ul = (1 << i);
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, ul))
      {
#ifdef WIN32
        switch (ul)
        {
        case MAP_ATIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|atime|ams");
          break;
        case MAP_MTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|mtime|mms");
          break;
        case MAP_CTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|ctime|cms");
          break;
        case MAP_CHTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|chtime|chms");
          break;
        default:
          iIndex += sprintf(&acHeaderData[iIndex], "|%s", (char *) psMaskTable[i].acName);
          break;
        }
#else
        iIndex += sprintf(&acHeaderData[iIndex], "|%s", (char *) psMaskTable[i].acName);
#endif
      }
    }
  }
  iIndex += sprintf(&acHeaderData[iIndex], "%s", psProperties->acNewLine);

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, acHeaderData, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Cycle(&psProperties->sOutFileHashContext, acHeaderData, iIndex);

  return ER_OK;
}


#ifdef UNIX
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
  const char          acRoutine[] = "MapGetAttributes()";

  psFTData->iFileFlags = Have_Nothing;

  /*-
   *********************************************************************
   *
   * Collect attributes. Use lstat() so links aren't followed.
   *
   *********************************************************************
   */
  if (lstat(psFTData->pcRawPath, &psFTData->sStatEntry) != ER)
  {
    psFTData->iFileFlags = Have_lstat;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: lstat(): %s", acRoutine, psFTData->pcNeuteredPath, strerror(errno));
    ErrorHandler(ER_lstat, pcError, ERROR_FAILURE);
    pcError[0] = 0;
  }
  return psFTData->iFileFlags;
}
#endif


#ifdef WIN32
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
  const char          acRoutine[] = "MapGetAttributes()";
  BY_HANDLE_FILE_INFORMATION sFileInfo;
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcMessage;
  HANDLE              hFile;
  WIN32_FILE_ATTRIBUTE_DATA sFileAttributeData;

#ifdef WIN98
  DWORD               dwFileAttributes;
#endif

#ifdef WINNT
  DWORD               dwStatus;
  FILE_BASIC_INFORMATION sFileBasicInfo;
  IO_STATUS_BLOCK     sIOStatusBlock;
  int                 iStatus;
#endif

  psFTData->iFileFlags = Have_Nothing;

#ifdef WIN98
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
      if (GetFileAttributesEx(psFTData->pcRawPath, GetFileExInfoStandard, &sFileAttributeData))
      {
        psFTData->iFileFlags              = Have_GetFileAttributesEx;
        psFTData->dwFileAttributes        = sFileAttributeData.dwFileAttributes;
        psFTData->sFTATime.dwLowDateTime  = sFileAttributeData.ftLastAccessTime.dwLowDateTime;
        psFTData->sFTATime.dwHighDateTime = sFileAttributeData.ftLastAccessTime.dwHighDateTime;
        psFTData->sFTMTime.dwLowDateTime  = sFileAttributeData.ftLastWriteTime.dwLowDateTime;
        psFTData->sFTMTime.dwHighDateTime = sFileAttributeData.ftLastWriteTime.dwHighDateTime;
        psFTData->sFTCTime.dwLowDateTime  = sFileAttributeData.ftCreationTime.dwLowDateTime;
        psFTData->sFTCTime.dwHighDateTime = sFileAttributeData.ftCreationTime.dwHighDateTime;
        psFTData->dwFileSizeHigh          = sFileAttributeData.nFileSizeHigh;
        psFTData->dwFileSizeLow           = sFileAttributeData.nFileSizeLow;
      }
      else
      {
        ErrorFormatWin32Error(&pcMessage);
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileAttributesEx(): %s", acRoutine, psFTData->pcNeuteredPath, pcMessage);
        ErrorHandler(ER_GetFileAttrsEx, pcError, ERROR_FAILURE);
        pcError[0] = 0;
      }
      return psFTData->iFileFlags;
    }
  }
  else
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileAttributes(): %s", acRoutine, psFTData->pcNeuteredPath, pcMessage);
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
    if (GetFileInformationByHandle(hFile, &sFileInfo))
    {
      psFTData->iFileFlags               = Have_GetFileInformationByHandle;
      psFTData->dwVolumeSerialNumber     = sFileInfo.dwVolumeSerialNumber;
      psFTData->dwFileIndexHigh          = sFileInfo.nFileIndexHigh;
      psFTData->dwFileIndexLow           = sFileInfo.nFileIndexLow;
      psFTData->dwFileAttributes         = sFileInfo.dwFileAttributes;
      psFTData->sFTATime.dwLowDateTime   = sFileInfo.ftLastAccessTime.dwLowDateTime;
      psFTData->sFTATime.dwHighDateTime  = sFileInfo.ftLastAccessTime.dwHighDateTime;
      psFTData->sFTMTime.dwLowDateTime   = sFileInfo.ftLastWriteTime.dwLowDateTime;
      psFTData->sFTMTime.dwHighDateTime  = sFileInfo.ftLastWriteTime.dwHighDateTime;
      psFTData->sFTCTime.dwLowDateTime   = sFileInfo.ftCreationTime.dwLowDateTime;
      psFTData->sFTCTime.dwHighDateTime  = sFileInfo.ftCreationTime.dwHighDateTime;
      psFTData->sFTChTime.dwLowDateTime  = 0;
      psFTData->sFTChTime.dwHighDateTime = 0;
      psFTData->dwFileSizeHigh           = sFileInfo.nFileSizeHigh;
      psFTData->dwFileSizeLow            = sFileInfo.nFileSizeLow;
    }
    else
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileInformationByHandle(): %s", acRoutine, psFTData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_GetFileInfo, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }

#ifdef WINNT
    memset(&sFileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
    dwStatus = NtdllNQIF(hFile, &sIOStatusBlock, &sFileBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    if (dwStatus == 0)
    {
      psFTData->iFileFlags               = Have_NTQueryInformationFile;
      psFTData->dwFileAttributes         = sFileBasicInfo.FileAttributes;
      psFTData->sFTATime.dwLowDateTime   = sFileBasicInfo.LastAccessTime.LowPart;
      psFTData->sFTATime.dwHighDateTime  = sFileBasicInfo.LastAccessTime.HighPart;
      psFTData->sFTMTime.dwLowDateTime   = sFileBasicInfo.LastWriteTime.LowPart;
      psFTData->sFTMTime.dwHighDateTime  = sFileBasicInfo.LastWriteTime.HighPart;
      psFTData->sFTCTime.dwLowDateTime   = sFileBasicInfo.CreationTime.LowPart;
      psFTData->sFTCTime.dwHighDateTime  = sFileBasicInfo.CreationTime.HighPart;
      psFTData->sFTChTime.dwLowDateTime  = sFileBasicInfo.ChangeTime.LowPart;
      psFTData->sFTChTime.dwHighDateTime = sFileBasicInfo.ChangeTime.HighPart;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: NtdllNQIF(): %08x", acRoutine, psFTData->pcNeuteredPath, dwStatus);
      ErrorHandler(ER_NQIF, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }

    /*-
     *********************************************************************
     *
     * Determine the number of alternate streams. This check applies to
     * files and directories, and it is NTFS specific. A valid handle
     * is required to perform the check.
     *
     *********************************************************************
     */
    if (psFTData->iFSType == FSTYPE_NTFS)
    {
      iStatus = MapCountNamedStreams(hFile, &psFTData->iStreamCount, &psFTData->pucStreamInfo, acLocalError);
      if (iStatus == ER_OK)
      {
        psFTData->iFileFlags = Have_MapCountNamedStreams;
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Stream Count Failed: %s", acRoutine, psFTData->pcNeuteredPath, acLocalError);
        ErrorHandler(ER_MapCountNamedStreams, pcError, ERROR_FAILURE);
        pcError[0] = 0;
      }
    }
#endif
    CloseHandle(hFile);
  }
  else
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: MapGetFileHandle(): %s", acRoutine, psFTData->pcNeuteredPath, pcMessage);
    ErrorHandler(ER_CreateFile, pcError, ERROR_FAILURE);
    pcError[0] = 0;

    if (GetFileAttributesEx(psFTData->pcRawPath, GetFileExInfoStandard, &sFileAttributeData))
    {
      psFTData->iFileFlags              = Have_GetFileAttributesEx;
      psFTData->dwFileAttributes        = sFileAttributeData.dwFileAttributes;
      psFTData->sFTATime.dwLowDateTime  = sFileAttributeData.ftLastAccessTime.dwLowDateTime;
      psFTData->sFTATime.dwHighDateTime = sFileAttributeData.ftLastAccessTime.dwHighDateTime;
      psFTData->sFTMTime.dwLowDateTime  = sFileAttributeData.ftLastWriteTime.dwLowDateTime;
      psFTData->sFTMTime.dwHighDateTime = sFileAttributeData.ftLastWriteTime.dwHighDateTime;
      psFTData->sFTCTime.dwLowDateTime  = sFileAttributeData.ftCreationTime.dwLowDateTime;
      psFTData->sFTCTime.dwHighDateTime = sFileAttributeData.ftCreationTime.dwHighDateTime;
      psFTData->dwFileSizeHigh          = sFileAttributeData.nFileSizeHigh;
      psFTData->dwFileSizeLow           = sFileAttributeData.nFileSizeLow;
    }
    else
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileAttributesEx(): %s", acRoutine, psFTData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_GetFileAttrsEx, pcError, ERROR_FAILURE);
      pcError[0] = 0;
    }
  }
  return psFTData->iFileFlags;
}
#endif
