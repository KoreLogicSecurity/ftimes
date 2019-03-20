/*-
 ***********************************************************************
 *
 * $Id: compare.c,v 1.4 2003/01/16 21:36:32 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Globals
 *
 ***********************************************************************
 */
ALL_FIELDS_TABLE gsAllFieldsTable[ALL_FIELDS_TABLE_LENGTH] =
{
  { "name",       0 },
  { "dev",        1 },
  { "inode",      1 },
  { "volume",     1 },
  { "findex",     1 },
  { "mode",       1 },
  { "attributes", 1 },
  { "nlink",      1 },
  { "uid",        1 },
  { "gid",        1 },
  { "rdev",       1 },
  { "atime",      2 },
  { "ams",        0 },
  { "mtime",      2 },
  { "mms",        0 },
  { "ctime",      2 },
  { "cms",        0 },
  { "chtime",     2 },
  { "chms",       0 },
  { "size",       1 },
  { "altstreams", 1 },
  { "magic",      1 },
  { "md5",        1 }
};
static const int giAllFieldsTableLength = sizeof(gsAllFieldsTable) / sizeof(gsAllFieldsTable[0]);
static CMP_PROPERTIES *gpsCmpProperties;

/*-
 ***********************************************************************
 *
 * CompareDecodeLine
 *
 ***********************************************************************
 */
int
CompareDecodeLine(char *pcLine, unsigned long ulFieldsMask, char aacDecodeFields[][CMP_MAX_LINE_LENGTH], char *pcError)
{
  const char          acRoutine[] = "CompareDecodeLine()";
  char                acTempLine[CMP_MAX_LINE_LENGTH];
  char               *pcHead;
  char               *pcTail;
  int                 i;
  int                 iDone;
  int                 iFound;
  int                 iField;
  int                 iLength;

  /*-
   *********************************************************************
   *
   * Terminate each output field.
   *
   *********************************************************************
   */
  for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
  {
    aacDecodeFields[i][0] = 0;
  }

  /*-
   *********************************************************************
   *
   * Check the line's length.
   *
   *********************************************************************
   */
  iLength = strlen(pcLine);
  if (iLength > CMP_MAX_LINE_LENGTH - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", acRoutine, iLength, CMP_MAX_LINE_LENGTH - 1);
    return ER;
  }
  strncpy(acTempLine, pcLine, CMP_MAX_LINE_LENGTH);

  /*-
   *********************************************************************
   *
   * Parse data, and make a copy of each specified field.
   *
   *********************************************************************
   */
  for (pcHead = pcTail = acTempLine, iDone = iField = 0; !iDone; pcHead = ++pcTail, iField++)
  {
    while (*pcTail != CMP_SEPARATOR_C && *pcTail != 0)
    {
      pcTail++;
    }
    if (*pcTail == 0)
    {
      iDone = 1;
    }
    else
    {
      *pcTail = 0;
    }
    for (i = 0, iFound = -1; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      if ((ulFieldsMask & (1 << i)) == (unsigned long) 1 << i)
      {
        iFound++;
      }
      if (iFound == iField)
      {
        break;
      }
    }
    if (i != ALL_FIELDS_TABLE_LENGTH)
    {
      strcpy(aacDecodeFields[i], pcHead);
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareEnumerateChanges
 *
 ***********************************************************************
 */
int
CompareEnumerateChanges(char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "CompareEnumerateChanges()";
  char                aacBaselineFields[ALL_FIELDS_TABLE_LENGTH][CMP_MAX_LINE_LENGTH];
  char                aacSnapshotFields[ALL_FIELDS_TABLE_LENGTH][CMP_MAX_LINE_LENGTH];
  char                acLine[CMP_MAX_LINE_LENGTH];
  char                acLocalError[MESSAGE_SIZE];
  CMP_DATA            sCompareData;
  int                 iLastIndex;
  int                 iTempIndex;
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
  int                *piNodeIndex;
#endif
  CMP_PROPERTIES     *psProperties;
  FILE               *pFile;
  int                 i;
  int                 iError;
  int                 iFound;
  int                 iKeysIndex;
  int                 iNodeCount;
  int                 iNodeIndex;
  int                 iLineLength;
  int                 iLineNumber;
  int                 iToLower;
  unsigned char       aucHash[MD5_HASH_LENGTH];

  acLocalError[0] = iLineLength = iLineNumber = 0;

  psProperties = CompareGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Open the snapshot file.
   *
   *********************************************************************
   */
  if (strcmp(pcFilename, "-") == 0)
  {
    pFile = stdin;
  }
  else
  {
    pFile = fopen(pcFilename, "rb");
    if (pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
      return ER;
    }
  }

  /*-
   *********************************************************************
   *
   * Read and parse the header.
   *
   *********************************************************************
   */
  iLineNumber++;
  iError = CompareReadHeader(pFile, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
    fclose(pFile);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Adjust the compare mask to look at the fields we actually have.
   *
   *********************************************************************
   */
  psProperties->ulCompareMask &= psProperties->ulFieldsMask;
  if (psProperties->ulCompareMask == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: None of the specified fields exist in the snapshot data.", acRoutine, pcFilename, iLineNumber);
    fclose(pFile);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the ToLower conversion flag.
   *
   *********************************************************************
   */
  iToLower = (psProperties->piCompareRoutine == strcasecmp) ? 1 : 0;

  /*-
   *********************************************************************
   *
   * Enumerate changed and new files.
   *
   *********************************************************************
   */
  iNodeCount = iNodeIndex = 0;
  for (acLine[0] = 0, iLineNumber++; fgets(acLine, CMP_MAX_LINE_LENGTH, pFile) != NULL; acLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Preprocess the line.
     *
     *******************************************************************
     */
    iError = ComparePreprocessLine(pFile, iToLower, acLine, &iLineLength, aucHash, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      fclose(pFile);
      return ER;
    }

#ifdef USE_SNAPSHOT_COLLISION_DETECTION
    /*-
     *******************************************************************
     *
     * Check memory size, and allocate more, if necessary.
     *
     *******************************************************************
     */
    if (iNodeIndex >= iNodeCount)
    {
      iNodeCount += CMP_NODE_REQUEST_COUNT;
      psProperties->psSnapshotNodes = (CMP_NODE *) realloc(psProperties->psSnapshotNodes, (iNodeCount * sizeof(CMP_NODE)));
      if (psProperties->psSnapshotNodes == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
        fclose(pFile);
        return ER;
      }
    }

    /*-
     *******************************************************************
     *
     * Insert a new node. Drop collisions and warn the user.
     *
     *******************************************************************
     */
    piNodeIndex = CompareGetNodeIndexReference(aucHash, psProperties->aiSnapshotKeys, psProperties->psSnapshotNodes);
    if (piNodeIndex == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: Hash collision. Check for duplicate filenames.", acRoutine, pcFilename, iLineNumber);
      ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    }
    else
    {
      *piNodeIndex = iNodeIndex;
      psProperties->psSnapshotNodes[*piNodeIndex].iNextIndex = -1;
      memcpy(psProperties->psSnapshotNodes[*piNodeIndex].aucHash, aucHash, CMP_HASHB_SIZE);
      iNodeIndex++;
    }
#endif

    /*-
     *******************************************************************
     *
     * Search for the hash and compare specified fields.
     *
     *******************************************************************
     */
    psProperties->ulAnalyzed++;
    iFound = 0;
    CompareDecodeLine(acLine, psProperties->ulFieldsMask, aacSnapshotFields, acLocalError);
    sCompareData.cCategory = 0;
    sCompareData.pcRecord = NULL;
    iKeysIndex = CMP_GET_NODE_INDEX(aucHash);
    iLastIndex = iTempIndex = psProperties->aiBaselineKeys[iKeysIndex];
    while (iTempIndex != -1)
    {
      if (memcmp(psProperties->psBaselineNodes[iTempIndex].aucHash, aucHash, CMP_HASHB_SIZE) == 0)
      {
        iFound = psProperties->psBaselineNodes[iTempIndex].iFound = 1;
        CompareDecodeLine(psProperties->psBaselineNodes[iTempIndex].pcData, psProperties->ulFieldsMask, aacBaselineFields, acLocalError);
        sCompareData.ulChangedMask = 0;
        sCompareData.ulUnknownMask = 0;
        for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
        {
          if ((psProperties->ulCompareMask & 1 << i) == (unsigned long) 1 << i)
          {
            if (aacBaselineFields[i][0] != 0 && aacSnapshotFields[i][0] != 0)
            {
              if (strcmp(aacBaselineFields[i], aacSnapshotFields[i]) != 0)
              {
                sCompareData.ulChangedMask |= 1 << i;
              }
            }
            else
            {
              sCompareData.ulUnknownMask |= 1 << i;
            }
          }
        }
        if (sCompareData.ulChangedMask && !sCompareData.ulUnknownMask)
        {
          sCompareData.cCategory = 'C';
          psProperties->ulChanged++;
        }
        else if (!sCompareData.ulChangedMask && sCompareData.ulUnknownMask)
        {
          sCompareData.cCategory = 'U';
          psProperties->ulUnknown++;
        }
        else if (sCompareData.ulChangedMask && sCompareData.ulUnknownMask)
        {
          sCompareData.cCategory = 'X';
          psProperties->ulCrossed++;
        }
        sCompareData.pcRecord = aacBaselineFields[0];
        break;
      }
      iLastIndex = iTempIndex;
      iTempIndex = psProperties->psBaselineNodes[iTempIndex].iNextIndex;
    }
    if (iFound == 0 || iLastIndex == -1)
    {
      sCompareData.cCategory = 'N';
      sCompareData.pcRecord = aacSnapshotFields[0];
      psProperties->ulNew++;
    }
    else if (sCompareData.cCategory == 0)
    {
      continue; /* Nothing to report. */
    }
    iError = CompareWriteRecord(psProperties, &sCompareData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      fclose(pFile);
      return ER;
    }
  }
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fgets(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    fclose(pFile);
    return ER;
  }
  fclose(pFile);

  /*-
   *********************************************************************
   *
   * Enumerate missing objects.
   *
   *********************************************************************
   */
  for (iKeysIndex = 0; iKeysIndex < CMP_MODULUS; iKeysIndex++)
  {
    iTempIndex = psProperties->aiBaselineKeys[iKeysIndex];
    while (iTempIndex != -1)
    {
      if (psProperties->psBaselineNodes[iTempIndex].iFound == 0)
      {
        sCompareData.cCategory = 'M';
        sCompareData.pcRecord = psProperties->psBaselineNodes[iTempIndex].pcData;
        iError = CompareWriteRecord(psProperties, &sCompareData, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
          return ER;
        }
        psProperties->ulMissing++;
      }
      iTempIndex = psProperties->psBaselineNodes[iTempIndex].iNextIndex;
    }
  }
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareFreeProperties
 *
 ***********************************************************************
 */
void
CompareFreeProperties(CMP_PROPERTIES *psProperties)
{
  if (psProperties != NULL)
  {
    if (psProperties->pcPackedData != NULL)
    {
      free(psProperties->pcPackedData);
    }
    if (psProperties->psBaselineNodes != NULL)
    {
      free(psProperties->psBaselineNodes);
    }
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
    if (psProperties->psSnapshotNodes != NULL)
    {
      free(psProperties->psSnapshotNodes);
    }
#endif
    free(psProperties);
  }
}


/*-
 ***********************************************************************
 *
 * CompareGetChangedCount
 *
 ***********************************************************************
 */
int
CompareGetChangedCount(void)
{
  return gpsCmpProperties->ulChanged;
}


/*-
 ***********************************************************************
 *
 * CompareGetCrossedCount
 *
 ***********************************************************************
 */
int
CompareGetCrossedCount(void)
{
  return gpsCmpProperties->ulCrossed;
}


/*-
 ***********************************************************************
 *
 * CompareGetMissingCount
 *
 ***********************************************************************
 */
int
CompareGetMissingCount(void)
{
  return gpsCmpProperties->ulMissing;
}


/*-
 ***********************************************************************
 *
 * CompareGetNewCount
 *
 ***********************************************************************
 */
int
CompareGetNewCount(void)
{
  return gpsCmpProperties->ulNew;
}


/*-
 ***********************************************************************
 *
 * CompareGetNodeIndexReference
 *
 ***********************************************************************
 */
int *
CompareGetNodeIndexReference(unsigned char *pucHash, int *piKeys, CMP_NODE *psNodes)
{
  int                 iKeysIndex;
  int                 iLastIndex;
  int                 iTempIndex;

  iKeysIndex = CMP_GET_NODE_INDEX(pucHash);

  if ((iLastIndex = iTempIndex = piKeys[iKeysIndex]) == -1)
  {
    return &piKeys[iKeysIndex];
  }
  else
  {
    while (iTempIndex != -1)
    {
      if (memcmp(psNodes[iTempIndex].aucHash, pucHash, CMP_HASHB_SIZE) == 0)
      {
        return NULL;
      }
      iLastIndex = iTempIndex;
      iTempIndex = psNodes[iTempIndex].iNextIndex;
    }
    return &psNodes[iLastIndex].iNextIndex;
  }
}


/*-
 ***********************************************************************
 *
 * CompareGetPropertiesReference
 *
 ***********************************************************************
 */
CMP_PROPERTIES *
CompareGetPropertiesReference(void)
{
  return gpsCmpProperties;
}


/*-
 ***********************************************************************
 *
 * CompareGetRecordCount
 *
 ***********************************************************************
 */
int
CompareGetRecordCount(void)
{
  return gpsCmpProperties->ulAnalyzed;
}


/*-
 ***********************************************************************
 *
 * CompareGetUnknownCount
 *
 ***********************************************************************
 */
int
CompareGetUnknownCount(void)
{
  return gpsCmpProperties->ulUnknown;
}


/*-
 ***********************************************************************
 *
 * CompareLoadBaselineData
 *
 ***********************************************************************
 */
int
CompareLoadBaselineData(char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "CompareLoadBaselineData()";
  char                acLine[CMP_MAX_LINE_LENGTH];
  char                acLocalError[MESSAGE_SIZE];
  char               *pcPackedData;
  CMP_PROPERTIES     *psProperties;
  FILE               *pFile;
  int                 iError;
  int                 iFirst;
  int                 iLineLength;
  int                 iLineNumber;
  int                 iNodeCount;
  int                 iNodeIndex;
  int                 iToLower;
  int                *piNodeIndex;
  struct stat         statEntry;
  unsigned char       aucHash[MD5_HASH_LENGTH];

  acLocalError[0] = iLineLength = iLineNumber = 0;

  psProperties = CompareGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Open the baseline.
   *
   *********************************************************************
   */
  pFile = fopen(pcFilename, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory to hold the data.
   *
   *********************************************************************
   */
  iError = fstat(fileno(pFile), &statEntry);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fstat(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    fclose(pFile);
    return ER;
  }
  pcPackedData = malloc(statEntry.st_size);
  if (pcPackedData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    fclose(pFile);
    return ER;
  }
  psProperties->pcPackedData = pcPackedData;

  /*-
   *********************************************************************
   *
   * Read and parse the header.
   *
   *********************************************************************
   */
  iLineNumber++;
  iError = CompareReadHeader(pFile, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
    fclose(pFile);
    return ER;
  }

  iFirst = 1;
  iNodeCount = iNodeIndex = iToLower = 0;
  for (acLine[0] = 0, iLineNumber++; fgets(acLine, CMP_MAX_LINE_LENGTH, pFile) != NULL; acLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Determine the compare function based on the OsType. This affects
     * name digest calculations and comparisions. This test expects
     * WIN32 files to begin with a drive letter followed by a colon.
     *
     *******************************************************************
     */
    if (iFirst)
    {
      if (isupper(toupper((int) acLine[1])) && acLine[2] == ':')
      {
        psProperties->piCompareRoutine = strcasecmp; /* WIN32 */
        iToLower = 1;
      }
      else
      {
        psProperties->piCompareRoutine = strcmp; /* UNIX */
        iToLower = 0;
      }
      iFirst = 0;
    }

    /*-
     *******************************************************************
     *
     * Preprocess the line.
     *
     *******************************************************************
     */
    iError = ComparePreprocessLine(pFile, iToLower, acLine, &iLineLength, aucHash, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      fclose(pFile);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Check memory size, and allocate more, if necessary.
     *
     *******************************************************************
     */
    if (iNodeIndex >= iNodeCount)
    {
      iNodeCount += CMP_NODE_REQUEST_COUNT;
      psProperties->psBaselineNodes = (CMP_NODE *) realloc(psProperties->psBaselineNodes, (iNodeCount * sizeof(CMP_NODE)));
      if (psProperties->psBaselineNodes == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
        fclose(pFile);
        return ER;
      }
    }

    /*-
     *******************************************************************
     *
     * Insert a new node. Drop collisions and warn the user.
     *
     *******************************************************************
     */
    piNodeIndex = CompareGetNodeIndexReference(aucHash, psProperties->aiBaselineKeys, psProperties->psBaselineNodes);
    if (piNodeIndex == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: Hash collision. Check for duplicate filenames.", acRoutine, pcFilename, iLineNumber);
      ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    }
    else
    {
      *piNodeIndex = iNodeIndex;
      psProperties->psBaselineNodes[*piNodeIndex].iNextIndex = -1;
      memcpy(psProperties->psBaselineNodes[*piNodeIndex].aucHash, aucHash, CMP_HASHB_SIZE);
      psProperties->psBaselineNodes[*piNodeIndex].pcData = pcPackedData;
      strcpy(psProperties->psBaselineNodes[*piNodeIndex].pcData, acLine);
      pcPackedData += iLineLength + 1;
      iNodeIndex++;
    }
  }
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fgets(): File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    fclose(pFile);
    return ER;
  }
  fclose(pFile);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareNewProperties
 *
 ***********************************************************************
 */
CMP_PROPERTIES *
CompareNewProperties(char *pcError)
{
  const char          acRoutine[] = "CompareNewProperties()";
  CMP_PROPERTIES     *psProperties;
  int                 i;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (CMP_PROPERTIES *) calloc(sizeof(CMP_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Initialize non-zero variables.
   *
   *********************************************************************
   */
  for (i = 0; i < CMP_MODULUS; i++)
  {
    psProperties->aiBaselineKeys[i] = -1;
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
    psProperties->aiSnapshotKeys[i] = -1;
#endif
  }

#ifdef FTimes_WIN32
  psProperties->piCompareRoutine = strcasecmp;
  strncpy(psProperties->acNewLine, CRLF, NEWLINE_LENGTH);
#endif
#ifdef FTimes_UNIX
  psProperties->piCompareRoutine = strcmp;
  strncpy(psProperties->acNewLine, LF, NEWLINE_LENGTH);
#endif

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * CompareParseStringMask
 *
 ***********************************************************************
 */
int
CompareParseStringMask(char *pcMask, unsigned long *ulMask, int iRunMode, MASK_TABLE *pMaskTable, int iMaskTableLength, char *pcError)
{
  const char          acRoutine[] = "CompareParseStringMask()";
  char                acTempLine[ALL_FIELDS_MASK_SIZE];
  char                cLastAction;
  char                cNextAction;
  char               *pcToken;
  int                 i;
  int                 j;
  int                 iDone;
  int                 iLength;
  int                 iOffset;

  iLength = strlen(pcMask);
  if (iLength > ALL_FIELDS_MASK_SIZE - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Argument exceeds [%d] bytes.", acRoutine, iLength, ALL_FIELDS_MASK_SIZE - 1);
    return ER;
  }

  if (strncasecmp(pcMask, "ALL", 3) == 0)
  {
    *ulMask = ~0;
    iLength = 3;
  }
  else if (strncasecmp(pcMask, "NONE", 4) == 0)
  {
    *ulMask = 0;
    iLength = 4;
  }
  else
  {
    *ulMask = 0;
    snprintf(pcError, MESSAGE_SIZE, "%s: Prefix = [%s] != [ALL|NONE]: Invalid prefix.", acRoutine, pcMask);
    return ER;
  }

  switch (pcMask[iLength])
  {
  case '+':
  case '-':
    cLastAction = '?';
    cNextAction = pcMask[iLength++];
    break;
  case 0:
    *ulMask &= (iRunMode == FTIMES_CMPMODE) ? ~0 : ALL_MASK;
    return ER_OK;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Operator = [%c] != [+|-]: Invalid operator.", acRoutine, pcMask[iLength]);
    return ER;
    break;
  }

  /*-
   *********************************************************************
   *
   * Copy the remainder of the input to a scratch pad.
   *
   *********************************************************************
   */
  strncpy(acTempLine, &pcMask[iLength], ALL_FIELDS_MASK_SIZE);

  /*-
   *********************************************************************
   *
   * Remove EOL characters.
   *
   *********************************************************************
   */
  SupportChopEOLs(acTempLine, 0, NULL);

  /*-
   *********************************************************************
   *
   * Scan through the string looking for tokens delimited by '+', '-',
   * or 0.
   *
   *********************************************************************
   */
  for (pcToken = acTempLine, iOffset = 0, iDone = 0; !iDone;)
  {
    if (acTempLine[iOffset] == '+' || acTempLine[iOffset] == '-' || acTempLine[iOffset] == 0)
    {
      if (acTempLine[iOffset] == 0)
      {
        iDone = 1;
      }

      /*-
       *****************************************************************
       *
       * Update the action values.
       *
       *****************************************************************
       */
      cLastAction = cNextAction;
      cNextAction = acTempLine[iOffset];

      /*-
       *****************************************************************
       *
       * Terminate the token.
       *
       *****************************************************************
       */
      acTempLine[iOffset] = 0;

      /*-
       *****************************************************************
       *
       * Scan the table looking for this token. Add or subtract the
       * expanded token value (i.e. the xtime tokens count for more
       * than one mask bit each) from the mask depending on whether
       * '+' or '-' was given.
       *
       *****************************************************************
       */
      if (iRunMode == FTIMES_CMPMODE)
      {
        for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
        {
          if (strcasecmp(pcToken, gsAllFieldsTable[i].acName) == 0)
          {
            if (gsAllFieldsTable[i].iBitsToSet == 0)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: Token = [%c%s]: Illegal value.", acRoutine, cLastAction, pcToken);
              return ER;
            }
            for (j = 0; j < gsAllFieldsTable[i].iBitsToSet; j++)
            {
              PUTBIT(*ulMask, ((cLastAction == '+') ? 1 : 0), (i + j));
            }
            break;
          }
        }
        if (i == ALL_FIELDS_TABLE_LENGTH)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Token = [%c%s]: Invalid value.", acRoutine, cLastAction, pcToken);
          return ER;
        }
      }
      else
      {
        for (i = 0; i < iMaskTableLength; i++)
        {
          if (pMaskTable[i].MaskName[0] && strcasecmp(pcToken, pMaskTable[i].MaskName) == 0)
          {
            PUTBIT(*ulMask, ((cLastAction == '+') ? 1 : 0), i);
            break;
          }
        }
        if (i == iMaskTableLength)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Token = [%c%s]: Invalid value.", acRoutine, cLastAction, pcToken);
          return ER;
        }
      }
      if (!iDone)
      {
        iOffset++;
        pcToken = &acTempLine[iOffset];
      }
    }
    else
    {
      iOffset++;
    }
  }

  *ulMask &= (iRunMode == FTIMES_CMPMODE) ? ~0 : ALL_MASK;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * ComparePreprocessLine
 *
 ***********************************************************************
 */
int
ComparePreprocessLine(FILE *pFile, int iToLower, char *pcLine, int *piLength, unsigned char *pucHash, char *pcError)
{
  const char          acRoutine[] = "ComparePreprocessLine()";
  char                acLocalError[MESSAGE_SIZE];
  char                acName[CMP_MAX_LINE_LENGTH];
  int                 i;
  int                 iLength;

  /*-
   *********************************************************************
   *
   * Remove EOL characters.
   *
   *********************************************************************
   */
  if (SupportChopEOLs(pcLine, feof(pFile) ? 0 : 1, acLocalError) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Calculate line length.
   *
   *********************************************************************
   */
  *piLength = strlen(pcLine);

  /*-
   *********************************************************************
   *
   * Extract the name, and calculate it's hash.
   *
   *********************************************************************
   */
  for (iLength = 0; iLength < *piLength && pcLine[iLength] != CMP_SEPARATOR_C; iLength++)
  {
    acName[iLength] = pcLine[iLength];
  }
  acName[iLength] = 0;
  if (iToLower)
  {
    for (i = 0; i < iLength; i++)
    {
      if (acName[i] >= 'A' && acName[i] <= 'Z')
      {
        acName[i] += 0x20;
      }
    }
  }
  md5_string((unsigned char *) acName, iLength, pucHash);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareReadHeader
 *
 ***********************************************************************
 */
int
CompareReadHeader(FILE *pFile, char *pcError)
{
  const char          acRoutine[] = "CompareReadHeader()";
  char                acHeader[CMP_MAX_LINE_LENGTH];
  char                acLocalError[MESSAGE_SIZE];
  char               *pcHead;
  char               *pcTail;
  CMP_PROPERTIES     *psProperties;
  int                 i;
  int                 iDone;
  int                 iFields;

  psProperties = CompareGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Read header line.
   *
   *********************************************************************
   */
  fgets(acHeader, CMP_MAX_LINE_LENGTH, pFile);
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fgets(): %s", acRoutine, strerror(errno));
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Remove EOL characters.
   *
   *********************************************************************
   */
  if (SupportChopEOLs(acHeader, feof(pFile) ? 0 : 1, acLocalError) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Check that there's at least a name field.
   *
   *********************************************************************
   */
  if (strncmp(acHeader, "name", 4) != 0)
  {
    if (strncmp(acHeader, "zname", 5) == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Header magic indicates that this file contains compressed data.", acRoutine);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Header magic is not recognized.", acRoutine);
    }
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Parse header, and construct a field mask.
   *
   *********************************************************************
   */
  for (pcHead = pcTail = acHeader, iDone = iFields = 0; !iDone; pcHead = ++pcTail, iFields++)
  {
    while (*pcTail != CMP_SEPARATOR_C && *pcTail != 0)
    {
      pcTail++;
    }
    if (*pcTail == 0)
    {
      iDone = 1;
    }
    else
    {
      *pcTail = 0;
    }
    for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      if (strcasecmp(pcHead, gsAllFieldsTable[i].acName) == 0)
      {
        PUTBIT(psProperties->ulFieldsMask, 1, i);
        break;
      }
    }
    if (i == ALL_FIELDS_TABLE_LENGTH)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Token = [%s]: Unknown field.", acRoutine, (*pcHead == 0) ? "" : pcHead);
      return ER;
    }
  }

  psProperties->iFields = iFields;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareSetMask
 *
 ***********************************************************************
 */
void
CompareSetMask(unsigned long ulMask)
{
  gpsCmpProperties->ulCompareMask = ulMask;
}


/*-
 ***********************************************************************
 *
 * CompareSetNewLine
 *
 ***********************************************************************
 */
void
CompareSetNewLine(char *pcNewLine)
{
  strcpy(gpsCmpProperties->acNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
}


/*-
 ***********************************************************************
 *
 * CompareSetOutputStream
 *
 ***********************************************************************
 */
void
CompareSetOutputStream(FILE *pFile)
{
  gpsCmpProperties->pFileOut = pFile;
}


/*-
 ***********************************************************************
 *
 * CompareSetPropertiesReference
 *
 ***********************************************************************
 */
void
CompareSetPropertiesReference(CMP_PROPERTIES *psProperties)
{
  gpsCmpProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * CompareWriteHeader
 *
 ***********************************************************************
 */
int
CompareWriteHeader(FILE *pFile, char *pcNewLine, char *pcError)
{
  const char          acRoutine[] = "CompareWriteHeader()";
  char                acHeader[FTIMES_MAX_LINE];
  char                acLocalError[MESSAGE_SIZE];
  int                 iError;
  int                 iIndex;

  iIndex = sprintf(acHeader, "category|name|changed|unknown%s", pcNewLine);

  iError = SupportWriteData(pFile, acHeader, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareWriteRecord
 *
 ***********************************************************************
 */
int
CompareWriteRecord(CMP_PROPERTIES *psProperties, CMP_DATA *psData, char *pcError)
{
  const char          acRoutine[] = "CompareWriteRecord()";
  char                acLocalError[MESSAGE_SIZE];
  char               *pc;
  int                 i;
  int                 iError;
  int                 iFirst;
  int                 iIndex;
  unsigned long       ul;

  /*-
   *********************************************************************
   *
   * category      1
   * name          CMP_MAX_LINE_LENGTH
   * changed       ALL_FIELDS_MASK_SIZE
   * unknown       ALL_FIELDS_MASK_SIZE
   * |'s           3
   * newline       2
   * -----------------------------------------------------------------
   * Total         CMP_MAX_LINE_LENGTH + 2*ALL_FIELDS_MASK_SIZE + 6
   *
   *********************************************************************
   */
  char                acOutput[CMP_MAX_LINE_LENGTH + (2 * ALL_FIELDS_MASK_SIZE) + 6];

  /*-
   *********************************************************************
   *
   * Category = category
   *
   *********************************************************************
   */
  switch (psData->cCategory)
  {
  case 'C': /* changed */
  case 'M': /* missing */
  case 'N': /* new */
  case 'U': /* unknown */
  case 'X': /* both changed and unknown */
    acOutput[0] = psData->cCategory;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Category = [%c] != [C|M|N|U|X]: That shouldn't happen.", acRoutine, psData->cCategory);
    return ER;
    break;
  }
  iIndex = 1;

  /*-
   *********************************************************************
   *
   * Name = name
   *
   *********************************************************************
   */
  pc = strstr(&psData->pcRecord[1], "\"");
  if (psData->pcRecord[0] != '"' || pc == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Name = [%s]: Name is not quoted. That shouldn't happen.", acRoutine, psData->pcRecord);
    return ER;
  }
  acOutput[iIndex++] = CMP_SEPARATOR_C;
  for (i = 0; i <= pc - psData->pcRecord; i++)
  {
    acOutput[iIndex++] = psData->pcRecord[i];
  }

  /*-
   *********************************************************************
   *
   * Changed, Unknown, Cross = changed, unknown, cross
   *
   *********************************************************************
   */
  if (psData->cCategory == 'C' || psData->cCategory == 'U' || psData->cCategory == 'X')
  {
    acOutput[iIndex++] = CMP_SEPARATOR_C;
    for (i = 0, iFirst = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      ul = 1 << i;
      if ((psData->ulChangedMask & ul) == ul)
      {
        iIndex += sprintf(&acOutput[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", gsAllFieldsTable[i].acName);
      }
    }
    acOutput[iIndex++] = CMP_SEPARATOR_C;
    for (i = 0, iFirst = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      ul = 1 << i;
      if ((psData->ulUnknownMask & ul) == ul)
      {
        iIndex += sprintf(&acOutput[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", gsAllFieldsTable[i].acName);
      }
    }
  }
  else
  {
    acOutput[iIndex++] = CMP_SEPARATOR_C;
    acOutput[iIndex++] = CMP_SEPARATOR_C;
  }

  /*-
   *********************************************************************
   *
   * Newline
   *
   *********************************************************************
   */
  iIndex += sprintf(&acOutput[iIndex], "%s", psProperties->acNewLine);

  /*-
   *********************************************************************
   *
   * Write the output data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, acOutput, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  return ER_OK;
}
