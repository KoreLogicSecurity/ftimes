/*-
 ***********************************************************************
 *
 * $Id: develop.c,v 1.31 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
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

static unsigned char  gaucMd5ZeroHash[MD5_HASH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char  gaucSha1ZeroHash[SHA1_HASH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char  gaucSha256ZeroHash[SHA256_HASH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*-
 ***********************************************************************
 *
 * DevelopHaveNothingOutput
 *
 ***********************************************************************
 */
int
DevelopHaveNothingOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  int                 i = 0;
  int                 n = 0;
  int                 m = 0;
  int                 iMaskTableLength = MaskGetTableLength(MASK_RUNMODE_TYPE_MAP);
  MASK_B2S_TABLE     *psMaskTable = MaskGetTableReference(MASK_RUNMODE_TYPE_MAP);
  unsigned long       ul = 0;

  /*-
   *********************************************************************
   *
   * Loop over the mask table, and output a NULL value for each field
   * that is set in the mask. Then, update the write count and return.
   *
   *********************************************************************
   */
  for (i = 0; i < iMaskTableLength; i++)
  {
    ul = (1 << i);
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, ul))
    {
#ifdef WIN32
      switch (ul)
      {
      case MAP_ATIME:
      case MAP_MTIME:
      case MAP_CTIME:
      case MAP_CHTIME:
        n += sprintf(&pcOutData[n], "||");
        break;
      default:
        n += sprintf(&pcOutData[n], "|");
        break;
      }
#else
      n += sprintf(&pcOutData[n], "|");
#endif
      m += sprintf(&pcError[m], "%s%s", (i == 0) ? "" : ",", (char *) psMaskTable[i].acName);
    }
  }
  n += sprintf(&pcOutData[n], "%s", psProperties->acNewLine);
  *iWriteCount += n;

  return ER_NullFields;
}


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
  char                acTime[FTIMES_TIME_FORMAT_SIZE];
  int                 n;
  int                 iError;
  int                 iStatus = ER_OK;

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
   * If there are no attributes to develop, just generate a series of
   * NULL fields and return.
   *
   *********************************************************************
   */
  if (psFTData->iFileFlags == Have_Nothing)
  {
    *iWriteCount = n;
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTData, pcError);
  }

  /*-
   *********************************************************************
   *
   * Device = dev
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_DEV))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_dev);
  }

  /*-
   *********************************************************************
   *
   * Inode = inode
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_INODE))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_ino);
  }

  /*-
   *********************************************************************
   *
   * Permissions and Mode = mode
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MODE))
  {
    n += sprintf(&pcOutData[n], "|%o", (unsigned) psFTData->sStatEntry.st_mode);
  }

  /*-
   *********************************************************************
   *
   * Number of Links = nlink
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_NLINK))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_nlink);
  }

  /*-
   *********************************************************************
   *
   * User ID = uid
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_UID))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_uid);
  }

  /*-
   *********************************************************************
   *
   * Group ID = gid
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_GID))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_gid);
  }

  /*-
   *********************************************************************
   *
   * Special Device Type = rdev
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_RDEV))
  {
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTData->sStatEntry.st_rdev);
  }

  /*-
   *********************************************************************
   *
   * Last Access Time = atime
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME))
  {
    iError = TimeFormatTime(&psFTData->sStatEntry.st_atime, acTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", acTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, (pcError[0]) ? ",atime" : "atime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME))
  {
    iError = TimeFormatTime(&psFTData->sStatEntry.st_mtime, acTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", acTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CTIME))
  {
    iError = TimeFormatTime(&psFTData->sStatEntry.st_ctime, acTime);
    if (iError == ER_OK)
    {
      n += sprintf(&pcOutData[n], "|%s", acTime);
    }
    else
    {
      n += sprintf(&pcOutData[n], "|");
      strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SIZE))
  {
#ifdef USE_AP_SNPRINTF
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "|%qu", (unsigned long long) psFTData->sStatEntry.st_size);
#else
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "|%llu", (unsigned long long) psFTData->sStatEntry.st_size);
#endif
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTData->aucFileMd5, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTData->aucFileMd5, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",md5" : "md5");
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

  /*-
   *********************************************************************
   *
   * File SHA1 = sha1
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTData->aucFileSha1, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToHex(psFTData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTData->aucFileSha1, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
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

  /*-
   *********************************************************************
   *
   * File SHA256 = sha256
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTData->aucFileSha256, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToHex(psFTData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTData->aucFileSha256, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
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

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
#ifdef USE_XMAGIC
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
    if (psFTData->acType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTData->acType);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",magic" : "magic");
      iStatus = ER_NullFields;
    }
  }
#else
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
  }
#endif

  /*-
   *********************************************************************
   *
   * EOL
   *
   *********************************************************************
   */
  n += sprintf(&pcOutData[n], "%s", psProperties->acNewLine);

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
  char                acTime[FTIMES_TIME_FORMAT_SIZE];
  int                 iError;
  int                 n;
  int                 iStatus = ER_OK;
  unsigned __int64    ui64FileIndex;
  unsigned __int64    ui64FileSize;
  static DWORD        dwLastVolumeSerialNumber;
  static DWORD        dwLastFileIndexHigh;
  static DWORD        dwLastFileIndexLow;
  static DWORD        dwLastFileAttributes;
  static unsigned __int64 ui64LastATime;
  static unsigned __int64 ui64LastMTime;
  static unsigned __int64 ui64LastCTime;
  static unsigned __int64 ui64LastChTime;

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
   * If there are no attributes to develop, just generate a series of
   * NULL fields and return.
   *
   *********************************************************************
   */
  if (psFTData->iFileFlags == Have_Nothing)
  {
    *iWriteCount = n;
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTData, pcError);
  }

  /*-
   *********************************************************************
   *
   * Volume Number = volume
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_VOLUME))
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
        strcat(pcError, (pcError[0]) ? ",volume" : "volume");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_FINDEX))
  {
    pcOutData[n++] = '|';
    if (psFTData->dwFileIndexHigh != 0xffffffff && psFTData->dwFileIndexLow != 0xffffffff)
    {
      ui64FileIndex = (((unsigned __int64) psFTData->dwFileIndexHigh) << 32) | psFTData->dwFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64u", ui64FileIndex);
    }
    else
    {
#ifdef WIN98
      if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
      {
        strcat(pcError, (pcError[0]) ? ",findex" : "findex");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATTRIBUTES))
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME))
  {
    if (psFTData->sFTATime.dwLowDateTime == 0 && psFTData->sFTATime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, (pcError[0]) ? ",atime" : "atime");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->sFTATime, acTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", acTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, (pcError[0]) ? ",atime" : "atime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME))
  {
    if (psFTData->sFTMTime.dwLowDateTime == 0 && psFTData->sFTMTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->sFTMTime, acTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", acTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CTIME))
  {
    if (psFTData->sFTCTime.dwLowDateTime == 0 && psFTData->sFTCTime.dwHighDateTime == 0)
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
      strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
      iStatus = ER_NullFields;
#endif
      n += sprintf(&pcOutData[n], "||");
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->sFTCTime, acTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", acTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CHTIME))
  {
    if (psFTData->sFTChTime.dwLowDateTime == 0 && psFTData->sFTChTime.dwHighDateTime == 0)
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
        iStatus = ER_NullFields;
      }
#endif
      n += sprintf(&pcOutData[n], "||");
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTData->sFTChTime, acTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", acTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        if (psFTData->iFSType == FSTYPE_NTFS)
        {
          strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SIZE))
  {
    ui64FileSize = (((unsigned __int64) psFTData->dwFileSizeHigh) << 32) | psFTData->dwFileSizeLow;
    n += sprintf(&pcOutData[n], "|%I64u", ui64FileSize);
  }

  /*-
   *********************************************************************
   *
   * Number of Alternate Streams = altstreams
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ALTSTREAMS))
  {
    pcOutData[n++] = '|';
    if (psFTData->iStreamCount != FTIMES_INVALID_STREAM_COUNT)
    {
      n += sprintf(&pcOutData[n], "%u", psFTData->iStreamCount);
    }
    else
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",altstreams" : "altstreams");
        iStatus = ER_NullFields;
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTData->aucFileMd5, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA1 = sha1
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTData->aucFileSha1, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToHex(psFTData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA256 = sha256
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTData->aucFileSha256, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToHex(psFTData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
    if (psFTData->acType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTData->acType);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",magic" : "magic");
      iStatus = ER_NullFields;
    }
  }
#else
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
  }
#endif

  /*-
   *********************************************************************
   *
   * EOL
   *
   *********************************************************************
   */
  n += sprintf(&pcOutData[n], "%s", psProperties->acNewLine);

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
  int                 iStatus = ER_OK;
  static char         acLastName[4 * FTIMES_MAX_PATH]; /* This is an encoded name. */
  static long         lRecoveryCounter = 0;
  static struct stat  sStatLastEntry;

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
    strncpy(acLastName, psFTData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
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
    if (acLastName[0] == '\0')
    {
      n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (acLastName[i] != '\0') &&
             (psFTData->pcNeuteredPath[i] != '\0') &&
             (acLastName[i] == psFTData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTData->pcNeuteredPath[i]);
    }
    strncpy(&acLastName[i], &psFTData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
  }

  /*-
   *********************************************************************
   *
   * If there are no attributes to develop, reset the recovery counter,
   * zero out state variables, generate a series of NULL fields, and
   * return.
   *
   *********************************************************************
   */
  if (psFTData->iFileFlags == Have_Nothing)
  {
    *iWriteCount = n;
    lRecoveryCounter = 0;
    memset(acLastName, 0, (4 * FTIMES_MAX_PATH));
    memset(&sStatLastEntry, 0, sizeof(struct stat));
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTData, pcError);
  }

  /*-
   *********************************************************************
   *
   * Device = dev
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_DEV))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_dev);
    }
    else
    {
      if (psFTData->sStatEntry.st_dev == sStatLastEntry.st_dev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_dev);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_INODE))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_ino);
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTData->sStatEntry.st_ino, sStatLastEntry.st_ino);
    }
  }

  /*-
   *********************************************************************
   *
   * Permissions and Mode = mode
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MODE))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_mode);
    }
    else
    {
      if (psFTData->sStatEntry.st_mode == sStatLastEntry.st_mode)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_mode);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_NLINK))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_nlink);
    }
    else
    {
      if (psFTData->sStatEntry.st_nlink == sStatLastEntry.st_nlink)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_nlink);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_UID))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_uid);
    }
    else
    {
      if (psFTData->sStatEntry.st_uid == sStatLastEntry.st_uid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_uid);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_GID))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_gid);
    }
    else
    {
      if (psFTData->sStatEntry.st_gid == sStatLastEntry.st_gid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_gid);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_RDEV))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_rdev);
    }
    else
    {
      if (psFTData->sStatEntry.st_rdev == sStatLastEntry.st_rdev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_rdev);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_atime);
    }
    else if (psFTData->sStatEntry.st_atime == sStatLastEntry.st_atime)
    {
      pcOutData[n++] = '#';
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTData->sStatEntry.st_atime, sStatLastEntry.st_atime);
    }
  }

  /*-
   *********************************************************************
   *
   * Last Modification Time = mtime
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME))
  {
    pcOutData[n++] = '|';
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && psFTData->sStatEntry.st_mtime == psFTData->sStatEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_mtime);
    }
    else
    {
      if (psFTData->sStatEntry.st_mtime == sStatLastEntry.st_mtime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTData->sStatEntry.st_mtime, sStatLastEntry.st_mtime);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CTIME))
  {
    pcOutData[n++] = '|';
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && psFTData->sStatEntry.st_ctime == psFTData->sStatEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME) && psFTData->sStatEntry.st_ctime == psFTData->sStatEntry.st_mtime)
    {
      pcOutData[n++] = 'Y';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTData->sStatEntry.st_ctime);
    }
    else
    {
      if (psFTData->sStatEntry.st_ctime == sStatLastEntry.st_ctime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTData->sStatEntry.st_ctime, sStatLastEntry.st_ctime);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SIZE))
  {
    pcOutData[n++] = '|';
#ifdef USE_AP_SNPRINTF
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "%qx", (unsigned long long) psFTData->sStatEntry.st_size);
#else
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "%llx", (unsigned long long) psFTData->sStatEntry.st_size);
#endif
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTData->aucFileMd5, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",md5" : "md5");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'D';
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTData->aucFileMd5, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",md5" : "md5");
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

  /*-
   *********************************************************************
   *
   * File SHA1 = sha1
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTData->aucFileSha1, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'D';
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToBase64(psFTData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTData->aucFileSha1, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
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

  /*-
   *********************************************************************
   *
   * File SHA256 = sha256
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTData->aucFileSha256, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        pcOutData[n++] = 'D';
      }
    }
    else if (S_ISREG(psFTData->sStatEntry.st_mode))
    {
      if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToBase64(psFTData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTData->aucFileSha256, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
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

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * EOL
   *
   *********************************************************************
   */
  n += sprintf(&pcOutData[n], "%s", psProperties->acNewLine);

  /*-
   *********************************************************************
   *
   * Copy psFTData->sStatEntry to sStatLastEntry for next time around.
   *
   *********************************************************************
   */
  memcpy(&sStatLastEntry, &psFTData->sStatEntry, sizeof(struct stat));

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
  char                acTime[FTIMES_TIME_FORMAT_SIZE];
  int                 i;
  int                 n;
  int                 iError;
  int                 iStatus = ER_OK;
  unsigned long       ulATimeSeconds;
  unsigned long       ulMTimeSeconds;
  unsigned long       ulCTimeSeconds;
  unsigned long       ulChTimeSeconds;
  unsigned long       ulATimeMilliseconds;
  unsigned long       ulMTimeMilliseconds;
  unsigned long       ulCTimeMilliseconds;
  unsigned long       ulChTimeMilliseconds;
  unsigned long       ulTempATimeSeconds;
  unsigned long       ulTempMTimeSeconds;
  unsigned long       ulTempCTimeSeconds;
  unsigned long       ulTempChTimeSeconds;
  unsigned __int64    ui64ATime = 0;
  unsigned __int64    ui64MTime = 0;
  unsigned __int64    ui64CTime = 0;
  unsigned __int64    ui64ChTime = 0;
  unsigned __int64    ui64FileIndex = 0;
  unsigned __int64    ui64FileSize = 0;
  static char         acLastName[4 * FTIMES_MAX_PATH]; /* This is an encoded name. */
  static DWORD        dwLastVolumeSerialNumber;
  static DWORD        dwLastFileIndexHigh;
  static DWORD        dwLastFileIndexLow;
  static DWORD        dwLastFileAttributes;
  static long         lRecoveryCounter = 0;
  static unsigned __int64 ui64LastATime;
  static unsigned __int64 ui64LastMTime;
  static unsigned __int64 ui64LastCTime;
  static unsigned __int64 ui64LastChTime;

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
    strncpy(acLastName, psFTData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
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
    if (acLastName[0] == '\0')
    {
      n = sprintf(pcOutData, "00\"%s\"", psFTData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (acLastName[i] != '\0') &&
             (psFTData->pcNeuteredPath[i] != '\0') &&
             (acLastName[i] == psFTData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTData->pcNeuteredPath[i]);
    }
    strncpy(&acLastName[i], &psFTData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
  }

  /*-
   *********************************************************************
   *
   * If there are no attributes to develop, reset the recovery counter,
   * zero out state variables, generate a series of NULL fields, and
   * return.
   *
   *********************************************************************
   */
  if (psFTData->iFileFlags == Have_Nothing)
  {
    *iWriteCount = n;
    lRecoveryCounter = 0;
    memset(acLastName, 0, (4 * FTIMES_MAX_PATH));
    dwLastVolumeSerialNumber = 0;
    dwLastFileIndexHigh = 0;
    dwLastFileIndexLow = 0;
    dwLastFileAttributes = 0;
    ui64LastATime = 0;
    ui64LastMTime = 0;
    ui64LastCTime = 0;
    ui64LastChTime = 0;
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTData, pcError);
  }

  /*-
   *********************************************************************
   *
   * Volume Number = volume
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_VOLUME))
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
          strcat(pcError, (pcError[0]) ? ",volume" : "volume");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_FINDEX))
  {
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      ui64FileIndex = (((unsigned __int64) psFTData->dwFileIndexHigh) << 32) | psFTData->dwFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64x", ui64FileIndex);
    }
    else
    {
      if (psFTData->dwFileIndexHigh != 0xffffffff && psFTData->dwFileIndexLow != 0xffffffff)
      {
        if (psFTData->dwFileIndexHigh == dwLastFileIndexHigh)
        {
          pcOutData[n++] = '#';
          n += DevelopCompressHex(&pcOutData[n], psFTData->dwFileIndexLow, dwLastFileIndexLow);
        }
        else
        {
          ui64FileIndex = (((unsigned __int64) psFTData->dwFileIndexHigh) << 32) | psFTData->dwFileIndexLow;
          n += sprintf(&pcOutData[n], "%I64x", ui64FileIndex);
        }
      }
      else
      {
#ifdef WIN98
        if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
#endif
        {
          strcat(pcError, (pcError[0]) ? ",findex" : "findex");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATTRIBUTES))
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME))
  {
    pcOutData[n++] = '|';
    if (psFTData->sFTATime.dwLowDateTime == 0 && psFTData->sFTATime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, (pcError[0]) ? ",atime" : "atime");
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
      ui64ATime = (((unsigned __int64) psFTData->sFTATime.dwHighDateTime) << 32) | psFTData->sFTATime.dwLowDateTime;
      if (ui64ATime < UNIX_EPOCH_IN_NT_TIME || ui64ATime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->sFTATime, acTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", acTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, (pcError[0]) ? ",atime" : "atime");
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME))
  {
    pcOutData[n++] = '|';
    if (psFTData->sFTMTime.dwLowDateTime == 0 && psFTData->sFTMTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
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
      ui64MTime = (((unsigned __int64) psFTData->sFTMTime.dwHighDateTime) << 32) | psFTData->sFTMTime.dwLowDateTime;
      if (ui64MTime < UNIX_EPOCH_IN_NT_TIME || ui64MTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->sFTMTime, acTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", acTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        ulMTimeSeconds = (unsigned long) ((ui64MTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulMTimeMilliseconds = (unsigned long) (((ui64MTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && ui64MTime == ui64ATime)
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CTIME))
  {
    pcOutData[n++] = '|';
    if (psFTData->sFTCTime.dwLowDateTime == 0 && psFTData->sFTCTime.dwHighDateTime == 0)
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
      strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
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
      ui64CTime = (((unsigned __int64) psFTData->sFTCTime.dwHighDateTime) << 32) | psFTData->sFTCTime.dwLowDateTime;
      if (ui64CTime < UNIX_EPOCH_IN_NT_TIME || ui64CTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->sFTCTime, acTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", acTime);
        }
        else
        {
          pcOutData[n++] = '|';
          strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
          iStatus = ER_NullFields;
        }
      }
      else
      {
        ulCTimeSeconds = (unsigned long) ((ui64CTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulCTimeMilliseconds = (unsigned long) (((ui64CTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && ui64CTime == ui64ATime)
        {
          pcOutData[n++] = 'X';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'X';
        }
        else if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME) && ui64CTime == ui64MTime)
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CHTIME))
  {
    pcOutData[n++] = '|';
    if (psFTData->sFTChTime.dwLowDateTime == 0 && psFTData->sFTChTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
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
      ui64ChTime = (((unsigned __int64) psFTData->sFTChTime.dwHighDateTime) << 32) | psFTData->sFTChTime.dwLowDateTime;
      if (ui64ChTime < UNIX_EPOCH_IN_NT_TIME || ui64ChTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTData->sFTChTime, acTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", acTime);
        }
        else
        {
          pcOutData[n++] = '|';
          if (psFTData->iFSType == FSTYPE_NTFS)
          {
            strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
            iStatus = ER_NullFields;
          }
        }
      }
      else
      {
        ulChTimeSeconds = (unsigned long) ((ui64ChTime - UNIX_EPOCH_IN_NT_TIME) / 10000000);
        ulChTimeMilliseconds = (unsigned long) (((ui64ChTime - UNIX_EPOCH_IN_NT_TIME) % 10000000) / 10000);
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && ui64ChTime == ui64ATime)
        {
          pcOutData[n++] = 'X';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'X';
        }
        else if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME) && ui64ChTime == ui64MTime)
        {
          pcOutData[n++] = 'Y';
          pcOutData[n++] = '|';
          pcOutData[n++] = 'Y';
        }
        else if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_CTIME) && ui64ChTime == ui64CTime)
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SIZE))
  {
    pcOutData[n++] = '|';
    ui64FileSize = (((unsigned __int64) psFTData->dwFileSizeHigh) << 32) | psFTData->dwFileSizeLow;
    n += sprintf(&pcOutData[n], "%I64x", ui64FileSize);
  }

  /*-
   *********************************************************************
   *
   * Number of Alternate Streams = altstreams
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ALTSTREAMS))
  {
    pcOutData[n++] = '|';
    if (psFTData->iStreamCount != FTIMES_INVALID_STREAM_COUNT)
    {
      n += sprintf(&pcOutData[n], "%x", psFTData->iStreamCount);
    }
    else
    {
#ifndef WIN98
      if (psFTData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",altstreams" : "altstreams");
        iStatus = ER_NullFields;
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTData->aucFileMd5, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",md5" : "md5");
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
      if (memcmp(psFTData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA1 = sha1
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTData->aucFileSha1, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
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
      if (memcmp(psFTData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToBase64(psFTData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA256 = sha256
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    pcOutData[n++] = '|';
    if ((psFTData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTData->aucFileSha256, &pcOutData[n]);
        }
        else
        {
          strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
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
      if (memcmp(psFTData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToBase64(psFTData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
  }

  /*-
   *********************************************************************
   *
   * File Magic = magic
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * EOL
   *
   *********************************************************************
   */
  n += sprintf(&pcOutData[n], "%s", psProperties->acNewLine);

  /*-
   *********************************************************************
   *
   * Save pertinent fields for next time around.
   *
   *********************************************************************
   */
  dwLastVolumeSerialNumber = psFTData->dwVolumeSerialNumber;
  dwLastFileIndexHigh = psFTData->dwFileIndexHigh;
  dwLastFileIndexLow = psFTData->dwFileIndexLow;
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
  int                 iHexDigitCount;
  int                 iHexDigitDeltaCount;
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
