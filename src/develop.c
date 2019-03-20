/*-
 ***********************************************************************
 *
 * $Id: develop.c,v 1.10 2003/08/13 21:39:49 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define COMPRESS_RECOVERY_RATE 100

static unsigned char  ucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char  ucZeroHash[MD5_HASH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


/*-
 ***********************************************************************
 *
 * DevelopNoOutput
 *
 ***********************************************************************
 */
int
DevelopNoOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  /*-
   *********************************************************************
   *
   * Set the write count for the caller.
   *
   *********************************************************************
   */
  *iWriteCount = 0;

  return ER_OK;
}


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * DevelopNormalOutput
 *
 ***********************************************************************
 */
int
DevelopNormalOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  char                cTime[FTIMES_TIME_FORMAT_SIZE];
  int                 i,
                      n,
                      iError,
                      iStatus;

  iStatus = ER_OK;

  /*-
   *********************************************************************
   *
   * This is required since only strcats are used below.
   *
   *********************************************************************
   */
  pcError[0] = 0;

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  n = sprintf(pcOutData, "\"%s\"", psFTData->pcNeuteredPath);

  /*-
   *********************************************************************
   *
   * Device = dev
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & DEV_SET) == DEV_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_dev);
  }

  /*-
   *********************************************************************
   *
   * Inode = inode
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & INODE_SET) == INODE_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_ino);
  }

  /*-
   *********************************************************************
   *
   * Permissions and Mode = mode
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MODE_SET) == MODE_SET)
  {
    n += sprintf(&pcOutData[n], "|%o", (unsigned) psFTData->statEntry.st_mode);
  }

  /*-
   *********************************************************************
   *
   * Number of Links = nlink
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & NLINK_SET) == NLINK_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_nlink);
  }

  /*-
   *********************************************************************
   *
   * User ID = uid
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & UID_SET) == UID_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_uid);
  }

  /*-
   *********************************************************************
   *
   * Group ID = gid
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & GID_SET) == GID_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_gid);
  }

  /*-
   *********************************************************************
   *
   * Special Device Type = rdev
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & RDEV_SET) == RDEV_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->statEntry.st_rdev);
  }

  /*-
   *********************************************************************
   *
   * Last Access Time = atime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET)
  {
    iError = TimeFormatTime(&psFTData->statEntry.st_atime, cTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", cTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, "atime,");
      iStatus = ER_NullFields;
    }
  }

  /*-
   *********************************************************************
   *
   * Last Modification Time = mtime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MTIME_SET) == MTIME_SET)
  {
    iError = TimeFormatTime(&psFTData->statEntry.st_mtime, cTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", cTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, "mtime,");
      iStatus = ER_NullFields;
    }
  }

  /*-
   *********************************************************************
   *
   * Last Status Change Time = ctime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CTIME_SET) == CTIME_SET)
  {
    iError = TimeFormatTime(&psFTData->statEntry.st_ctime, cTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", cTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, "ctime,");
      iStatus = ER_NullFields;
    }
  }

  /*-
   *********************************************************************
   *
   * File Size = size
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & SIZE_SET) == SIZE_SET)
  {
#ifdef USE_AP_SNPRINTF
    /*-
     *******************************************************************
     *
     * The USE_AP_SNPRINTF is needed to support certain versions of
     * Solaris that can't grok %q.
     *
     *******************************************************************
     */
    n += snprintf(&pcOutData[n], 22, "|%qu", (unsigned long long) psFTData->statEntry.st_size);
#else
    n += sprintf(&pcOutData[n], "|%qu", (unsigned long long) psFTData->statEntry.st_size);
#endif
  }

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
#ifdef USE_XMAGIC
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->cType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTData->cType);
    }
    else
    {
      strcat(pcError, "magic,");
      iStatus = ER_NullFields;
    }
  }
#else
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
  }
#endif


  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->statEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0; i < MD5_HASH_SIZE; i++)
          {
            n += sprintf(&pcOutData[n], "%02x", psFTData->ucFileMD5[i]);
          }
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTData->statEntry.st_mode))
    {
      if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
      {
        for (i = 0; i < MD5_HASH_SIZE; i++)
        {
          n += sprintf(&pcOutData[n], "%02x", psFTData->ucFileMD5[i]);
        }
      }
      else
      {
        strcat(pcError, "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->statEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0; i < MD5_HASH_SIZE; i++)
          {
            n += sprintf(&pcOutData[n], "%02x", psFTData->ucFileMD5[i]);
          }
        }
        else
        {
          strcat(pcError, "md5");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "SYMLINK");
      }
    }
    else
    {
      n += sprintf(&pcOutData[n], "SPECIAL");
    }
  }

  n += sprintf(&pcOutData[n], "%s", psProperties->cNewLine);

  /*-
   *********************************************************************
   *
   * Set the write count for the caller.
   *
   *********************************************************************
   */
  *iWriteCount = n;

  return iStatus;
}
#endif


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * DevelopNormalOutput
 *
 ***********************************************************************
 */
int
DevelopNormalOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  char                cTime[FTIMES_TIME_FORMAT_SIZE];
  int                 iError,
                      i,
                      n,
                      iStatus;
  unsigned __int64    ui64FileIndex,
                      ui64FileSize;
  static DWORD        dwLastVolumeSerialNumber,
                      dwLastFileIndexHigh,
                      dwLastFileIndexLow,
                      dwLastFileAttributes;
  static unsigned __int64 ui64LastATime,
                      ui64LastMTime,
                      ui64LastCTime,
                      ui64LastChTime;

  iStatus = ER_OK;

  /*-
   *********************************************************************
   *
   * This is required since only strcats are used below.
   *
   *********************************************************************
   */
  pcError[0] = 0;

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  n = sprintf(pcOutData, "\"%s\"", psFTData->pcNeuteredPath);

  /*-
   *********************************************************************
   *
   * Volume Number = volume
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & VOLUME_SET) == VOLUME_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->dwVolumeSerialNumber != 0xffffffff)
    {
      n += sprintf(&pcOutData[n], "%u", psFTData->dwVolumeSerialNumber);
    }
    else
    {
#ifdef WIN98
      if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
      {
        strcat(pcError, "volume,");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Index = findex
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & FINDEX_SET) == FINDEX_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->nFileIndexHigh != 0xffffffff && psFTData->nFileIndexLow != 0xffffffff)
    {
      ui64FileIndex = (((unsigned __int64) psFTData->nFileIndexHigh) << 32) | psFTData->nFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64u", ui64FileIndex);
    }
    else
    {
#ifdef WIN98
      if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
      {
        strcat(pcError, "findex,");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Attributes = attributes
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATTRIBUTES_SET) == ATTRIBUTES_SET)
  {
    n += sprintf(&pcOutData[n], "|%u", psFTData->dwFileAttributes);
  }

  /*-
   *********************************************************************
   *
   * Last Access Time = atime|ams
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET)
  {
    if (psFTData->ftLastAccessTime.dwLowDateTime == 0 && psFTData->ftLastAccessTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, "atime,");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->ftLastAccessTime, cTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", cTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, "atime,");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Write Time = mtime|mms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MTIME_SET) == MTIME_SET)
  {
    if (psFTData->ftLastWriteTime.dwLowDateTime == 0 && psFTData->ftLastWriteTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, "mtime,");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->ftLastWriteTime, cTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", cTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, "mtime,");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Creation Time = ctime|cms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CTIME_SET) == CTIME_SET)
  {
    if (psFTData->ftCreationTime.dwLowDateTime == 0 && psFTData->ftCreationTime.dwHighDateTime == 0)
    {
#ifndef WIN98

      /*-
       *****************************************************************
       *
       * Win 98 has many many files with Creation Time == 0 This ifdef
       * prevents a boat load of warning messages.
       *
       *****************************************************************
       */
      strcat(pcError, "ctime,");
      iStatus = ER_NullFields;
#endif
      n += sprintf(&pcOutData[n], "||");
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->ftCreationTime, cTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", cTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, "ctime,");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Change Time = chtime|chms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CHTIME_SET) == CHTIME_SET)
  {
    if (psFTData->ftChangeTime.dwLowDateTime == 0 && psFTData->ftChangeTime.dwHighDateTime == 0)
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, "chtime,");
        iStatus = ER_NullFields;
      }
#endif
      n += sprintf(&pcOutData[n], "||");
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->ftChangeTime, cTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", cTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        if (psFTData->iFSType == FSTYPE_NTFS)
        {
          strcat(pcError, "chtime,");
          iStatus = ER_NullFields;
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Size = size
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & SIZE_SET) == SIZE_SET)
  {
    ui64FileSize = (((unsigned __int64) psFTData->nFileSizeHigh) << 32) | psFTData->nFileSizeLow;
    n += sprintf(&pcOutData[n], "|%I64u", ui64FileSize);
  }

  /*-
   *********************************************************************
   *
   * Number of Alternate Streams = altstreams
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ALTSTREAMS_SET) == ALTSTREAMS_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->iStreamCount != 0xffffffff)
    {
      n += sprintf(&pcOutData[n], "%u", psFTData->iStreamCount);
    }
    else
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, "altstreams,");
        iStatus = ER_NullFields;
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
#ifdef USE_XMAGIC
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->cType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTData->cType);
    }
    else
    {
      strcat(pcError, "magic,");
      iStatus = ER_NullFields;
    }
  }
#else
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
  }
#endif

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0; i < MD5_HASH_SIZE; i++)
          {
            n += sprintf(&pcOutData[n], "%02x", psFTData->ucFileMD5[i]);
          }
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
      {
        for (i = 0; i < MD5_HASH_SIZE; i++)
        {
          n += sprintf(&pcOutData[n], "%02x", psFTData->ucFileMD5[i]);
        }
      }
      else
      {
        strcat(pcError, "md5");
        iStatus = ER_NullFields;
      }
    }
  }
  n += sprintf(&pcOutData[n], "%s", psProperties->cNewLine);

  /*-
   *********************************************************************
   *
   * Set the write count for the caller.
   *
   *********************************************************************
   */
  *iWriteCount = n;

  return iStatus;
}
#endif


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * DevelopCompressedOutput
 *
 ***********************************************************************
 */
int
DevelopCompressedOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  int                 i;
  int                 n;
  int                 iStatus;
  unsigned long       x;
  unsigned long       ulLeft;
  static struct stat  statLastEntry;
  static long         lRecoveryCounter = 0;
  static char         cLastName[4 * FTIMES_MAX_PATH]; /* This is an encoded name. */

  iStatus = ER_OK;

  /*-
   *********************************************************************
   *
   * This is required since only strcats are used below.
   *
   *********************************************************************
   */
  pcError[0] = 0;

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  if (lRecoveryCounter == 0)
  {
    n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    strncpy(cLastName, psFTData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
  }
  else
  {
    /*-
     *******************************************************************
     *
     * Compress name by appending repeat count and deleting letters in
     * common with previous name.
     *
     *******************************************************************
     */
    i = 0;
    if (cLastName[0] == '\0')
    {
      n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (cLastName[i] != '\0') &&
             (psFTData->pcNeuteredPath[i] != '\0') &&
             (cLastName[i] == psFTData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTData->pcNeuteredPath[i]);
    }
    strncpy(&cLastName[i], &psFTData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
  }

  /*-
   *********************************************************************
   *
   * Device = dev
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & DEV_SET) == DEV_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_dev);
    }
    else
    {
      if (psFTData->statEntry.st_dev == statLastEntry.st_dev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_dev);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Inode = inode
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & INODE_SET) == INODE_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_ino);
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTData->statEntry.st_ino, statLastEntry.st_ino);
    }
  }

  /*-
   *********************************************************************
   *
   * Permissions and Mode = mode
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MODE_SET) == MODE_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_mode);
    }
    else
    {
      if (psFTData->statEntry.st_mode == statLastEntry.st_mode)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_mode);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Number of Links = nlink
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & NLINK_SET) == NLINK_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_nlink);
    }
    else
    {
      if (psFTData->statEntry.st_nlink == statLastEntry.st_nlink)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_nlink);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * User ID = uid
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & UID_SET) == UID_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_uid);
    }
    else
    {
      if (psFTData->statEntry.st_uid == statLastEntry.st_uid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_uid);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Group ID = gid
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & GID_SET) == GID_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_gid);
    }
    else
    {
      if (psFTData->statEntry.st_gid == statLastEntry.st_gid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_gid);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Special Device Type = rdev
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & RDEV_SET) == RDEV_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_rdev);
    }
    else
    {
      if (psFTData->statEntry.st_rdev == statLastEntry.st_rdev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_rdev);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Access Time = atime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_atime);
    }
    else if (psFTData->statEntry.st_atime == statLastEntry.st_atime)
    {
      pcOutData[n++] = '#';
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTData->statEntry.st_atime, statLastEntry.st_atime);
    }
  }

  /*-
   *********************************************************************
   *
   * Last Modification Time = mtime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MTIME_SET) == MTIME_SET)
  {
    pcOutData[n++] = '|';
    if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && psFTData->statEntry.st_mtime == psFTData->statEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_mtime);
    }
    else
    {
      if (psFTData->statEntry.st_mtime == statLastEntry.st_mtime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTData->statEntry.st_mtime, statLastEntry.st_mtime);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Status Change Time = ctime
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CTIME_SET) == CTIME_SET)
  {
    pcOutData[n++] = '|';
    if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && psFTData->statEntry.st_ctime == psFTData->statEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if ((psProperties->ulFieldMask & MTIME_SET) == MTIME_SET && psFTData->statEntry.st_ctime == psFTData->statEntry.st_mtime)
    {
      pcOutData[n++] = 'Y';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->statEntry.st_ctime);
    }
    else
    {
      if (psFTData->statEntry.st_ctime == statLastEntry.st_ctime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTData->statEntry.st_ctime, statLastEntry.st_ctime);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Size = size
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & SIZE_SET) == SIZE_SET)
  {
    pcOutData[n++] = '|';
#ifdef USE_AP_SNPRINTF
    /*-
     *******************************************************************
     *
     * The USE_AP_SNPRINTF is needed to support certain versions of
     * Solaris that can't grok %q.
     *
     *******************************************************************
     */
    n += snprintf(&pcOutData[n], 18, "%qx", (unsigned long long) psFTData->statEntry.st_size);
#else
    n += sprintf(&pcOutData[n], "%qx", (unsigned long long) psFTData->statEntry.st_size);
#endif
  }

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->statEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0, x = 0, ulLeft = 0; i < MD5_HASH_SIZE; i++)
          {
            x = (x << 8) | psFTData->ucFileMD5[i];
            ulLeft += 8;
            while (ulLeft > 6)
            {
              pcOutData[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
              ulLeft -= 6;
            }
          }
          if (ulLeft != 0)
          {
            pcOutData[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
          }
        }
        else
        {
          strcat(pcError, "md5");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'D';
      }
    }
    else if (S_ISREG(psFTData->statEntry.st_mode))
    {
      if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
      {
        for (i = 0, x = 0, ulLeft = 0; i < MD5_HASH_SIZE; i++)
        {
          x = (x << 8) | psFTData->ucFileMD5[i];
          ulLeft += 8;
          while (ulLeft > 6)
          {
            pcOutData[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
            ulLeft -= 6;
          }
        }
        if (ulLeft != 0)
        {
          pcOutData[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
        }
      }
      else
      {
        strcat(pcError, "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->statEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0, x = 0, ulLeft = 0; i < MD5_HASH_SIZE; i++)
          {
            x = (x << 8) | psFTData->ucFileMD5[i];
            ulLeft += 8;
            while (ulLeft > 6)
            {
              pcOutData[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
              ulLeft -= 6;
            }
          }
          if (ulLeft != 0)
          {
            pcOutData[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
          }
        }
        else
        {
          strcat(pcError, "md5");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'L';
      }
    }
    else
    {
      pcOutData[n++] = 'S';
    }
  }

  n += sprintf(&pcOutData[n], "%s", psProperties->cNewLine);

  /*-
   *********************************************************************
   *
   * Copy psFTData->statEntry to statLastEntry for next time around.
   *
   *********************************************************************
   */
  memcpy(&statLastEntry, &psFTData->statEntry, sizeof(struct stat));

  if (++lRecoveryCounter >= COMPRESS_RECOVERY_RATE)
  {
    lRecoveryCounter = 0;
  }

  /*-
   *********************************************************************
   *
   * Set the write count for the caller.
   *
   *********************************************************************
   */
  *iWriteCount = n;

  return iStatus;
}
#endif


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * DevelopCompressedOutput
 *
 ***********************************************************************
 */
int
DevelopCompressedOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  char                cTime[FTIMES_TIME_FORMAT_SIZE];
  int                 i,
                      n,
                      iError,
                      iStatus;
  unsigned long       x,
                      ulATimeSeconds,
                      ulMTimeSeconds,
                      ulCTimeSeconds,
                      ulChTimeSeconds,
                      ulATimeMilliseconds,
                      ulMTimeMilliseconds,
                      ulCTimeMilliseconds,
                      ulChTimeMilliseconds,
                      ulTempATimeSeconds,
                      ulTempMTimeSeconds,
                      ulTempCTimeSeconds,
                      ulTempChTimeSeconds,
                      ulLeft;
  unsigned __int64    ui64ATime,
                      ui64MTime,
                      ui64CTime,
                      ui64ChTime,
                      ui64FileIndex,
                      ui64FileSize;
  static char         cLastName[4 * FTIMES_MAX_PATH]; /* This is an encoded name. */
  static DWORD        dwLastVolumeSerialNumber,
                      dwLastFileIndexHigh,
                      dwLastFileIndexLow,
                      dwLastFileAttributes;
  static unsigned __int64 ui64LastATime,
                      ui64LastMTime,
                      ui64LastCTime,
                      ui64LastChTime;
  static long         lRecoveryCounter = 0;

  iStatus = ER_OK;

  /*-
   *********************************************************************
   *
   * This is required since only strcats are used below.
   *
   *********************************************************************
   */
  pcError[0] = 0;

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  if (lRecoveryCounter == 0)
  {
    n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    strncpy(cLastName, psFTData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
  }
  else
  {
    /*-
     *******************************************************************
     *
     * Compress name by appending repeat count and deleting letters in
     * common with previous name.
     *
     *******************************************************************
     */
    i = 0;
    if (cLastName[0] == '\0')
    {
      n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (cLastName[i] != '\0') &&
             (psFTData->pcNeuteredPath[i] != '\0') &&
             (cLastName[i] == psFTData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTData->pcNeuteredPath[i]);
    }
    strncpy(&cLastName[i], &psFTData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
  }

  /*-
   *********************************************************************
   *
   * Volume Number = volume
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & VOLUME_SET) == VOLUME_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", psFTData->dwVolumeSerialNumber);
    }
    else
    {
      if (psFTData->dwVolumeSerialNumber != 0xffffffff)
      {
        if (psFTData->dwVolumeSerialNumber == dwLastVolumeSerialNumber)
        {
          pcOutData[n++] = '#';
        }
        else
        {
          n += sprintf(&pcOutData[n], "%x", psFTData->dwVolumeSerialNumber);
        }
      }
      else
      {
#ifdef WIN98
        if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
        {
          strcat(pcError, "volume,");
          iStatus = ER_NullFields;
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Index = findex
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & FINDEX_SET) == FINDEX_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      ui64FileIndex = (((unsigned __int64) psFTData->nFileIndexHigh) << 32) | psFTData->nFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64x", ui64FileIndex);
    }
    else
    {
      if (psFTData->nFileIndexHigh != 0xffffffff && psFTData->nFileIndexLow != 0xffffffff)
      {
        if (psFTData->nFileIndexHigh == dwLastFileIndexHigh)
        {
          pcOutData[n++] = '#';
          n += DevelopCompressHex(&pcOutData[n], psFTData->nFileIndexLow, dwLastFileIndexLow);
        }
        else
        {
          ui64FileIndex = (((unsigned __int64) psFTData->nFileIndexHigh) << 32) | psFTData->nFileIndexLow;
          n += sprintf(&pcOutData[n], "%I64x", ui64FileIndex);
        }
      }
      else
      {
#ifdef WIN98
        if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
        {
          strcat(pcError, "findex,");
          iStatus = ER_NullFields;
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Attributes = attributes
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATTRIBUTES_SET) == ATTRIBUTES_SET)
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", psFTData->dwFileAttributes);
    }
    else
    {
      if (psFTData->dwFileAttributes == dwLastFileAttributes)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", psFTData->dwFileAttributes);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Access Time = atime|ams
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->ftLastAccessTime.dwLowDateTime == 0 && psFTData->ftLastAccessTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, "atime,");
      iStatus = ER_NullFields;

      /*-
       *****************************************************************
       *
       * Ensure that ui64LastATime will be properly initialized.
       *
       *****************************************************************
       */
      ui64ATime = 0;
    }
    else
    {
      ui64ATime = (((unsigned __int64) psFTData->ftLastAccessTime.dwHighDateTime) << 32) | psFTData->ftLastAccessTime.dwLowDateTime;
      if (ui64ATime < UNIX_EPOCH_IN_NT_TIME || ui64ATime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->ftLastAccessTime, cTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", cTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, "atime,");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        ulATimeSeconds = (unsigned long) ((ui64ATime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulATimeMilliseconds = (unsigned long) (((ui64ATime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if (lRecoveryCounter == 0)
        {
          n += sprintf(&pcOutData[n], "%x|%x", ulATimeSeconds, ulATimeMilliseconds);
        }
        else
        {
          if (ui64ATime == ui64LastATime)
          {
            pcOutData[n++] = '#';
            pcOutData[n++] = '|';
            pcOutData[n++] = '#';
          }
          else
          {
            ulTempATimeSeconds = (unsigned long) ((ui64LastATime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
            n += DevelopCompressHex(&pcOutData[n], ulATimeSeconds, ulTempATimeSeconds);
            n += sprintf(&pcOutData[n], "|%x", ulATimeMilliseconds);
          }
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Write Time = mtime|mms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MTIME_SET) == MTIME_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->ftLastWriteTime.dwLowDateTime == 0 && psFTData->ftLastWriteTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, "mtime,");
      iStatus = ER_NullFields;

      /*-
       *****************************************************************
       *
       * Ensure that ui64LastMTime will be properly initialized.
       *
       *****************************************************************
       */
      ui64MTime = 0;
    }
    else
    {
      ui64MTime = (((unsigned __int64) psFTData->ftLastWriteTime.dwHighDateTime) << 32) | psFTData->ftLastWriteTime.dwLowDateTime;
      if (ui64MTime < UNIX_EPOCH_IN_NT_TIME || ui64MTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->ftLastWriteTime, cTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", cTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, "mtime,");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        ulMTimeSeconds = (unsigned long) ((ui64MTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulMTimeMilliseconds = (unsigned long) (((ui64MTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && ui64MTime == ui64ATime)
        {
          pcOutData[n++] = 'X';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'X';
        }
        else if (lRecoveryCounter == 0)
        {
          n += sprintf(&pcOutData[n], "%x|%x", ulMTimeSeconds, ulMTimeMilliseconds);
        }
        else if (ui64MTime == ui64LastMTime)
        {
          pcOutData[n++] = '#';
          pcOutData[n++] = '|';
          pcOutData[n++] = '#';
        }
        else
        {
          ulTempMTimeSeconds = (unsigned long) ((ui64LastMTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
          n += DevelopCompressHex(&pcOutData[n], ulMTimeSeconds, ulTempMTimeSeconds);
          n += sprintf(&pcOutData[n], "|%x", ulMTimeMilliseconds);
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Creation Time = ctime|cms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CTIME_SET) == CTIME_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->ftCreationTime.dwLowDateTime == 0 && psFTData->ftCreationTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
#ifndef WIN98

      /*-
       *****************************************************************
       *
       * Win 98 has many many files with Creation Time == 0 This ifdef
       * prevents a boat load of warning messages.
       *
       *****************************************************************
       */
      strcat(pcError, "ctime,");
      iStatus = ER_NullFields;
#endif

      /*-
       *****************************************************************
       *
       * Ensure that ui64LastCTime will be properly initialized.
       *
       *****************************************************************
       */
      ui64CTime = 0;
    }
    else
    {
      ui64CTime = (((unsigned __int64) psFTData->ftCreationTime.dwHighDateTime) << 32) | psFTData->ftCreationTime.dwLowDateTime;
      if (ui64CTime < UNIX_EPOCH_IN_NT_TIME || ui64CTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->ftCreationTime, cTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", cTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, "ctime,");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        ulCTimeSeconds = (unsigned long) ((ui64CTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulCTimeMilliseconds = (unsigned long) (((ui64CTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && ui64CTime == ui64ATime)
        {
          pcOutData[n++] = 'X';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'X';
        }
        else if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && ui64CTime == ui64MTime)
        {
          pcOutData[n++] = 'Y';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'Y';
        }
        else if (lRecoveryCounter == 0)
        {
          n += sprintf(&pcOutData[n], "%x|%x", ulCTimeSeconds, ulCTimeMilliseconds);
        }
        else if (ui64CTime == ui64LastCTime)
        {
          pcOutData[n++] = '#';
          pcOutData[n++] = '|';
          pcOutData[n++] = '#';
        }
        else
        {
          ulTempCTimeSeconds = (unsigned long) ((ui64LastCTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
          n += DevelopCompressHex(&pcOutData[n], ulCTimeSeconds, ulTempCTimeSeconds);
          n += sprintf(&pcOutData[n], "|%x", ulCTimeMilliseconds);
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Last Change Time = chtime|chms
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & CHTIME_SET) == CHTIME_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->ftChangeTime.dwLowDateTime == 0 && psFTData->ftChangeTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, "chtime,");
        iStatus = ER_NullFields;
      }
#endif

      /*-
       *****************************************************************
       *
       * Ensure that ui64LastChTime will be properly initialized.
       *
       *****************************************************************
       */
      ui64ChTime = 0;
    }
    else
    {
      ui64ChTime = (((unsigned __int64) psFTData->ftChangeTime.dwHighDateTime) << 32) | psFTData->ftChangeTime.dwLowDateTime;
      if (ui64ChTime < UNIX_EPOCH_IN_NT_TIME || ui64ChTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->ftChangeTime, cTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", cTime);
        }
        else
        {
          pcOutData[n++] = '|';
          if (psFTData->iFSType == FSTYPE_NTFS)
          {
            strcat(pcError, "chtime,");
            iStatus = ER_NullFields;
          }
        }
      }
      else
      {
        ulChTimeSeconds = (unsigned long) ((ui64ChTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulChTimeMilliseconds = (unsigned long) (((ui64ChTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && ui64ChTime == ui64ATime)
        {
          pcOutData[n++] = 'X';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'X';
        }
        else if ((psProperties->ulFieldMask & ATIME_SET) == ATIME_SET && ui64ChTime == ui64MTime)
        {
          pcOutData[n++] = 'Y';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'Y';
        }
        else if ((psProperties->ulFieldMask & CTIME_SET) == CTIME_SET && ui64ChTime == ui64CTime)
        {
          pcOutData[n++] = 'Z';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'Z';
        }
        else if (lRecoveryCounter == 0)
        {
          n += sprintf(&pcOutData[n], "%x|%x", ulChTimeSeconds, ulChTimeMilliseconds);
        }
        else if (ui64ChTime == ui64LastChTime)
        {
          pcOutData[n++] = '#';
          pcOutData[n++] = '|';
          pcOutData[n++] = '#';
        }
        else
        {
          ulTempChTimeSeconds = (unsigned long) ((ui64LastChTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
          n += DevelopCompressHex(&pcOutData[n], ulChTimeSeconds, ulTempChTimeSeconds);
          n += sprintf(&pcOutData[n], "|%x", ulChTimeMilliseconds);
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Size = size
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & SIZE_SET) == SIZE_SET)
  {
    pcOutData[n++] = '|';
    ui64FileSize = (((unsigned __int64) psFTData->nFileSizeHigh) << 32) | psFTData->nFileSizeLow;
    n += sprintf(&pcOutData[n], "%I64x", ui64FileSize);
  }

  /*-
   *********************************************************************
   *
   * Number of Alternate Streams = altstreams
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & ALTSTREAMS_SET) == ALTSTREAMS_SET)
  {
    pcOutData[n++] = '|';
    if (psFTData->iStreamCount != 0xffffffff)
    {
      n += sprintf(&pcOutData[n], "%x", psFTData->iStreamCount);
    }
    else
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, "altstreams,");
        iStatus = ER_NullFields;
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
        {
          for (i = 0, x = 0, ulLeft = 0; i < MD5_HASH_SIZE; i++)
          {
            x = (x << 8) | psFTData->ucFileMD5[i];
            ulLeft += 8;
            while (ulLeft > 6)
            {
              pcOutData[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
              ulLeft -= 6;
            }
          }
          if (ulLeft != 0)
          {
            pcOutData[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
          }
        }
        else
        {
          strcat(pcError, "md5");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'D';
      }
    }
    else
    {
      if (memcmp(psFTData->ucFileMD5, ucZeroHash, MD5_HASH_SIZE) != 0)
      {
        for (i = 0, x = 0, ulLeft = 0; i < MD5_HASH_SIZE; i++)
        {
          x = (x << 8) | psFTData->ucFileMD5[i];
          ulLeft += 8;
          while (ulLeft > 6)
          {
            pcOutData[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
            ulLeft -= 6;
          }
        }
        if (ulLeft != 0)
        {
          pcOutData[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
        }
      }
      else
      {
        strcat(pcError, "md5");
        iStatus = ER_NullFields;
      }
    }
  }

  n += sprintf(&pcOutData[n], "%s", psProperties->cNewLine);

  /*-
   *********************************************************************
   *
   * Save pertinent fields for next time around.
   *
   *********************************************************************
   */
  dwLastVolumeSerialNumber = psFTData->dwVolumeSerialNumber;
  dwLastFileIndexHigh = psFTData->nFileIndexHigh;
  dwLastFileIndexLow = psFTData->nFileIndexLow;
  dwLastFileAttributes = psFTData->dwFileAttributes;
  ui64LastATime = ui64ATime;
  ui64LastMTime = ui64MTime;
  ui64LastCTime = ui64CTime;
  ui64LastChTime = ui64ChTime;

  if (++lRecoveryCounter >= COMPRESS_RECOVERY_RATE)
  {
    lRecoveryCounter = 0;
  }

  /*-
   *********************************************************************
   *
   * Set the write count for the caller.
   *
   *********************************************************************
   */
  *iWriteCount = n;

  return iStatus;
}
#endif


/*-
 ***********************************************************************
 *
 * DevelopCompressHex
 *
 ***********************************************************************
 */
int
DevelopCompressHex(unsigned char *pcData, unsigned long ulHex, unsigned long ulOldHex)
{
  int                 iHexDigitCount,
                      iHexDigitDeltaCount;
  unsigned long       ulDelta;

  /*-
   *********************************************************************
   *
   * We normally print ulHex in hex, taking up to 8 hex digits; maybe
   * fewer because we don't print leading zeros. However, if ulHex is
   * 'close to' ulOldHex, it may take less space to print the difference.
   * We have to be careful, because simply subtracting ulHex - ulOldHex
   * may overflow a signed number.
   *
   *********************************************************************
   */
  iHexDigitCount = DevelopCountHexDigits(ulHex);

  if (ulHex >= ulOldHex)
  {
    ulDelta = ulHex - ulOldHex;
    iHexDigitDeltaCount = 1 + DevelopCountHexDigits(ulDelta);
    if (iHexDigitDeltaCount < iHexDigitCount)
    {
      return sprintf(pcData, "+%lx", ulDelta);
    }
  }
  else
  {
    ulDelta = ulOldHex - ulHex;
    iHexDigitDeltaCount = 1 + DevelopCountHexDigits(ulDelta);
    if (iHexDigitDeltaCount < iHexDigitCount)
    {
      return sprintf(pcData, "-%lx", ulDelta);
    }
  }

  return sprintf(pcData, "%lx", ulHex);
}


/*-
 ***********************************************************************
 *
 * DevelopCountHexDigits
 *
 ***********************************************************************
 */
int
DevelopCountHexDigits(unsigned long ulHex)
{
  int                 i = 8;

  if (ulHex == 0)
  {
    return 1;
  }
  while ((ulHex & 0xf0000000) == 0)
  {
    ulHex <<= 4;
    i--;
  }
  return i;
}
