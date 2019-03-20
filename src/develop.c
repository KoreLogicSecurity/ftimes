/*-
 ***********************************************************************
 *
 * $Id: develop.c,v 1.52 2014/07/30 07:24:15 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2014 The FTimes Project, All Rights Reserved.
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
DevelopHaveNothingOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
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
DevelopNoOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
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
DevelopNormalOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
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
  n = sprintf(pcOutData, "\"%s\"", psFTFileData->pcNeuteredPath);

  /*-
   *********************************************************************
   *
   * If there are no attributes to develop, just generate a series of
   * NULL fields and return.
   *
   *********************************************************************
   */
  if (psFTFileData->ulAttributeMask == 0)
  {
    *iWriteCount = n;
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTFileData, pcError);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_dev);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_ino);
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
    n += sprintf(&pcOutData[n], "|%o", (unsigned) psFTFileData->sStatEntry.st_mode);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_nlink);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_uid);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_gid);
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned) psFTFileData->sStatEntry.st_rdev);
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
    iError = TimeFormatTime(&psFTFileData->sStatEntry.st_atime, acTime);
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
    iError = TimeFormatTime(&psFTFileData->sStatEntry.st_mtime, acTime);
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
    iError = TimeFormatTime(&psFTFileData->sStatEntry.st_ctime, acTime);
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
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "|%qu", (unsigned long long) psFTFileData->sStatEntry.st_size);
#else
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "|%llu", (unsigned long long) psFTFileData->sStatEntry.st_size);
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        n += sprintf(&pcOutData[n], "SPECIAL");
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its MD5 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterMd5List && SupportMatchFilter(psProperties->psExcludeFilterMd5List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterMd5List && SupportMatchFilter(psProperties->psIncludeFilterMd5List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
#endif
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        n += sprintf(&pcOutData[n], "SPECIAL");
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its SHA1 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterSha1List && SupportMatchFilter(psProperties->psExcludeFilterSha1List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterSha1List && SupportMatchFilter(psProperties->psIncludeFilterSha1List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
#endif
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        n += sprintf(&pcOutData[n], "SPECIAL");
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its SHA256 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterSha256List && SupportMatchFilter(psProperties->psExcludeFilterSha256List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterSha256List && SupportMatchFilter(psProperties->psIncludeFilterSha256List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
    if (psFTFileData->acType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTFileData->acType);
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
DevelopNormalOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  char                acTime[FTIMES_TIME_FORMAT_SIZE];
  int                 iError;
  int                 n;
  int                 iStatus = ER_OK;
  unsigned __int64    ui64FileIndex;
  unsigned __int64    ui64FileSize;

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
  n = sprintf(pcOutData, "\"%s\"", psFTFileData->pcNeuteredPath);

  /*-
   *********************************************************************
   *
   * If there are no attributes to develop, just generate a series of
   * NULL fields and return.
   *
   *********************************************************************
   */
  if (psFTFileData->ulAttributeMask == 0)
  {
    *iWriteCount = n;
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTFileData, pcError);
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
    if (psFTFileData->dwVolumeSerialNumber != 0xffffffff)
    {
      n += sprintf(&pcOutData[n], "%u", (unsigned int) psFTFileData->dwVolumeSerialNumber);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",volume" : "volume");
      iStatus = ER_NullFields;
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
    if (psFTFileData->dwFileIndexHigh != 0xffffffff && psFTFileData->dwFileIndexLow != 0xffffffff)
    {
      ui64FileIndex = (((unsigned __int64) psFTFileData->dwFileIndexHigh) << 32) | psFTFileData->dwFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64u", (APP_UI64) ui64FileIndex);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",findex" : "findex");
      iStatus = ER_NullFields;
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
    n += sprintf(&pcOutData[n], "|%u", (unsigned int) psFTFileData->dwFileAttributes);
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
    if (psFTFileData->sFTATime.dwLowDateTime == 0 && psFTFileData->sFTATime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, (pcError[0]) ? ",atime" : "atime");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTFileData->sFTATime, acTime);
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
    if (psFTFileData->sFTMTime.dwLowDateTime == 0 && psFTFileData->sFTMTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTFileData->sFTMTime, acTime);
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
    if (psFTFileData->sFTCTime.dwLowDateTime == 0 && psFTFileData->sFTCTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
      iStatus = ER_NullFields;
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTFileData->sFTCTime, acTime);
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
    if (psFTFileData->sFTChTime.dwLowDateTime == 0 && psFTFileData->sFTChTime.dwHighDateTime == 0)
    {
      n += sprintf(&pcOutData[n], "||");
      if (psFTFileData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
        iStatus = ER_NullFields;
      }
    }
    else
    {
      iError = TimeFormatTime((FILETIME *) &psFTFileData->sFTChTime, acTime);
      if (iError == ER_OK)
      {
        n += sprintf(&pcOutData[n], "|%s", acTime);
      }
      else
      {
        n += sprintf(&pcOutData[n], "||");
        if (psFTFileData->iFSType == FSTYPE_NTFS)
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
    ui64FileSize = (((unsigned __int64) psFTFileData->dwFileSizeHigh) << 32) | psFTFileData->dwFileSizeLow;
    n += sprintf(&pcOutData[n], "|%I64u", (APP_UI64) ui64FileSize);
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
    if (psFTFileData->iStreamCount != FTIMES_INVALID_STREAM_COUNT)
    {
      n += sprintf(&pcOutData[n], "%u", psFTFileData->iStreamCount);
    }
    else
    {
      if (psFTFileData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",altstreams" : "altstreams");
        iStatus = ER_NullFields;
      }
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToHex(psFTFileData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its MD5 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterMd5List && SupportMatchFilter(psProperties->psExcludeFilterMd5List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterMd5List && SupportMatchFilter(psProperties->psIncludeFilterMd5List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
#endif
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToHex(psFTFileData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its SHA1 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterSha1List && SupportMatchFilter(psProperties->psExcludeFilterSha1List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterSha1List && SupportMatchFilter(psProperties->psIncludeFilterSha1List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
#endif
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
#ifdef USE_PCRE
    int iFilterIndex = n + 1;
#endif
    pcOutData[n++] = '|';
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
        }
      }
      else
      {
        n += sprintf(&pcOutData[n], "DIRECTORY");
      }
    }
    else
    {
      if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToHex(psFTFileData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * Conditionally filter this record based on its SHA256 value.
     *
     *******************************************************************
     */
    if
    (
      (psProperties->psExcludeFilterSha256List && SupportMatchFilter(psProperties->psExcludeFilterSha256List, &pcOutData[iFilterIndex]) != NULL) ||
      (psProperties->psIncludeFilterSha256List && SupportMatchFilter(psProperties->psIncludeFilterSha256List, &pcOutData[iFilterIndex]) == NULL)
    )
    {
      return ER_Filtered;
    }
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    pcOutData[n++] = '|';
    if (psFTFileData->acType[0])
    {
      n += sprintf(&pcOutData[n], "%s", psFTFileData->acType);
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
   * Owner SID = osid
   *
   *********************************************************************
   */
#ifdef USE_SDDL
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_OWNER))
  {
    char *pcSidOwner = NULL;
    pcOutData[n++] = '|';
    ConvertSidToStringSidA(psFTFileData->psSidOwner, &pcSidOwner);
    if (pcSidOwner)
    {
      n += sprintf(&pcOutData[n], "%s", pcSidOwner);
      LocalFree(pcSidOwner);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",osid" : "osid");
      iStatus = ER_NullFields;
    }
  }
#else
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_OWNER))
  {
    pcOutData[n++] = '|';
  }
#endif

  /*-
   *********************************************************************
   *
   * Group SID = gsid
   *
   *********************************************************************
   */
#ifdef USE_SDDL
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_GROUP))
  {
    char *pcSidGroup = NULL;
    pcOutData[n++] = '|';
    ConvertSidToStringSidA(psFTFileData->psSidGroup, &pcSidGroup);
    if (pcSidGroup)
    {
      n += sprintf(&pcOutData[n], "%s", pcSidGroup);
      LocalFree(pcSidGroup);
    }
    else
    {
      strcat(pcError, (pcError[0]) ? ",gsid" : "gsid");
      iStatus = ER_NullFields;
    }
  }
#else
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_GROUP))
  {
    pcOutData[n++] = '|';
  }
#endif

  /*-
   *********************************************************************
   *
   * DACL = dacl
   *
   *********************************************************************
   */
#ifdef USE_SDDL
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_DACL))
  {
    char *pcAclDacl = NULL;
    DWORD dwLength = 0;
    pcOutData[n++] = '|';
    ConvertSecurityDescriptorToStringSecurityDescriptorA(psFTFileData->psSd, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &pcAclDacl, &dwLength);
    if (pcAclDacl && dwLength < FTIMES_MAX_ACL_SIZE)
    {
      n += sprintf(&pcOutData[n], "%s", pcAclDacl);
      LocalFree(pcAclDacl);
    }
    else
    {
      if (dwLength >= FTIMES_MAX_ACL_SIZE)
      {
        snprintf(pcError, MESSAGE_SIZE, "DevelopNormalOutput(): NeuteredPath = [%s], DaclLength = [%d]: Length exceeds %d bytes.", psFTFileData->pcNeuteredPath, (int) dwLength, FTIMES_MAX_ACL_SIZE - 1);
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
        pcError[0] = 0;
      }
      strcat(pcError, (pcError[0]) ? ",dacl" : "dacl");
      iStatus = ER_NullFields;
    }
  }
#else
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_DACL))
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
DevelopCompressedOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
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
    n = sprintf(pcOutData, "00\"%s\"", psFTFileData->pcNeuteredPath);
    strncpy(acLastName, psFTFileData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
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
      n = sprintf(pcOutData, "00\"%s\"", psFTFileData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (acLastName[i] != '\0') &&
             (psFTFileData->pcNeuteredPath[i] != '\0') &&
             (acLastName[i] == psFTFileData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTFileData->pcNeuteredPath[i]);
    }
    strncpy(&acLastName[i], &psFTFileData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
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
  if (psFTFileData->ulAttributeMask == 0)
  {
    *iWriteCount = n;
    lRecoveryCounter = 0;
    memset(acLastName, 0, (4 * FTIMES_MAX_PATH));
    memset(&sStatLastEntry, 0, sizeof(struct stat));
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTFileData, pcError);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_dev);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_dev == sStatLastEntry.st_dev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_dev);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_ino);
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTFileData->sStatEntry.st_ino, sStatLastEntry.st_ino);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_mode);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_mode == sStatLastEntry.st_mode)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_mode);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_nlink);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_nlink == sStatLastEntry.st_nlink)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_nlink);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_uid);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_uid == sStatLastEntry.st_uid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_uid);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_gid);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_gid == sStatLastEntry.st_gid)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_gid);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_rdev);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_rdev == sStatLastEntry.st_rdev)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_rdev);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_atime);
    }
    else if (psFTFileData->sStatEntry.st_atime == sStatLastEntry.st_atime)
    {
      pcOutData[n++] = '#';
    }
    else
    {
      n += DevelopCompressHex(&pcOutData[n], psFTFileData->sStatEntry.st_atime, sStatLastEntry.st_atime);
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
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && psFTFileData->sStatEntry.st_mtime == psFTFileData->sStatEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_mtime);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_mtime == sStatLastEntry.st_mtime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTFileData->sStatEntry.st_mtime, sStatLastEntry.st_mtime);
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
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_ATIME) && psFTFileData->sStatEntry.st_ctime == psFTFileData->sStatEntry.st_atime)
    {
      pcOutData[n++] = 'X';
    }
    else if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MTIME) && psFTFileData->sStatEntry.st_ctime == psFTFileData->sStatEntry.st_mtime)
    {
      pcOutData[n++] = 'Y';
    }
    else if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned) psFTFileData->sStatEntry.st_ctime);
    }
    else
    {
      if (psFTFileData->sStatEntry.st_ctime == sStatLastEntry.st_ctime)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += DevelopCompressHex(&pcOutData[n], psFTFileData->sStatEntry.st_ctime, sStatLastEntry.st_ctime);
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
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "%qx", (unsigned long long) psFTFileData->sStatEntry.st_size);
#else
    n += snprintf(&pcOutData[n], FTIMES_MAX_64BIT_SIZE, "%llx", (unsigned long long) psFTFileData->sStatEntry.st_size);
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
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
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
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",md5" : "md5");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
      }
      else
      {
        pcOutData[n++] = 'S';
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
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
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
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha1" : "sha1");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
      }
      else
      {
        pcOutData[n++] = 'S';
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
    if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
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
    else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
    {
      if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        strcat(pcError, (pcError[0]) ? ",sha256" : "sha256");
        iStatus = ER_NullFields;
      }
    }
    else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
    {
      if (psProperties->bHashSymbolicLinks)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
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
      if (psProperties->bAnalyzeDeviceFiles && memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
      }
      else
      {
        pcOutData[n++] = 'S';
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
   * Copy psFTFileData->sStatEntry to sStatLastEntry for next time around.
   *
   *********************************************************************
   */
  memcpy(&sStatLastEntry, &psFTFileData->sStatEntry, sizeof(struct stat));

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
DevelopCompressedOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError)
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
    n = sprintf(pcOutData, "00\"%s\"", psFTFileData->pcNeuteredPath);
    strncpy(acLastName, psFTFileData->pcNeuteredPath, (4 * FTIMES_MAX_PATH));
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
      n = sprintf(pcOutData, "00\"%s\"", psFTFileData->pcNeuteredPath);
    }
    else
    {
      while ((i < 254) &&
             (acLastName[i] != '\0') &&
             (psFTFileData->pcNeuteredPath[i] != '\0') &&
             (acLastName[i] == psFTFileData->pcNeuteredPath[i]))
      {
        i++;
      }
      n = sprintf(pcOutData, "%02x%s\"", i + 1 /* Add 1 for the leading quote. */, &psFTFileData->pcNeuteredPath[i]);
    }
    strncpy(&acLastName[i], &psFTFileData->pcNeuteredPath[i], ((4 * FTIMES_MAX_PATH) - i) /* Must subtract i here to prevent overruns. */);
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
  if (psFTFileData->ulAttributeMask == 0)
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
    return DevelopHaveNothingOutput(psProperties, &pcOutData[n], iWriteCount, psFTFileData, pcError);
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
      n += sprintf(&pcOutData[n], "%x", (unsigned int) psFTFileData->dwVolumeSerialNumber);
    }
    else
    {
      if (psFTFileData->dwVolumeSerialNumber != 0xffffffff)
      {
        if (psFTFileData->dwVolumeSerialNumber == dwLastVolumeSerialNumber)
        {
          pcOutData[n++] = '#';
        }
        else
        {
          n += sprintf(&pcOutData[n], "%x", (unsigned int) psFTFileData->dwVolumeSerialNumber);
        }
      }
      else
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
    if (lRecoveryCounter == 0)
    {
      ui64FileIndex = (((unsigned __int64) psFTFileData->dwFileIndexHigh) << 32) | psFTFileData->dwFileIndexLow;
      n += sprintf(&pcOutData[n], "%I64x", (APP_UI64) ui64FileIndex);
    }
    else
    {
      if (psFTFileData->dwFileIndexHigh != 0xffffffff && psFTFileData->dwFileIndexLow != 0xffffffff)
      {
        if (psFTFileData->dwFileIndexHigh == dwLastFileIndexHigh)
        {
          pcOutData[n++] = '#';
          n += DevelopCompressHex(&pcOutData[n], psFTFileData->dwFileIndexLow, dwLastFileIndexLow);
        }
        else
        {
          ui64FileIndex = (((unsigned __int64) psFTFileData->dwFileIndexHigh) << 32) | psFTFileData->dwFileIndexLow;
          n += sprintf(&pcOutData[n], "%I64x", (APP_UI64) ui64FileIndex);
        }
      }
      else
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
    pcOutData[n++] = '|';
    if (lRecoveryCounter == 0)
    {
      n += sprintf(&pcOutData[n], "%x", (unsigned int) psFTFileData->dwFileAttributes);
    }
    else
    {
      if (psFTFileData->dwFileAttributes == dwLastFileAttributes)
      {
        pcOutData[n++] = '#';
      }
      else
      {
        n += sprintf(&pcOutData[n], "%x", (unsigned int) psFTFileData->dwFileAttributes);
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
    if (psFTFileData->sFTATime.dwLowDateTime == 0 && psFTFileData->sFTATime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, (pcError[0]) ? ",atime" : "atime");
      iStatus = ER_NullFields;
      ui64ATime = 0; /* Ensure that ui64LastATime will be properly initialized. */
    }
    else
    {
      ui64ATime = (((unsigned __int64) psFTFileData->sFTATime.dwHighDateTime) << 32) | psFTFileData->sFTATime.dwLowDateTime;
      if (ui64ATime < UNIX_EPOCH_IN_NT_TIME || ui64ATime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTFileData->sFTATime, acTime);
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
          n += sprintf(&pcOutData[n], "%lx|%lx", ulATimeSeconds, ulATimeMilliseconds);
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
            n += sprintf(&pcOutData[n], "|%lx", ulATimeMilliseconds);
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
    if (psFTFileData->sFTMTime.dwLowDateTime == 0 && psFTFileData->sFTMTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, (pcError[0]) ? ",mtime" : "mtime");
      iStatus = ER_NullFields;
      ui64MTime = 0; /* Ensure that ui64LastMTime will be properly initialized. */
    }
    else
    {
      ui64MTime = (((unsigned __int64) psFTFileData->sFTMTime.dwHighDateTime) << 32) | psFTFileData->sFTMTime.dwLowDateTime;
      if (ui64MTime < UNIX_EPOCH_IN_NT_TIME || ui64MTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTFileData->sFTMTime, acTime);
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
          n += sprintf(&pcOutData[n], "%lx|%lx", ulMTimeSeconds, ulMTimeMilliseconds);
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
          n += sprintf(&pcOutData[n], "|%lx", ulMTimeMilliseconds);
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
    if (psFTFileData->sFTCTime.dwLowDateTime == 0 && psFTFileData->sFTCTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      strcat(pcError, (pcError[0]) ? ",ctime" : "ctime");
      iStatus = ER_NullFields;
      ui64CTime = 0; /* Ensure that ui64LastCTime will be properly initialized. */
    }
    else
    {
      ui64CTime = (((unsigned __int64) psFTFileData->sFTCTime.dwHighDateTime) << 32) | psFTFileData->sFTCTime.dwLowDateTime;
      if (ui64CTime < UNIX_EPOCH_IN_NT_TIME || ui64CTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTFileData->sFTCTime, acTime);
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
          n += sprintf(&pcOutData[n], "%lx|%lx", ulCTimeSeconds, ulCTimeMilliseconds);
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
          n += sprintf(&pcOutData[n], "|%lx", ulCTimeMilliseconds);
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
    if (psFTFileData->sFTChTime.dwLowDateTime == 0 && psFTFileData->sFTChTime.dwHighDateTime == 0)
    {
      pcOutData[n++] = '|';
      if (psFTFileData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",chtime" : "chtime");
        iStatus = ER_NullFields;
      }
      ui64ChTime = 0; /* Ensure that ui64LastChTime will be properly initialized. */
    }
    else
    {
      ui64ChTime = (((unsigned __int64) psFTFileData->sFTChTime.dwHighDateTime) << 32) | psFTFileData->sFTChTime.dwLowDateTime;
      if (ui64ChTime < UNIX_EPOCH_IN_NT_TIME || ui64ChTime > UNIX_LIMIT_IN_NT_TIME)
      {
        iError = TimeFormatOutOfBandTime((FILETIME *) &psFTFileData->sFTChTime, acTime);
        if (iError == ER_OK)
        {
          n += sprintf(&pcOutData[n], "~%s", acTime);
        }
        else
        {
          pcOutData[n++] = '|';
          if (psFTFileData->iFSType == FSTYPE_NTFS)
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
          n += sprintf(&pcOutData[n], "%lx|%lx", ulChTimeSeconds, ulChTimeMilliseconds);
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
          n += sprintf(&pcOutData[n], "|%lx", ulChTimeMilliseconds);
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
    ui64FileSize = (((unsigned __int64) psFTFileData->dwFileSizeHigh) << 32) | psFTFileData->dwFileSizeLow;
    n += sprintf(&pcOutData[n], "%I64x", (APP_UI64) ui64FileSize);
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
    if (psFTFileData->iStreamCount != FTIMES_INVALID_STREAM_COUNT)
    {
      n += sprintf(&pcOutData[n], "%x", psFTFileData->iStreamCount);
    }
    else
    {
      if (psFTFileData->iFSType == FSTYPE_NTFS)
      {
        strcat(pcError, (pcError[0]) ? ",altstreams" : "altstreams");
        iStatus = ER_NullFields;
      }
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
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
        {
          n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
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
      if (memcmp(psFTFileData->aucFileMd5, gaucMd5ZeroHash, MD5_HASH_SIZE) != 0)
      {
        n += MD5HashToBase64(psFTFileData->aucFileMd5, &pcOutData[n]);
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
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
        {
          n += SHA1HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
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
      if (memcmp(psFTFileData->aucFileSha1, gaucSha1ZeroHash, SHA1_HASH_SIZE) != 0)
      {
        n += SHA1HashToBase64(psFTFileData->aucFileSha1, &pcOutData[n]);
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
    if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
    {
      if (psProperties->bHashDirectories)
      {
        if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
        {
          n += SHA256HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
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
      if (memcmp(psFTFileData->aucFileSha256, gaucSha256ZeroHash, SHA256_HASH_SIZE) != 0)
      {
        n += SHA256HashToBase64(psFTFileData->aucFileSha256, &pcOutData[n]);
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
   * Owner SID = osid (Compression for this field is not yet supported.)
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_OWNER))
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * Group SID = gsid (Compression for this field is not yet supported.)
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_GROUP))
  {
    pcOutData[n++] = '|';
  }

  /*-
   *********************************************************************
   *
   * DACL = dacl (Compression for this field is not yet supported.)
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_OWNER))
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
  dwLastVolumeSerialNumber = psFTFileData->dwVolumeSerialNumber;
  dwLastFileIndexHigh = psFTFileData->dwFileIndexHigh;
  dwLastFileIndexLow = psFTFileData->dwFileIndexLow;
  dwLastFileAttributes = psFTFileData->dwFileAttributes;
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
DevelopCompressHex(char *pcData, unsigned long ulHex, unsigned long ulOldHex)
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
