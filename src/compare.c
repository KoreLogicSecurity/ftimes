/*
 ***********************************************************************
 *
 * $Id: compare.c,v 1.1.1.1 2002/01/18 03:17:29 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "all-includes.h"

ALL_FIELDS_TABLE      AllFieldsTable[ALL_FIELDS_TABLE_LENGTH] =
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
static const int      AllFieldsTableLength = sizeof(AllFieldsTable) / sizeof(AllFieldsTable[0]);

#ifdef FTimes_WIN32
static char           gcNewLine[NEWLINE_LENGTH] = CRLF;
#endif
#ifdef FTimes_UNIX
static char           gcNewLine[NEWLINE_LENGTH] = LF;
#endif
unsigned char        *gpucHashDB;
unsigned long         gulHashTable[1 << CMP_HASHB_SIZE];
CMP_PROPERTIES        gsCompareProperties;


/*-
 ***********************************************************************
 *
 * CompareCreateDatabase
 *
 ***********************************************************************
 */
int
CompareCreateDatabase(char *pcFilename, char *pcError)
{
  const char          cRoutine[] = "CompareCreateDatabase()";
  char                cLocalError[ERRBUF_SIZE];
  FILE               *pFile;
  int                 i;
  int                 j;
  int                 iError;
  int                 iFirst;
  int                 iLength;
  int                 iLineNumber;
  int                 iSaveLength;
  unsigned char       ucLine[CMP_MAX_LINE_LENGTH];
  unsigned char       ucName[CMP_MAX_LINE_LENGTH];
  unsigned char       ucNameMD5[MD5_HASH_LENGTH];
  unsigned long       ulIndex;
  unsigned long       ulStored;
  unsigned long       ulLast = 0;
  unsigned long       ulHashDBFree = 0;

  /*-
   *********************************************************************
   *
   * Get a _whole_ bunch of memory.
   *
   *********************************************************************
   */
  gpucHashDB = (unsigned char *) malloc(CMP_ARRAY_SIZE * sizeof(char)); /* The caller must free this storage. */
  if (gpucHashDB == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER_BadHandle;
  }

  /*-
   *********************************************************************
   *
   * Load system description into hash database.
   *
   *********************************************************************
   */
  strcpy(&gpucHashDB[ulHashDBFree], "Baseline");
  ulHashDBFree += strlen("Baseline") + 1;

  /*-
   *********************************************************************
   *
   * Open the baseline file.
   *
   *********************************************************************
   */
  pFile = fopen(pcFilename, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilename, strerror(errno));
    return ER_fopen;
  }

  /*-
   *********************************************************************
   *
   * Read the header line.
   *
   *********************************************************************
   */
  iLineNumber = 1;
  fgets(ucLine, CMP_MAX_LINE_LENGTH, pFile);
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER_fgets;
  }

  /*-
   *********************************************************************
   *
   * Test that this is a valid file by parsing the header.
   *
   *********************************************************************
   */
  iError = CompareDecodeHeader(ucLine, cLocalError);
  if (iError != ER_OK)
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, cLocalError);
    return iError;
  }

  for (ucLine[0] = 0, iFirst = 1, iLineNumber++; fgets(ucLine, CMP_MAX_LINE_LENGTH, pFile) != NULL; ucLine[0] = 0, iLineNumber++)
  {
    iLength = iSaveLength = strlen(ucLine);

    /*-
     *******************************************************************
     *
     * Determine the compare function based on the OsType. This affects
     * name digest calculations and comparisions. This test expects
     * WINDOWS files to begin with a drive letter followed by a colon.
     *
     *******************************************************************
     */
    if (iFirst)
    {
      if (isupper(toupper((int) ucLine[1])) && ucLine[2] == ':')
      {
        gsCompareProperties.piCompareRoutine = strcasecmp; /* WIN32 */
      }
      else
      {
        gsCompareProperties.piCompareRoutine = strcmp; /* UNIX */
      }
      iFirst = 0;
    }

    /*-
     *******************************************************************
     *
     * Scan backwards over EOL characters.
     *
     *******************************************************************
     */
    while (iLength && ((ucLine[iLength - 1] == '\r') || (ucLine[iLength - 1] == '\n')))
    {
      iLength--;
    }

    /*-
     *******************************************************************
     *
     * Its an error if no EOL was found. The exception to this is EOF.
     *
     *******************************************************************
     */
    if (iLength == iSaveLength && !feof(pFile))
    {
      fclose(pFile);
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: Length exceeds %d bytes.", cRoutine, pcFilename, iLineNumber, CMP_MAX_LINE_LENGTH - 1);
      return ER_Length;
    }
    
    /*-
     *******************************************************************
     *
     * Terminate the line excluding any EOL characters.
     *
     *******************************************************************
     */
    ucLine[iLength] = 0;

    /*-
     *******************************************************************
     *
     * Extract the name for name digest calculation.
     *
     *******************************************************************
     */
    for (i = 0; i < iLength && ucLine[i] != '|'; i++)
    {
      ucName[i] = ucLine[i];
    }
    ucName[i] = 0;

    /*-
     *******************************************************************
     *
     * If this is WIN32 data, tolower() the filename.
     *
     *******************************************************************
     */
    if (gsCompareProperties.piCompareRoutine == strcasecmp)
    {
      for (i = 0; i < (int) strlen(ucName); i++)
      {
        ucName[i] = (char) tolower((int) ucName[i]);
      }
    }

    /*-
     *******************************************************************
     *
     * Compute the name digest from the possibly modified name.
     *
     *******************************************************************
     */
    md5_string((unsigned char *) ucName, strlen(ucName), ucNameMD5);

    /*-
     *******************************************************************
     *
     * Check for enough room in the database.
     *
     *******************************************************************
     */
    if ((ulHashDBFree + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE + iLength) > CMP_ARRAY_SIZE)
    {
      fclose(pFile);
      snprintf(pcError, ERRBUF_SIZE, "%s: HashDB array overflow.", cRoutine);
      return ER_Overflow;
    }

    /*-
     *******************************************************************
     *
     * Compute an array index based on some portion of the low 3 bytes
     * of the name digest.
     *
     *******************************************************************
     */
    ulIndex = (ucNameMD5[13] << 16) | (ucNameMD5[14] << 8) | ucNameMD5[15];
    ulIndex &= CMP_HASH_MASK;

    /*-
     *******************************************************************
     *
     * Search for this hash in table.
     *
     *******************************************************************
     */
    if (gulHashTable[ulIndex] == 0)
    {
      /*-
       *****************************************************************
       *
       * This is the first occurrence of this index.
       *
       *****************************************************************
       */
      gulHashTable[ulIndex] = ulHashDBFree;
      for (i = 0; i < CMP_HASHB_SIZE; i++)
      {
        gpucHashDB[ulHashDBFree++] = ucNameMD5[i];
      }
      for (i = 0; i < CMP_NEXT_NAME_SIZE; i++)
      {
        gpucHashDB[ulHashDBFree++] = 0;
      }
      for (i = 0; i < 32; i += 8)
      {
        gpucHashDB[ulHashDBFree++] = (unsigned char) 0;
      }
      gpucHashDB[ulHashDBFree++] = 0; /* Initialize the found flag. */
      strcpy(&gpucHashDB[ulHashDBFree], ucLine);
      ulHashDBFree += iLength;
      gpucHashDB[ulHashDBFree++] = 0;
    }
    else
    {
      ulIndex = gulHashTable[ulIndex];
      ulStored = 0;
      while (ulIndex != 0)
      {
        /*-
         ***************************************************************
         *
         * Check hash.
         *
         ***************************************************************
         */
        for (i = 0; i < CMP_HASHB_SIZE; i++)
        {
          if (gpucHashDB[ulIndex + i] != ucNameMD5[i])
          {
            break;
          }
        }

        if (i != CMP_HASHB_SIZE)
        {
          /*-
           *************************************************************
           *
           * The hash didn't match. Advance to next hash. Collect $200.
           *
           *************************************************************
           */
          ulLast = ulIndex + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE;
          ulIndex = 0;
          for (j = 0; j < CMP_HASHC_SIZE; j += 8)
          {
            ulIndex |= gpucHashDB[ulLast++] << j;
          }
        }
        else
        {
          /*-
           *************************************************************
           *
           * Hash matched. If CMP_DEPLETED, check for matching filename.
           * If not CMP_DEPLETED or no matching filename then add to end.
           *
           *************************************************************
           */
          ulIndex += CMP_HASHB_SIZE;
          do
          {
            if (CMP_DEPLETED)
            {
              if (strcmp(ucLine, &gpucHashDB[ulIndex + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE]) == 0)
              {
                /*-
                 *******************************************************
                 *
                 * Hash and filename matched, so just deep six this one.
                 *
                 *******************************************************
                 */
                ulStored = 1;
                break;
              }
            }

            ulLast = ulIndex;
            ulIndex = 0;
            for (j = 0; j < CMP_HASHC_SIZE; j += 8)
            {
              ulIndex |= gpucHashDB[ulLast++] << j;
            }
          } while (ulIndex != 0);

          if (ulStored == 1)
          {
            break;
          }

          ulLast -= CMP_NEXT_HASH_SIZE;
          for (i = 0; i < 32; i += 8)
          {
            gpucHashDB[ulLast++] = (unsigned char) ((ulHashDBFree >> i) & 0xff);
          }
          for (i = 0; i < 32; i += 8)
          {
            gpucHashDB[ulHashDBFree++] = (unsigned char) 0;
          }
          gpucHashDB[ulHashDBFree++] = 0; /* Initialize the found flag. */
          strcpy(&gpucHashDB[ulHashDBFree], ucLine);
          ulHashDBFree += iLength;
          gpucHashDB[ulHashDBFree++] = 0;

          ulStored = 1;
          break;
        }
      }

      if (ulStored == 0)
      {
        ulLast -= CMP_NEXT_HASH_SIZE;
        for (i = 0; i < 32; i += 8)
        {
          gpucHashDB[ulLast++] = (unsigned char) ((ulHashDBFree >> i) & 0xff);
        }
        for (i = 0; i < CMP_HASHB_SIZE; i++)
        {
          gpucHashDB[ulHashDBFree++] = ucNameMD5[i];
        }
        for (i = 0; i < CMP_NEXT_NAME_SIZE; i++)
        {
          gpucHashDB[ulHashDBFree++] = 0;
        }
        for (i = 0; i < 32; i += 8)
        {
          gpucHashDB[ulHashDBFree++] = (unsigned char) 0;
        }
        gpucHashDB[ulHashDBFree++] = 0; /* Initialize the found flag. */
        strcpy(&gpucHashDB[ulHashDBFree], ucLine);
        ulHashDBFree += iLength;
        gpucHashDB[ulHashDBFree++] = 0;
      }
    }
  }
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER_fgets;
  }
  fclose(pFile);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareDecodeHeader
 *
 ***********************************************************************
 */
int
CompareDecodeHeader(char *pcLine, char *pcError)
{
  const char          cRoutine[] = "CompareDecodeHeader()";
  char               *pc;
  char                cTempLine[CMP_MAX_LINE_LENGTH];
  int                 i;
  int                 iFields;
  int                 iLength;

  /*-
   *********************************************************************
   *
   * Check input length.
   *
   *********************************************************************
   */
  iLength = strlen(pcLine);
  if (iLength > CMP_MAX_LINE_LENGTH - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", cRoutine, iLength, CMP_MAX_LINE_LENGTH - 1);
    return ER_Length;
  }
  strncpy(cTempLine, pcLine, CMP_MAX_LINE_LENGTH);

  /*-
   *********************************************************************
   *
   * Check that we at least have the name field.
   *
   *********************************************************************
   */
  if (strncmp(pcLine, "name", 4) != 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Unrecognized input stream.", cRoutine);
    return ER_Header;
  }

  /*-
   *********************************************************************
   *
   * Scan backwards over EOL characters.
   *
   *********************************************************************
   */
  while (iLength && ((cTempLine[iLength - 1] == '\r') || (cTempLine[iLength - 1] == '\n')))
  {
    iLength--;
  }
  cTempLine[iLength] = 0;

  for (pc = strtok(cTempLine, CMP_FIELD_SEPARATOR), iFields = 0; pc != NULL; pc = strtok(NULL, CMP_FIELD_SEPARATOR), iFields++)
  {
    for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      if (strcasecmp(pc, AllFieldsTable[i].cName) == 0)
      {
        PUTBIT(gsCompareProperties.ulFieldsMask, 1, i);
        break;
      }
    }
  }

  gsCompareProperties.iFields = iFields;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareDecodeLine
 *
 ***********************************************************************
 */
int
CompareDecodeLine(char *pcLine, CMP_FIELD_VALUE_TABLE *decodeFields, char *pcError)
{
  const char          cRoutine[] = "CompareDecodeLine()";
  char                cTempLine[CMP_MAX_LINE_LENGTH];
  char               *pcH;
  char               *pcT;
  int                 i;
  int                 iFound;
  int                 iItem;
  int                 iLength;

  /*-
   *********************************************************************
   *
   * Check input length.
   *
   *********************************************************************
   */
  iLength = strlen(pcLine);
  if (iLength > CMP_MAX_LINE_LENGTH - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", cRoutine, iLength, CMP_MAX_LINE_LENGTH - 1);
    return ER_Length;
  }
  strncpy(cTempLine, pcLine, CMP_MAX_LINE_LENGTH);

  /*-
   *********************************************************************
   *
   * Clear the compare fields.
   *
   *********************************************************************
   */
  for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
  {
    decodeFields[i].cValue[0] = 0;
  }

  iItem = 0;
  pcH = pcT = cTempLine;
  while (*pcH != 0)
  {
    while (*pcT != '|' && *pcT != 0)
    {
      pcT++;
    }

    *pcT = 0;
    iFound = -1;
    for (i = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      if ((gsCompareProperties.ulFieldsMask & (1 << i)) == (unsigned int) 1 << i)
      {
        iFound++;
      }
      if (iFound == iItem)
      {
        break;
      }
    }
    if (i != ALL_FIELDS_TABLE_LENGTH)
    {
      strcpy(decodeFields[i].cValue, pcH);
    }

    iItem++;
    pcH = ++pcT;
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
  const char          cRoutine[] = "CompareEnumerateChanges()";
  char                cLocalError[ERRBUF_SIZE];
  FILE               *pFile;
  int                 i;
  int                 j;
  int                 iError;
  int                 iLength;
  int                 iLevel;
  int                 iLineNumber;
  int                 iSaveLength;
  unsigned char       ucName[CMP_MAX_LINE_LENGTH];
  unsigned char       ucNameMD5[MD5_HASH_LENGTH];
  unsigned char       ucLine[CMP_MAX_LINE_LENGTH];
  unsigned long       ulChangedMask;
  unsigned long       ulIndex;
  unsigned long       ulLast;
  unsigned long       ulStored;
  unsigned long       ulTop;
  unsigned long       ulUnknownMask;
  CMP_FIELD_VALUE_TABLE sBaseFields[ALL_FIELDS_TABLE_LENGTH];
  CMP_FIELD_VALUE_TABLE sSnapFields[ALL_FIELDS_TABLE_LENGTH];

  cLocalError[0] = 0;

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
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilename, strerror(errno));
      return ER_fopen;
    }
  }

  /*-
   *********************************************************************
   *
   * Read the header line.
   *
   *********************************************************************
   */
  iLineNumber = 1;
  fgets(ucLine, CMP_MAX_LINE_LENGTH, pFile);
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER_fgets;
  }

  /*-
   *********************************************************************
   *
   * Test that this is a valid file by parsing the header.
   *
   *********************************************************************
   */
  iError = CompareDecodeHeader(ucLine, cLocalError);
  if (iError != ER_OK)
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Adjust the compare mask to look at the fields we actually have.
   *
   *********************************************************************
   */
  gsCompareProperties.ulCompareMask &= gsCompareProperties.ulFieldsMask;

  if (gsCompareProperties.ulCompareMask == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: None of the specified fields exist in the snapshot data.", cRoutine);
    return ER_NothingToDo;
  }

  /*-
   *********************************************************************
   *
   * Enumerate changed and new files.
   *
   *********************************************************************
   */
  for (ucLine[0] = 0, iLineNumber++; fgets(ucLine, CMP_MAX_LINE_LENGTH, pFile) != NULL; ucLine[0] = 0, iLineNumber++)
  {
    iLength = iSaveLength = strlen(ucLine);

    /*-
     *******************************************************************
     *
     * Scan backwards over EOL characters.
     *
     *******************************************************************
     */
    while (iLength && ((ucLine[iLength - 1] == '\r') || (ucLine[iLength - 1] == '\n')))
    {
      iLength--;
    }

    /*-
     *******************************************************************
     *
     * Its an error if no EOL was found. The exception to this is EOF.
     *
     *******************************************************************
     */
    if (iLength == iSaveLength && !feof(pFile))
    {
      fclose(pFile);
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: Length exceeds %d bytes.", cRoutine, pcFilename, iLineNumber, CMP_MAX_LINE_LENGTH - 1);
      return ER_Length;
    }
  
    /*-
     *******************************************************************
     *
     * Terminate the line excluding any EOL characters.
     *
     *******************************************************************
     */
    ucLine[iLength] = 0;

    /*-
     *******************************************************************
     *
     * Decode the line.
     *
     *******************************************************************
     */
    CompareDecodeLine(ucLine, sSnapFields, cLocalError);

    /*-
     *******************************************************************
     *
     * Extract the name to calculate it's digest.
     *
     *******************************************************************
     */
    for (i = 0; i < iLength && ucLine[i] != '|'; i++)
    {
      ucName[i] = ucLine[i];
    }
    ucName[i] = 0;

    /*-
     *******************************************************************
     *
     * If this is WINDOWS data, tolower() the filename.
     *
     *******************************************************************
     */
    if (gsCompareProperties.piCompareRoutine == strcasecmp)
    {
      for (i = 0; i < (int) strlen(ucName); i++)
      {
        ucName[i] = (char) tolower((int) ucName[i]);
      }
    }

    /*-
     *******************************************************************
     *
     * Compute a digest for the possibly modified name.
     *
     *******************************************************************
     */
    md5_string((unsigned char *) ucName, strlen(ucName), ucNameMD5);

    /*-
     *******************************************************************
     *
     * Compute an index based on the low 3 bytes of the name digest.
     *
     *******************************************************************
     */
    ulIndex = (ucNameMD5[13] << 16) | (ucNameMD5[14] << 8) | ucNameMD5[15];
    ulIndex &= CMP_HASH_MASK;

    /*-
     *******************************************************************
     *
     * Search for this hash in table. If not found, the file is new.
     *
     *******************************************************************
     */
    if (gulHashTable[ulIndex] == 0)
    {
      iError = CompareWriteRecord('N', sSnapFields[0].cValue, 0, 0, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        return iError;
      }
      gsCompareProperties.ulNew++;
    }
    else
    {
      ulIndex = gulHashTable[ulIndex];
      ulStored = 0;
      while (ulIndex != 0)
      {
        /*-
         ***************************************************************
         *
         * Check hash.
         *
         ***************************************************************
         */
        for (i = 0; i < CMP_HASHB_SIZE; i++)
        {
          if (gpucHashDB[ulIndex + i] != ucNameMD5[i])
          {
            break;
          }
        }

        if (i != CMP_HASHB_SIZE)
        {
          /*-
           *************************************************************
           *
           * The hash didn't match. Advance to next hash. Collect $200.
           *
           *************************************************************
           */
          ulLast = ulIndex + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE;
          ulIndex = 0;
          for (j = 0; j < CMP_HASHC_SIZE; j += 8)
          {
            ulIndex |= gpucHashDB[ulLast++] << j;
          }
        }
        else
        {
          /*-
           *************************************************************
           *
           * Hash matched. Look for a filename match.
           *
           *************************************************************
           */
          ulIndex += CMP_HASHB_SIZE;
          iLevel = 1;
          do
          {
            /*-
             ***********************************************************
             *
             * Decode the line.
             *
             ***********************************************************
             */
            if (iLevel == 1)
            {
              CompareDecodeLine(&gpucHashDB[ulIndex + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE], sBaseFields, cLocalError);
            }
            else
            {
              CompareDecodeLine(&gpucHashDB[ulIndex + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE], sBaseFields, cLocalError);
            }

            /*-
             ***********************************************************
             *
             * If the names are the same, compare each requested field
             * provided that the field is not NULL in either data set.
             *
             ***********************************************************
             */
            if (gsCompareProperties.piCompareRoutine(sBaseFields[0].cValue, sSnapFields[0].cValue) == 0)
            {
              for (i = 0, ulChangedMask = ulUnknownMask = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
              {
                if ((gsCompareProperties.ulCompareMask & 1 << i) == (unsigned int) 1 << i)
                {
                  if (sBaseFields[i].cValue[0] != 0 && sSnapFields[i].cValue[0] != 0)
                  {
                    if (strcmp(sBaseFields[i].cValue, sSnapFields[i].cValue) != 0)
                    {
                      ulChangedMask |= 1 << i;
                    }
                  }
                  else
                  {
                    ulUnknownMask |= 1 << i;
                  }

                  /*-
                   *****************************************************
                   *
                   * Mark entry as found.
                   *
                   *****************************************************
                   */
                  if (iLevel == 1)
                  {
                    gpucHashDB[ulIndex + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE] = 1;
                  }
                  else
                  {
                    gpucHashDB[ulIndex + CMP_NEXT_HASH_SIZE] = 1;
                  }
                }
              }

              /*-
               *********************************************************
               *
               * It's changed. Write out a record.
               *
               * FYI - The whole input line is located at the following address:
               *
               * &gpucHashDB[ulIndex + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE]
               *
               *********************************************************
               */
              if (ulChangedMask && !ulUnknownMask)
              {
                iError = CompareWriteRecord('C', sBaseFields[0].cValue, ulChangedMask, ulUnknownMask, cLocalError);
                if (iError != ER_OK)
                {
                  snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
                  return iError;
                }
                gsCompareProperties.ulChanged++;
              }
              else if (!ulChangedMask && ulUnknownMask)
              {
                iError = CompareWriteRecord('U', sBaseFields[0].cValue, ulChangedMask, ulUnknownMask, cLocalError);
                if (iError != ER_OK)
                {
                  snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
                  return iError;
                }
                gsCompareProperties.ulUnknown++;
              }
              else if (ulChangedMask && ulUnknownMask)
              {
                iError = CompareWriteRecord('X', sBaseFields[0].cValue, ulChangedMask, ulUnknownMask, cLocalError);
                if (iError != ER_OK)
                {
                  snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
                  return iError;
                }
                gsCompareProperties.ulChanged++;
                gsCompareProperties.ulUnknown++;
              }
              gsCompareProperties.ulFound++;
            }

            iLevel++;
            ulLast = ulIndex;
            ulIndex = 0;
            for (j = 0; j < CMP_HASHC_SIZE; j += 8)
            {
              ulIndex |= gpucHashDB[ulLast++] << j;
            }
          } while (ulIndex != 0);

          ulStored = 1;
          break;
        }
      }

      /*-
       *****************************************************************
       *
       * It's new. Write out a record.
       *
       *****************************************************************
       */
      if (ulStored == 0)
      {
        iError = CompareWriteRecord('N', sSnapFields[0].cValue, 0, 0, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return iError;
        }
        gsCompareProperties.ulNew++;
      }
    }
  }
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER_fgets;
  }
  fclose(pFile);

  /*-
   *********************************************************************
   *
   * Enumerate missing files.
   *
   *********************************************************************
   */
  for (i = 0; i < sizeof(gulHashTable) / sizeof(gulHashTable[0]); i++)
  {
    if (gulHashTable[i] != 0)
    {
      ulIndex = gulHashTable[i];
      while (ulIndex != 0)
      {
        ulTop = ulIndex;

        /*-
         ***************************************************************
         *
         * Check found flag. If not found, write out a missing record.
         *
         ***************************************************************
         */
        if (gpucHashDB[ulTop + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE] == 0)
        {
          iError = CompareWriteRecord('M', &gpucHashDB[ulIndex + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE], 0, 0, cLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
            return iError;
          }
          gsCompareProperties.ulMissing++;
        }

        /*-
         ***************************************************************
         *
         * Follow left branch of files with the same name digest.
         *
         ***************************************************************
         */
        ulIndex += CMP_HASHB_SIZE;
        if (gpucHashDB[ulIndex] != 0)
        {
          do
          {
            if (gpucHashDB[ulIndex + CMP_NEXT_HASH_SIZE] == 0)
            {
              iError = CompareWriteRecord('M', &gpucHashDB[ulIndex + CMP_HASHB_SIZE + CMP_NEXT_NAME_SIZE + CMP_NEXT_HASH_SIZE + CMP_FOUND_SIZE], 0, 0, cLocalError);
              if (iError != ER_OK)
              {
                snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
                return iError;
              }
              gsCompareProperties.ulMissing++;
            }

            ulLast = ulIndex;
            ulIndex = 0;
            for (j = 0; j < CMP_HASHC_SIZE; j += 8)
            {
              ulIndex |= gpucHashDB[ulLast++] << j;
            }
          } while (ulIndex != 0);
        }

        /*-
         ***************************************************************
         *
         * Go back to the top of the last branch and descend right.
         *
         ***************************************************************
         */
        ulLast = ulTop + CMP_HASHB_SIZE + CMP_NEXT_HASH_SIZE;
        ulIndex = 0;
        for (j = 0; j < CMP_HASHC_SIZE; j += 8)
        {
          ulIndex |= gpucHashDB[ulLast++] << j;
        }
      }
    }
  }
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CompareFreeDatabase
 *
 ***********************************************************************
 */
void
CompareFreeDatabase(void)
{
  if (gpucHashDB != NULL)
  {
    free(gpucHashDB);
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
  return gsCompareProperties.ulChanged;
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
  return gsCompareProperties.ulMissing;
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
  return gsCompareProperties.ulNew;
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
  return gsCompareProperties.ulFound;
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
  return gsCompareProperties.ulUnknown;
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
  const char          cRoutine[] = "CompareParseStringMask()";
  char                cLastAction;
  char                cNextAction;
  char                cTempLine[ALL_FIELDS_MASK_SIZE];
  char               *pcToken;
  int                 i;
  int                 j;
  int                 iDone;
  int                 iLength;
  int                 iOffset;

  iLength = strlen(pcMask);
  if (iLength > ALL_FIELDS_MASK_SIZE - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Argument exceeds [%d] bytes.", cRoutine, iLength, ALL_FIELDS_MASK_SIZE - 1);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Prefix = [%s] != [ALL|NONE]: Invalid prefix.", cRoutine, pcMask);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Operator = [%c] != [+|-]: Invalid operator.", cRoutine, pcMask[iLength]);
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
  strncpy(cTempLine, &pcMask[iLength], ALL_FIELDS_MASK_SIZE);
  iLength = strlen(cTempLine);

  /*-
   *********************************************************************
   *
   * Scan backwards over EOL characters. Expect no more than two
   * iterations of this loop.
   *
   *********************************************************************
   */
  while (iLength && ((cTempLine[iLength - 1] == '\r') || (cTempLine[iLength - 1] == '\n')))
  {
    iLength--;
  }
  cTempLine[iLength] = 0;

  /*-
   *********************************************************************
   *
   * Scan through the string looking for tokens delimited by '+', '-',
   * or 0.
   *
   *********************************************************************
   */
  for (pcToken = cTempLine, iOffset = 0, iDone = 0; !iDone;)
  {
    if (cTempLine[iOffset] == '+' || cTempLine[iOffset] == '-' || cTempLine[iOffset] == 0)
    {
      if (cTempLine[iOffset] == 0)
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
      cNextAction = cTempLine[iOffset];

      /*-
       *****************************************************************
       *
       * Terminate the token.
       *
       *****************************************************************
       */
      cTempLine[iOffset] = 0;

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
/* FIXME Remove global reference to AllFieldsTable. */
          if (strcasecmp(pcToken, AllFieldsTable[i].cName) == 0)
          {
            if (AllFieldsTable[i].iBitsToSet == 0)
            {
              snprintf(pcError, ERRBUF_SIZE, "%s: Token = [%c%s]: Illegal value.", cRoutine, cLastAction, pcToken);
              return ER;
            }
            for (j = 0; j < AllFieldsTable[i].iBitsToSet; j++)
            {
              PUTBIT(*ulMask, ((cLastAction == '+') ? 1 : 0), (i + j));
            }
            break;
          }
        }

        if (i == ALL_FIELDS_TABLE_LENGTH)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Token = [%c%s]: Invalid value.", cRoutine, cLastAction, pcToken);
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
          snprintf(pcError, ERRBUF_SIZE, "%s: Token = [%c%s]: Invalid value.", cRoutine, cLastAction, pcToken);
          return ER;
        }
      }

      if (!iDone)
      {
        iOffset++;
        pcToken = &cTempLine[iOffset];
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
 * CompareSetMask
 *
 ***********************************************************************
 */
void
CompareSetMask(unsigned long ulMask)
{
  gsCompareProperties.ulCompareMask = ulMask;
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
  strcpy(gcNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
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
  gsCompareProperties.pFileOut = pFile;
}


/*-
 ***********************************************************************
 *
 * CompareWriteHeader
 *
 ***********************************************************************
 */
int
CompareWriteHeader(FILE *pFile, char *pcError)
{
  const char          cRoutine[] = "CompareWriteHeader()";
  char                cHeaderData[FTIMES_MAX_LINE];
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  int                 iIndex;

  iIndex = sprintf(cHeaderData, "category|name|changed|unknown%s", gcNewLine);

  iError = SupportWriteData(pFile, cHeaderData, iIndex, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
CompareWriteRecord(char cCategory, char *pcRecord, unsigned long ulChangedMask, unsigned long ulUnknownMask, char *pcError)
{
  const char          cRoutine[] = "CompareWriteRecord()";
  char                cLocalError[ERRBUF_SIZE];
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
  char                cOutput[CMP_MAX_LINE_LENGTH + (2 * ALL_FIELDS_MASK_SIZE) + 6];

  /*-
   *********************************************************************
   *
   * Category = category
   *
   *********************************************************************
   */
  switch (cCategory)
  {
  case 'C': /* changed */
  case 'M': /* missing */
  case 'N': /* new */
  case 'U': /* unknown */
  case 'X': /* both changed and unknown */
    cOutput[0] = cCategory;
    break;
  default:
    snprintf(pcError, ERRBUF_SIZE, "%s: Category = [%c] != [C|M|N|U|X]: That shouldn't happen.", cRoutine, cCategory);
    return ER_BadValue;
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
  
  pc = strstr(&pcRecord[1], "\"");
  if (pcRecord[0] != '"' || pc == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Name = [%s]: Name is not quoted. That shouldn't happen.", cRoutine, pcRecord);
    return ER_BadValue;
  }
  cOutput[iIndex++] = '|';
  for (i = 0; i <= pc - pcRecord; i++)
  {
    cOutput[iIndex++] = pcRecord[i];
  }

  /*-
   *********************************************************************
   *
   * Changed, Unknown, Cross = changed, unknown, cross
   *
   *********************************************************************
   */
  if (cCategory == 'C' || cCategory == 'U' || cCategory == 'X')
  {
    cOutput[iIndex++] = '|';
    for (i = 0, iFirst = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      ul = 1 << i;
      if ((ulChangedMask & ul) == ul)
      {
        iIndex += sprintf(&cOutput[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", AllFieldsTable[i].cName);
      }
    }
    cOutput[iIndex++] = '|';
    for (i = 0, iFirst = 0; i < ALL_FIELDS_TABLE_LENGTH; i++)
    {
      ul = 1 << i;
      if ((ulUnknownMask & ul) == ul)
      {
        iIndex += sprintf(&cOutput[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", AllFieldsTable[i].cName);
      }
    }
  }
  else
  {
    cOutput[iIndex++] = '|';
    cOutput[iIndex++] = '|';
  }

  /*-
   *********************************************************************
   *
   * Newline
   *
   *********************************************************************
   */
  iIndex += sprintf(&cOutput[iIndex], "%s", gcNewLine);

  /*-
   *********************************************************************
   *
   * Write the output data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(gsCompareProperties.pFileOut, cOutput, iIndex, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  return ER_OK;
}
