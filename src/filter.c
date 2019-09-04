/*-
 ***********************************************************************
 *
 * $Id: filter.c,v 1.8 2019/08/30 19:22:00 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2019-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * FilterApplyFilters
 *
 ***********************************************************************
 */
void
FilterApplyFilters(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, int iFilterWhen)
{
  char                acMessage[MESSAGE_SIZE] = "";
  FILTER_LIST_KLEL   *psFilter = NULL;
  int                 iDataFilterCount = 0;

  /*-
   *********************************************************************
   *
   * Conditionally apply exclude filters. These filters take precedence
   * over include filters. If there's a match, return immediately.
   *
   *********************************************************************
   */
  if (psProperties->psExcludeFilterListKlel)
  {
    psFilter = FilterMatchFilter(psProperties->psExcludeFilterListKlel, psFTFileData, iFilterWhen);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=[%s] RawPath=[%s]", psFilter->pcExpression, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      psFTFileData->iFiltered = iFilterWhen;
      return;
    }
  }

  /*-
   *********************************************************************
   *
   * Conditionally apply include filters. Set the filtered flag based
   * on when it's OK to filter implicitly exlcuded directories/files.
   * Implicitly exlcuded directories, in particular, must be traversed
   * first because they may contain other files/directories that could
   * match an explicit include filter. Additionally, if there are any
   * POST_DATA filters, then all implicitly exlcuded directories/files
   * must be defered until that filter point.
   *
   *********************************************************************
   */
  if (psProperties->psIncludeFilterListKlel)
  {
    psFilter = FilterMatchFilter(psProperties->psIncludeFilterListKlel, psFTFileData, iFilterWhen);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=[%s], RawPath=[%s]", psFilter->pcExpression, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      psFTFileData->iFiltered = 0; /* We matched an include filter. Clear the filtered flag to indicate this object should not be filtered. */
      return;
    }
    else
    {
      switch (iFilterWhen)
      {
      case FTIMES_FILTER_POST_NAME:
        psFTFileData->iFiltered = FTIMES_FILTER_POST_ATTR; /* Defer until file type is known. */
        break;
      case FTIMES_FILTER_POST_ATTR:
        psFTFileData->iFiltered = FTIMES_FILTER_POST_ATTR; /* No deferment needed. */
        iDataFilterCount = FilterGetFilterCount(psProperties->psIncludeFilterListKlel, FTIMES_FILTER_POST_DATA);
        if (iDataFilterCount > 0)
        {
          psFTFileData->iFiltered = FTIMES_FILTER_POST_DATA; /* Defer until the last filter point. */
        }
        else
        {
          if (psFTFileData->ulAttributeMask != 0)
          {
#ifdef WIN32
            if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
#else
            if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
#endif
            {
              psFTFileData->iFiltered = FTIMES_FILTER_POST_ATTR_SCAN; /* Defer until directory has been traversed. */
            }
          }
        }
        break;
      case FTIMES_FILTER_POST_DATA:
      default:
        psFTFileData->iFiltered = FTIMES_FILTER_POST_DATA; /* Defer until file data have been processed. */
        break;
      }
    }
  }

  return;
}


/*-
 ***********************************************************************
 *
 * FilterAddFilter
 *
 ***********************************************************************
 */
int
FilterAddFilter(char *pcExpression, FILTER_LIST_KLEL **psHead, char *pcError)
{
  const char          acRoutine[] = "FilterAddFilter()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FILTER_LIST_KLEL   *psCurrent = NULL;
  FILTER_LIST_KLEL   *psFilter = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new filter.
   *
   *********************************************************************
   */
  psFilter = FilterNewFilter(pcExpression, acLocalError);
  if (psFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * If the head is NULL, insert the new filter and return. Otherwise,
   * append each new/unique filter to the end of the list. Each filter
   * is assigned an ID, which is a checksum of the compiled expression.
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
      if (psFilter->uiId == psCurrent->uiId)
      {
        FilterFreeFilter(psFilter);
        return ER_OK;
      }
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
 * FilterCheckFilterMasks
 *
 ***********************************************************************
 */
int
FilterCheckFilterMasks(FILTER_LIST_KLEL *psFilterList, unsigned long ulMask, char *pcError)
{
  FILTER_LIST_KLEL   *psFilter = NULL;

  /*-
   *********************************************************************
   *
   * Verify that the attributes used by each filter are present in the
   * overall field mask.
   *
   *********************************************************************
   */
  for (psFilter = psFilterList; psFilter != NULL; psFilter = psFilter->psNext)
  {
    if (((ulMask ^ psFilter->ulMask) & psFilter->ulMask))
    {
      snprintf(pcError, MESSAGE_SIZE, "Filter = [%s]: Filter depends on an attribute not specified in field mask.", psFilter->pcExpression);
      return ER;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FilterClassifyFilter
 *
 ***********************************************************************
 */
int
FilterClassifyFilter(FILTER_LIST_KLEL *psFilter, char *pcError)
{
  const char          acRoutine[] = "FilterClassifyFilter()";
  char               *pcMask = NULL;
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iFilterType = FTIMES_FILTER_UNDEFINED;
  MASK_USS_MASK      *psMask = NULL;

  /*-
   *********************************************************************
   *
   * Evaluate the filter and generate a mask.
   *
   *********************************************************************
   */
  pcMask = FilterEvaluateFilterNode(psFilter->psContext->psExpression, 0, acLocalError);
  if (pcMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  psMask = MaskParseMask(pcMask, MASK_MASK_TYPE_FILTER, acLocalError);
  if (psMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Progressively adjust the filter type based on attributes present.
   *
   *********************************************************************
   */
  if (psMask->ulMask & FILTER_NAME)
  {
    iFilterType = FTIMES_FILTER_POST_NAME;
  }

  if (psMask->ulMask & (FILTER_ALL_MASK ^ FILTER_NAME ^ FILTER_MAGIC ^ FILTER_MD5 ^ FILTER_SHA1 ^ FILTER_SHA256))
  {
    iFilterType = FTIMES_FILTER_POST_ATTR; /* Any attributes except name, magic, and hashes. */
  }

  if (psMask->ulMask & (FILTER_MAGIC | FILTER_MD5 | FILTER_SHA1 | FILTER_SHA256))
  {
    iFilterType = FTIMES_FILTER_POST_DATA;
  }

  if (iFilterType == FTIMES_FILTER_UNDEFINED)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter has no relevant attribute coverage/criteria.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Finalize relevant filter members.
   *
   *********************************************************************
   */
  psFilter->iType = iFilterType;
  psFilter->ulMask = psMask->ulMask;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FilterConvertFromPcre
 *
 ***********************************************************************
 */
char *
FilterConvertFromPcre(char *pcExpression, char *pcConversionPrefix, char *pcError)
{
  const char          acRoutine[] = "FilterConvertFromPcre()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pc = NULL;
  char               *pcExpressionKlel = NULL;
  char               *pcExpressionKlelEscaped = NULL;
  char               *pcExpressionTemp = NULL;
  int                 iCount = 0;
  int                 iError= ER_OK;
  int                 iIndex = 0;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Insert the conversion prefix.
   *
   *********************************************************************
   */
  iError = MaskSetDynamicString(&pcExpressionKlel, pcConversionPrefix, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto FAIL;
  }

  /*-
   *********************************************************************
   *
   * Insert the expression's left-hand quote.
   *
   *********************************************************************
   */
  pcExpressionTemp = MaskAppendToDynamicString(pcExpressionKlel, "\"", acLocalError);
  if (pcExpressionTemp == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto FAIL;
  }
  pcExpressionKlel = pcExpressionTemp;

  /*-
   *********************************************************************
   *
   * Insert the expression.
   *
   *********************************************************************
   */
  pcExpressionTemp = MaskAppendToDynamicString(pcExpressionKlel, pcExpression, acLocalError);
  if (pcExpressionTemp == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto FAIL;
  }
  pcExpressionKlel = pcExpressionTemp;

  /*-
   *********************************************************************
   *
   * Insert the expression's left-hand quote.
   *
   *********************************************************************
   */
  pcExpressionTemp = MaskAppendToDynamicString(pcExpressionKlel, "\"", acLocalError);
  if (pcExpressionTemp == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto FAIL;
  }
  pcExpressionKlel = pcExpressionTemp;

  /*-
   *********************************************************************
   *
   * Clone the expression.
   *
   *********************************************************************
   */
  iError = MaskSetDynamicString(&pcExpressionKlelEscaped, pcExpressionKlel, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    goto FAIL;
  }

  /*-
   *********************************************************************
   *
   * Count up the number of escaped '\'s (i.e., '\\'). Tack on enough
   * room to hold the escapes that will be used to escape the escape.
   *
   *********************************************************************
   */
  iLength = strlen(pcExpressionKlel);
  for (iIndex = 0; iIndex < iLength; iIndex++)
  {
    if (pcExpressionKlel[iIndex] == '\\' && pcExpressionKlel[iIndex + 1] == '\\')
    {
      iCount++;
      iIndex++;
    }
  }
  pcExpressionTemp = (char *)realloc((void *)pcExpressionKlelEscaped, iLength + (iCount * 2) + 1);
  if (pcExpressionTemp == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
    free(pcExpressionKlelEscaped);
    pcExpressionKlelEscaped = NULL;
    goto FAIL;
  }
  pcExpressionKlelEscaped = pcExpressionTemp;

  /*-
   *********************************************************************
   *
   * Escape the escaped '\'s.
   *
   *********************************************************************
   */
  for (iIndex = 0, pc = pcExpressionKlelEscaped; iIndex < iLength; iIndex++)
  {
    if (pcExpressionKlel[iIndex] == '\\' && pcExpressionKlel[iIndex + 1] == '\\')
    {
      *pc++ = '\\'; /* This escape will be removed by KLEL. */
      *pc++ = '\\'; /* This escape will be removed by PCRE. */
      *pc++ = '\\'; /* This escape will be removed by KLEL. */
      *pc++ = '\\'; /* This is the actual character to be matched. */
      iIndex++;
    }
    else
    {
      *pc++ = pcExpressionKlel[iIndex];
    }
  }
  *pc = 0;

FAIL:
  if (pcExpressionKlel != NULL)
  {
    free(pcExpressionKlel);
  }

  return pcExpressionKlelEscaped;
}


/*-
 ***********************************************************************
 *
 * FilterEvaluateFilterNode
 *
 ***********************************************************************
 */
char *
FilterEvaluateFilterNode(KLEL_NODE *psNode, int iLevel, char *pcError)
{
  const char          acRoutine[] = "FilterEvaluateFilterNode()";
  static char        *pcMask = NULL;
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcTemp = NULL;
  int                 i = 0;
  int                 iError= ER_OK;

  if (iLevel == 0)
  {
    iError = MaskSetDynamicString(&pcMask, "none", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }
  }

  if (psNode->iType == KLEL_NODE_DESIGNATOR)
  {
    pcTemp = pcMask;
    if (strncmp("f_name", psNode->acFragment, strlen("f_name")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+name", acLocalError);
    }
#ifdef WIN32
    else if (strncmp("f_altstreams", psNode->acFragment, strlen("f_altstreams")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+altstreams", acLocalError);
    }
#endif
    else if (strncmp("f_atime", psNode->acFragment, strlen("f_atime")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+atime", acLocalError);
    }
#ifdef WIN32
    else if (strncmp("f_attributes", psNode->acFragment, strlen("f_attributes")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+attributes", acLocalError);
    }
    else if (strncmp("f_chtime", psNode->acFragment, strlen("f_chtime")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+chtime", acLocalError);
    }
#endif
    else if (strncmp("f_ctime", psNode->acFragment, strlen("f_ctime")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+ctime", acLocalError);
    }
#ifdef WIN32
    else if (strncmp("f_dacl", psNode->acFragment, strlen("f_dacl")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+dacl", acLocalError);
    }
#endif
#ifndef WIN32
    else if (strncmp("f_dev", psNode->acFragment, strlen("f_dev")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+dev", acLocalError);
    }
#endif
#ifdef WIN32
    else if (strncmp("f_findex", psNode->acFragment, strlen("f_findex")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+findex", acLocalError);
    }
#endif
#ifndef WIN32
    else if (strncmp("f_gid", psNode->acFragment, strlen("f_gid")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+gid", acLocalError);
    }
#endif
#ifdef WIN32
    else if (strncmp("f_gsid", psNode->acFragment, strlen("f_gsid")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+gsid", acLocalError);
    }
#endif
#ifndef WIN32
    else if (strncmp("f_inode", psNode->acFragment, strlen("f_inode")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+inode", acLocalError);
    }
#endif
    else if (strncmp("f_magic", psNode->acFragment, strlen("f_magic")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+magic", acLocalError);
    }
    else if (strncmp("f_md5", psNode->acFragment, strlen("f_md5")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+md5", acLocalError);
    }
#ifndef WIN32
    else if (strncmp("f_mode", psNode->acFragment, strlen("f_mode")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+mode", acLocalError);
    }
#endif
    else if (strncmp("f_mtime", psNode->acFragment, strlen("f_mtime")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+mtime", acLocalError);
    }
#ifndef WIN32
    else if (strncmp("f_nlink", psNode->acFragment, strlen("f_nlink")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+nlink", acLocalError);
    }
#endif
#ifdef WIN32
    else if (strncmp("f_osid", psNode->acFragment, strlen("f_osid")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+osid", acLocalError);
    }
#endif
#ifndef WIN32
    else if (strncmp("f_rdev", psNode->acFragment, strlen("f_rdev")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+rdev", acLocalError);
    }
#endif
    else if (strncmp("f_sha1", psNode->acFragment, strlen("f_sha1")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+sha1", acLocalError);
    }
    else if (strncmp("f_sha256", psNode->acFragment, strlen("f_sha256")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+sha256", acLocalError);
    }
    else if (strncmp("f_size", psNode->acFragment, strlen("f_size")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+size", acLocalError);
    }
#ifndef WIN32
    else if (strncmp("f_uid", psNode->acFragment, strlen("f_uid")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+uid", acLocalError);
    }
#endif
#ifdef WIN32
    else if (strncmp("f_volume", psNode->acFragment, strlen("f_volume")) == 0)
    {
      pcTemp = MaskAppendToDynamicString(pcMask, "+volume", acLocalError);
    }
#endif
    else
    {
      /* Fragment is not a known (or platform-supported) attribute. */
    }

    if (pcTemp == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }
    pcMask = pcTemp;
  }

  for (i = 0; i < KLEL_MAX_CHILDREN; i++)
  {
    if (psNode->apsChildren[i] != NULL)
    {
      pcTemp = FilterEvaluateFilterNode(psNode->apsChildren[i], iLevel + 1, pcError);
      if (pcTemp == NULL)
      {
        return NULL; /* Error message has already been set. */
      }
      pcMask = pcTemp;
    }
  }

  return pcMask;
}


/*-
 ***********************************************************************
 *
 * FilterFreeFilter
 *
 ***********************************************************************
 */
void
FilterFreeFilter(FILTER_LIST_KLEL *psFilter)
{
  if (psFilter != NULL)
  {
    if (psFilter->pcExpression != NULL)
    {
      free(psFilter->pcExpression);
    }
    KlelFreeContext(psFilter->psContext);
    free(psFilter);
  }
}


/*-
 ***********************************************************************
 *
 * FilterGetFilterCount
 *
 ***********************************************************************
 */
int
FilterGetFilterCount(FILTER_LIST_KLEL *psFilterList, int iType)
{
  FILTER_LIST_KLEL *psFilter = NULL;
  int                 iCount = 0;

  for (psFilter = psFilterList; psFilter != NULL; psFilter = psFilter->psNext)
  {
    if (psFilter->iType == iType)
    {
      iCount++;
    }
  }

  return iCount;
}


/*-
 ***********************************************************************
 *
 * FilterGetTypeOfVar
 *
 ***********************************************************************
 */
KLEL_EXPR_TYPE
FilterGetTypeOfVar(const char *pcName, void *pvContext)
{
  int                 i = 0;
  FILTER_TYPE_SPEC    asTypes[] =
  {
#ifdef WIN32
    { "f_altstreams",                     KLEL_TYPE_INT64   }, /* file altstreams */
#endif
    { "f_atime",                          KLEL_TYPE_INT64   }, /* file atime */
#ifdef WIN32
    { "f_attributes",                     KLEL_TYPE_INT64   }, /* file attributes */
    { "f_attributes_archive",             KLEL_TYPE_BOOLEAN }, /* file attributes: archive */
    { "f_attributes_compressed",          KLEL_TYPE_BOOLEAN }, /* file attributes: compressed */
    { "f_attributes_device",              KLEL_TYPE_BOOLEAN }, /* file attributes: device */
    { "f_attributes_directory",           KLEL_TYPE_BOOLEAN }, /* file attributes: directory */
    { "f_attributes_encrypted",           KLEL_TYPE_BOOLEAN }, /* file attributes: encryptedj */
    { "f_attributes_hidden",              KLEL_TYPE_BOOLEAN }, /* file attributes: hidden */
    { "f_attributes_normal",              KLEL_TYPE_BOOLEAN }, /* file attributes: normal */
    { "f_attributes_not_content_indexed", KLEL_TYPE_BOOLEAN }, /* file attributes: not content indexed */
    { "f_attributes_offline",             KLEL_TYPE_BOOLEAN }, /* file attributes: offline */
    { "f_attributes_read_only",           KLEL_TYPE_BOOLEAN }, /* file attributes: read only */
    { "f_attributes_reparse_point",       KLEL_TYPE_BOOLEAN }, /* file attributes: reparse point */
    { "f_attributes_sparse_file",         KLEL_TYPE_BOOLEAN }, /* file attributes: sparse */
    { "f_attributes_system",              KLEL_TYPE_BOOLEAN }, /* file attributes: system */
    { "f_attributes_temporary",           KLEL_TYPE_BOOLEAN }, /* file attributes: temporary */
    { "f_attributes_unknown_1",           KLEL_TYPE_BOOLEAN }, /* file attributes: undocumented */
    { "f_attributes_unknown_2",           KLEL_TYPE_BOOLEAN }, /* file attributes: undocumented */
    { "f_attributes_virtual",             KLEL_TYPE_BOOLEAN }, /* file attributes: virtual */
    { "f_chtime",                         KLEL_TYPE_INT64   }, /* file chtime */
#endif
    { "f_ctime",                          KLEL_TYPE_INT64   }, /* file ctime */
#ifdef WIN32
    { "f_dacl",                           KLEL_TYPE_STRING  }, /* file dacl */
#endif
#ifndef WIN32
    { "f_dev",                            KLEL_TYPE_INT64   }, /* file dev */
#endif
    { "f_exists",                         KLEL_TYPE_BOOLEAN }, /* file exists */
#ifdef WIN32
    { "f_findex",                         KLEL_TYPE_INT64   }, /* file findex */
#endif
#ifndef WIN32
    { "f_fstype",                         KLEL_TYPE_STRING  }, /* file system type */
    { "f_gid",                            KLEL_TYPE_INT64   }, /* file group ID */
#endif
#ifdef WIN32
    { "f_gsid",                           KLEL_TYPE_STRING  }, /* file group SID */
#endif
#ifndef WIN32
    { "f_inode",                          KLEL_TYPE_INT64   }, /* file inode */
#endif
    { "f_magic",                          KLEL_TYPE_STRING  }, /* file type (XMagic) */
    { "f_md5",                            KLEL_TYPE_STRING  }, /* file hash (MD5) */
#ifndef WIN32
    { "f_mode",                           KLEL_TYPE_INT64   }, /* file mode */
    { "f_mode_gr",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: group read */
    { "f_mode_gw",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: group write */
    { "f_mode_gx",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: group execute */
    { "f_mode_or",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: other read */
    { "f_mode_ow",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: other write */
    { "f_mode_ox",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: other execute */
    { "f_mode_sg",                        KLEL_TYPE_BOOLEAN }, /* file mode SGID (set group id on execution) bit */
    { "f_mode_st",                        KLEL_TYPE_BOOLEAN }, /* file mode sticky bit */
    { "f_mode_su",                        KLEL_TYPE_BOOLEAN }, /* file mode SUID (set user id on execution) bit */
    { "f_mode_tb",                        KLEL_TYPE_BOOLEAN }, /* file mode type: block special */
    { "f_mode_tc",                        KLEL_TYPE_BOOLEAN }, /* file mode type: character special */
    { "f_mode_td",                        KLEL_TYPE_BOOLEAN }, /* file mode type: directory */
    { "f_mode_tl",                        KLEL_TYPE_BOOLEAN }, /* file mode type: symbolic link */
    { "f_mode_tp",                        KLEL_TYPE_BOOLEAN }, /* file mode type: named pipe (fifo) */
    { "f_mode_tr",                        KLEL_TYPE_BOOLEAN }, /* file mode type: regular file */
    { "f_mode_ts",                        KLEL_TYPE_BOOLEAN }, /* file mode type: socket */
    { "f_mode_tw",                        KLEL_TYPE_BOOLEAN }, /* file mode type: whiteout */
    { "f_mode_ur",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: user read */
    { "f_mode_uw",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: user write */
    { "f_mode_ux",                        KLEL_TYPE_BOOLEAN }, /* file mode permission: user execute */
#endif
    { "f_mtime",                          KLEL_TYPE_INT64   }, /* file mtime */
    { "f_name",                           KLEL_TYPE_STRING  }, /* file name (full path) */
#ifndef WIN32
    { "f_nlink",                          KLEL_TYPE_INT64   }, /* file nlink */
#endif
#ifdef WIN32
    { "f_osid",                           KLEL_TYPE_STRING  }, /* file owner SID */
#endif
#ifndef WIN32
    { "f_rdev",                           KLEL_TYPE_INT64   }, /* file rdev */
#endif
    { "f_sha1",                           KLEL_TYPE_STRING  }, /* file hash (SHA1) */
    { "f_sha256",                         KLEL_TYPE_STRING  }, /* file hash (SHA256) */
    { "f_size",                           KLEL_TYPE_INT64   }, /* file size */
#ifndef WIN32
    { "f_uid",                            KLEL_TYPE_INT64   }, /* file user ID */
#endif
#ifdef WIN32
    { "f_volume",                         KLEL_TYPE_INT64   }, /* file volume ID */
#endif
  };

  for (i = 0; i < (int)(sizeof(asTypes) / sizeof(asTypes[0])); i++)
  {
    if (strcmp(asTypes[i].pcName, pcName) == 0)
    {
      return asTypes[i].iType;
    }
  }

  return KLEL_TYPE_UNKNOWN; /* This causes KLEL to retrieve the type of the specified variable, should it exist in the standard library. */
}


/*-
 ***********************************************************************
 *
 * FilterGetValueOfVar
 *
 ***********************************************************************
 */
KLEL_VALUE *
FilterGetValueOfVar(const char *pcName, void *pvContext)
{
  FTIMES_FILE_DATA   *psFTFileData = (FTIMES_FILE_DATA *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
#ifdef UNIX
  struct stat        *psFStatEntry = &psFTFileData->sStatEntry;
#endif

  if (pcName[0] == 'f' && pcName[1] == '_')
  {
    if (strcmp(pcName, "f_name") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_STRING, strlen(psFTFileData->pcRawPath), psFTFileData->pcRawPath);
    }

#ifdef WIN32
    if (strcmp(pcName, "f_altstreams") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)psFTFileData->iStreamCount);
    }
#endif
    if (strcmp(pcName, "f_atime") == 0)
    {
#ifdef WIN32
      APP_SI64 i64Time = ((APP_SI64)psFTFileData->sFTATime.dwHighDateTime << 32) + ((APP_SI64)psFTFileData->sFTATime.dwLowDateTime);
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)i64Time);
#else
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_atime));
#endif
    }
#ifdef WIN32
    if (strcmp(pcName, "f_attributes") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)psFTFileData->dwFileAttributes);
    }
    if (strcmp(pcName, "f_attributes_archive") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE);
    }
    if (strcmp(pcName, "f_attributes_compressed") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED);
    }
    if (strcmp(pcName, "f_attributes_device") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DEVICE);
    }
    if (strcmp(pcName, "f_attributes_directory") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    if (strcmp(pcName, "f_attributes_encrypted") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED);
    }
    if (strcmp(pcName, "f_attributes_hidden") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
    }
    if (strcmp(pcName, "f_attributes_normal") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_NORMAL);
    }
    if (strcmp(pcName, "f_attributes_not_content_indexed") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    }
    if (strcmp(pcName, "f_attributes_offline") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE);
    }
    if (strcmp(pcName, "f_attributes_read_only") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY);
    }
    if (strcmp(pcName, "f_attributes_reparse_point") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
    }
    if (strcmp(pcName, "f_attributes_sparse_file") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE);
    }
    if (strcmp(pcName, "f_attributes_system") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM);
    }
    if (strcmp(pcName, "f_attributes_temporary") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY);
    }
    if (strcmp(pcName, "f_attributes_unknown_1") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & 0x00008);
    }
    if (strcmp(pcName, "f_attributes_unknown_2") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & 0x08000);
    }
    if (strcmp(pcName, "f_attributes_virtual") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL);
    }
    if (strcmp(pcName, "f_chtime") == 0)
    {
      APP_SI64 i64Time = ((APP_SI64)psFTFileData->sFTChTime.dwHighDateTime << 32) + ((APP_SI64)psFTFileData->sFTChTime.dwLowDateTime);
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)i64Time);
    }
#endif
    if (strcmp(pcName, "f_ctime") == 0)
    {
#ifdef WIN32
      APP_SI64 i64Time = ((APP_SI64)psFTFileData->sFTCTime.dwHighDateTime << 32) + ((APP_SI64)psFTFileData->sFTCTime.dwLowDateTime);
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)i64Time);
#else
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_ctime));
#endif
    }
#ifdef WIN32
    if (strcmp(pcName, "f_dacl") == 0)
    {
      char *pcAclDacl = NULL;
      DWORD dwLength = 0;
      KLEL_VALUE *psValue = NULL;
      ConvertSecurityDescriptorToStringSecurityDescriptorA(psFTFileData->psSd, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &pcAclDacl, &dwLength);
      psValue = KlelCreateValue(KLEL_TYPE_STRING, strlen(pcAclDacl), pcAclDacl);
      LocalFree(pcAclDacl);
      return psValue;
    }
#endif
#ifndef WIN32
    if (strcmp(pcName, "f_dev") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_dev));
    }
#endif
    if (strcmp(pcName, "f_exists") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFTFileData->iFileExists);
    }
#ifdef WIN32
    if (strcmp(pcName, "f_findex") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(((APP_SI64)psFTFileData->dwFileIndexHigh << 32) | psFTFileData->dwFileIndexLow));
    }
#endif
    if (strcmp(pcName, "f_fstype") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_STRING, strlen(gaacFSType[psFTFileData->iFSType]), gaacFSType[psFTFileData->iFSType]);
    }
#ifndef WIN32
    if (strcmp(pcName, "f_gid") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_gid));
    }
#endif
#ifdef WIN32
    if (strcmp(pcName, "f_gsid") == 0)
    {
      char *pcSidGroup = NULL;
      KLEL_VALUE *psValue = NULL;
      ConvertSidToStringSidA(psFTFileData->psSidGroup, &pcSidGroup);
      psValue = KlelCreateValue(KLEL_TYPE_STRING, strlen(pcSidGroup), pcSidGroup);
      LocalFree(pcSidGroup);
      return psValue;
    }
#endif
#ifndef WIN32
    if (strcmp(pcName, "f_inode") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_ino));
    }
#endif
    if (strcmp(pcName, "f_magic") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_STRING, strlen(psFTFileData->acType), psFTFileData->acType);
    }
    if (strcmp(pcName, "f_md5") == 0)
    {
      char ac[MD5_HASH_SIZE*2+1];
      MD5HashToHex(psFTFileData->aucFileMd5, ac);
      return KlelCreateValue(KLEL_TYPE_STRING, MD5_HASH_SIZE*2, ac);
    }
#ifndef WIN32
    if (strcmp(pcName, "f_mode") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_gr") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IRGRP);
    }
    if (strcmp(pcName, "f_mode_gw") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IWGRP);
    }
    if (strcmp(pcName, "f_mode_gx") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IXGRP);
    }
    if (strcmp(pcName, "f_mode_or") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IROTH);
    }
    if (strcmp(pcName, "f_mode_ow") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IWOTH);
    }
    if (strcmp(pcName, "f_mode_ox") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IXOTH);
    }
    if (strcmp(pcName, "f_mode_sg") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_ISGID);
    }
    if (strcmp(pcName, "f_mode_st") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_ISVTX);
    }
    if (strcmp(pcName, "f_mode_su") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_ISUID);
    }
    if (strcmp(pcName, "f_mode_tb") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISBLK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tc") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISCHR(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_td") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISDIR(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tl") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISLNK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tp") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISFIFO(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tr") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISREG(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_ts") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISSOCK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tw") == 0)
    {
#ifdef S_ISWHT
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, S_ISWHT(psFStatEntry->st_mode));
#else
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, (((psFStatEntry->st_mode) & 0170000) == 0160000));
#endif
    }
    if (strcmp(pcName, "f_mode_ur") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IRUSR);
    }
    if (strcmp(pcName, "f_mode_uw") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IWUSR);
    }
    if (strcmp(pcName, "f_mode_ux") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_BOOLEAN, psFStatEntry->st_mode & S_IXUSR);
    }
#endif
    if (strcmp(pcName, "f_mtime") == 0)
    {
#ifdef WIN32
      APP_SI64 i64Time = ((APP_SI64)psFTFileData->sFTMTime.dwHighDateTime << 32) + ((APP_SI64)psFTFileData->sFTMTime.dwLowDateTime);
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)i64Time);
#else
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_mtime));
#endif
    }
#ifndef WIN32
    if (strcmp(pcName, "f_nlink") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_nlink));
    }
#endif
#ifdef WIN32
    if (strcmp(pcName, "f_osid") == 0)
    {
      char *pcSidOwner = NULL;
      KLEL_VALUE *psValue = NULL;
      ConvertSidToStringSidA(psFTFileData->psSidOwner, &pcSidOwner);
      psValue = KlelCreateValue(KLEL_TYPE_STRING, strlen(pcSidOwner), pcSidOwner);
      LocalFree(pcSidOwner);
      return psValue;
    }
#endif
#ifndef WIN32
    if (strcmp(pcName, "f_rdev") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_rdev));
    }
#endif
    if (strcmp(pcName, "f_sha1") == 0)
    {
      char ac[SHA1_HASH_SIZE*2+1];
      SHA1HashToHex(psFTFileData->aucFileSha1, ac);
      return KlelCreateValue(KLEL_TYPE_STRING, SHA1_HASH_SIZE*2, ac);
    }
    if (strcmp(pcName, "f_sha256") == 0)
    {
      char ac[SHA256_HASH_SIZE*2+1];
      SHA256HashToHex(psFTFileData->aucFileSha256, ac);
      return KlelCreateValue(KLEL_TYPE_STRING, SHA256_HASH_SIZE*2, ac);
    }
    if (strcmp(pcName, "f_size") == 0)
    {
#ifdef WIN32
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(((APP_SI64)psFTFileData->dwFileSizeHigh << 32) | psFTFileData->dwFileSizeLow));
#else
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_size));
#endif
    }
#ifndef WIN32
    if (strcmp(pcName, "f_uid") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)(psFStatEntry->st_uid));
    }
#endif
#ifdef WIN32
    if (strcmp(pcName, "f_volume") == 0)
    {
      return KlelCreateValue(KLEL_TYPE_INT64, (APP_SI64)psFTFileData->dwVolumeSerialNumber);
    }
#endif
  }

  return KlelCreateUnknown(); /* This causes KLEL to retrieve the value of the specified variable, should it exist in the standard library. */
}


/*-
 ***********************************************************************
 *
 * FilterMatchFilter
 *
 ***********************************************************************
 */
FILTER_LIST_KLEL *
FilterMatchFilter(FILTER_LIST_KLEL *psFilterList, FTIMES_FILE_DATA *psFTFileData, int iFilterWhen)
{
  BOOL                bMatch = FALSE;
  const char          acRoutine[] = "FilterMatchFilter()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FILTER_LIST_KLEL   *psFilter = NULL;
  KLEL_VALUE         *psResult = NULL;

  /*-
   *********************************************************************
   *
   * Walk the filter list. Return a pointer to the first match.
   *
   *********************************************************************
   */
  for (psFilter = psFilterList; psFilter != NULL; psFilter = psFilter->psNext)
  {
    if (psFilter->iType > iFilterWhen)
    {
      continue;
    }
    KlelSetPrivateData(psFilter->psContext, (void *)psFTFileData);
    psResult = KlelExecute(psFilter->psContext);
    if (psResult == NULL)
    {
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: KlelExecute(): Filter (0x%08x) failed to execute expression (%s).", acRoutine, psFTFileData->pcNeuteredPath, psFilter->uiId, KlelGetError(psFilter->psContext));
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
      continue;
    }
    bMatch = (psResult->bBoolean) ? TRUE : FALSE;
    KlelFreeResult(psResult);
    if (bMatch)
    {
      return psFilter;
    }
  }

  return NULL;
}


/*-
 ***********************************************************************
 *
 * FilterNewFilter
 *
 ***********************************************************************
 */
FILTER_LIST_KLEL *
FilterNewFilter(char *pcExpression, char *pcError)
{
  const char          acRoutine[] = "FilterNewFilter()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FILTER_LIST_KLEL   *psFilter = NULL;
  int                 iError = ER_OK;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Verify that the expression is defined and has some non-zero length.
   *
   *********************************************************************
   */
  if (pcExpression == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = []: NULL input. That shouldn't happen.", acRoutine);
    return NULL;
  }

  iLength = strlen(pcExpression);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = [%s]: Filter length (%d) must be greater than zero.", acRoutine, pcExpression, iLength);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory for a new filter. The caller should free this
   * memory with FilterFreeFilter().
   *
   *********************************************************************
   */
  psFilter = calloc(sizeof(FILTER_LIST_KLEL), 1);
  if (psFilter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = [%s]: calloc(): %s", acRoutine, pcExpression, strerror(errno));
    return NULL;
  }

  psFilter->pcExpression = calloc(iLength + 1, 1);
  if (psFilter->pcExpression == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = [%s]: calloc(): %s", acRoutine, pcExpression, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Compile the expression, classify the filter, and fill out remaining
   * member values.
   *
   *********************************************************************
   */
  psFilter->psContext = KlelCompile(pcExpression, 0, FilterGetTypeOfVar, FilterGetValueOfVar, NULL);
  if (!KlelIsValid(psFilter->psContext))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = [%s]: KlelCompile(): Filter failed to compile (%s).", acRoutine, pcExpression, KlelGetError(psFilter->psContext));
    FilterFreeFilter(psFilter);
    return NULL;
  }
  strncpy(psFilter->pcExpression, pcExpression, iLength);
  iError = FilterClassifyFilter(psFilter, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Filter = [%s]: %s", acRoutine, pcExpression, acLocalError);
    FilterFreeFilter(psFilter);
    return NULL;
  }
  psFilter->uiId = KlelGetChecksum(psFilter->psContext, KLEL_EXPRESSION_ONLY);
  psFilter->psNext = NULL;

  return psFilter;
}
