/*-
 ***********************************************************************
 *
 * $Id: mask.c,v 1.10 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Global variables
 *
 ***********************************************************************
 */
/*-
 ***********************************************************************
 *
 * NOTE: The field order in this table must exactly match the order in
 *       gasDecodeTable (decode.c). Mask calculations rely on this.
 *
 ***********************************************************************
 */
static MASK_B2S_TABLE gasCmpMaskTable[] =
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
  { "md5",        1 },
  { "sha1",       1 },
  { "sha256",     1 },
  { "magic",      1 },
};

#ifdef WIN32
static MASK_B2S_TABLE gasMapMaskTable[] =
{
  { "volume",     1 },
  { "findex",     1 },
  { "attributes", 1 },
  { "atime",      1 },
  { "mtime",      1 },
  { "ctime",      1 },
  { "chtime",     1 },
  { "size",       1 },
  { "altstreams", 1 },
  { "md5",        1 },
  { "sha1",       1 },
  { "sha256",     1 },
  { "magic",      1 },
};
#else
static MASK_B2S_TABLE gasMapMaskTable[] =
{
  { "dev",        1 },
  { "inode",      1 },
  { "mode",       1 },
  { "nlink",      1 },
  { "uid",        1 },
  { "gid",        1 },
  { "rdev",       1 },
  { "atime",      1 },
  { "mtime",      1 },
  { "ctime",      1 },
  { "size",       1 },
  { "md5",        1 },
  { "sha1",       1 },
  { "sha256",     1 },
  { "magic",      1 },
};
#endif

/*-
 ***********************************************************************
 *
 * MaskBuildMask
 *
 ***********************************************************************
 */
char *
MaskBuildMask(unsigned long ulMask, int iType, char *pcError)
{
  const char          acRoutine[] = "MaskBuildMask()";
  char               *pcMask = NULL;
  int                 i = 0;
  int                 iCount = 0;
  int                 iIndex = 0;
  int                 iMaskSize = 0;
  int                 iMaskTableLength = MaskGetTableLength(iType);
  int                 iNLeft = 0;
  MASK_B2S_TABLE     *pasMaskTable = MaskGetTableReference(iType);
  unsigned long       ulTestBit = 0;

  /*-
   *********************************************************************
   *
   * Determine what type of mask we're dealing with.
   *
   *********************************************************************
   */
  switch (iType)
  {
  case MASK_RUNMODE_TYPE_CMP:
  case MASK_RUNMODE_TYPE_DIG:
  case MASK_RUNMODE_TYPE_MAP:
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type [%d]. That shouldn't happen.", acRoutine, iType);
    return NULL;
    break;
  }

  /*-
   *********************************************************************
   *
   * Allocate enough memory to hold the mask converted into a string.
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  iMaskSize = iNLeft = strlen("none") + ((strlen("+") + MASK_NAME_SIZE) * iMaskTableLength) + 1;
  pcMask = calloc(iMaskSize, 1);
  if (pcMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Start with the mask's anchor.
   *
   *********************************************************************
   */
  iCount = snprintf(&pcMask[iIndex], iNLeft, "none");
  iIndex += iCount;
  iNLeft -= iCount;

  /*-
   *********************************************************************
   *
   * Loop through the mask table adding a name only when the number of
   * bits to set is greater than zero. If the number is zero, it means
   * the field is (effectively) private.
   *
   *********************************************************************
   */
  for (i = 0; i < iMaskTableLength; i++)
  {
    ulTestBit = 1 << i;
    if (MASK_BIT_IS_SET(ulMask, ulTestBit) && pasMaskTable[i].iBits2Set > 0)
    {
      iCount = snprintf(&pcMask[iIndex], iNLeft, "+%s", pasMaskTable[i].acName);
      iIndex += iCount;
      iNLeft -= iCount;
    }
  }

  return pcMask;
}

/*-
 ***********************************************************************
 *
 * MaskFreeMask
 *
 ***********************************************************************
 */
void
MaskFreeMask(MASK_USS_MASK *psMask)
{
  if (psMask != NULL)
  {
    if (psMask->pcMask != NULL)
    {
      free(psMask->pcMask);
    }
    free(psMask);
  }
}


/*-
 ***********************************************************************
 *
 * MaskGetTableLength
 *
 ***********************************************************************
 */
int
MaskGetTableLength(int iType)
{
  switch (iType)
  {
  case MASK_RUNMODE_TYPE_CMP:
    return (sizeof(gasCmpMaskTable) / sizeof(gasCmpMaskTable[0]));
    break;
  case MASK_RUNMODE_TYPE_DIG:
    return 0;
    break;
  case MASK_RUNMODE_TYPE_MAP:
    return (sizeof(gasMapMaskTable) / sizeof(gasMapMaskTable[0]));
    break;
  default:
    return 0;
    break;
  }
}


/*-
 ***********************************************************************
 *
 * MaskGetTableReference
 *
 ***********************************************************************
 */
MASK_B2S_TABLE *
MaskGetTableReference(int iType)
{
  switch (iType)
  {
  case MASK_RUNMODE_TYPE_CMP:
    return gasCmpMaskTable;
    break;
  case MASK_RUNMODE_TYPE_DIG:
    return NULL;
    break;
  case MASK_RUNMODE_TYPE_MAP:
    return gasMapMaskTable;
    break;
  default:
    return NULL;
    break;
  }
}


/*-
 ***********************************************************************
 *
 * MaskNewMask
 *
 ***********************************************************************
 */
MASK_USS_MASK *
MaskNewMask(char *pcError)
{
  const char          acRoutine[] = "MaskNewMask()";
  MASK_USS_MASK      *psMask = NULL;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  psMask = (MASK_USS_MASK *) calloc(sizeof(MASK_USS_MASK), 1);
  if (psMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  return psMask;
}


/*-
 ***********************************************************************
 *
 * MaskParseMask
 *
 ***********************************************************************
 */
MASK_USS_MASK *
MaskParseMask(char *pcMask, int iType, char *pcError)
{
  const char          acRoutine[] = "MaskParseMask()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                cLastAction = 0;
  char                cNextAction = 0;
  char               *pcTemp = NULL;
  char               *pcTempLine = NULL;
  char               *pcToken = NULL;
  int                 i = 0;
  int                 j = 0;
  int                 iDone = 0;
  int                 iError = 0;
  int                 iLength = strlen(pcMask);
  int                 iMaskTableLength = MaskGetTableLength(iType);
  int                 iOffset = 0;
  MASK_B2S_TABLE     *pasMaskTable = MaskGetTableReference(iType);
  MASK_USS_MASK      *psMask = NULL;
  unsigned long       ulAllMask = 0;

  /*-
   *********************************************************************
   *
   * Sanity check the type and intialize variables.
   *
   *********************************************************************
   */
  switch (iType)
  {
  case MASK_RUNMODE_TYPE_CMP:
    ulAllMask = CMP_ALL_MASK;
    break;
  case MASK_RUNMODE_TYPE_DIG:
    ulAllMask = DIG_ALL_MASK;
    break;
  case MASK_RUNMODE_TYPE_MAP:
    ulAllMask = MAP_ALL_MASK;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type [%d]. That shouldn't happen.", acRoutine, iType);
    return NULL;
    break;
  }

  /*-
   *********************************************************************
   *
   * Allocate and initialize memory for the mask.
   *
   *********************************************************************
   */
  psMask = MaskNewMask(acLocalError);
  if (psMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the user-supplied string mask.
   *
   *********************************************************************
   */
  iError = MaskSetMask(psMask, pcMask, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MaskFreeMask(psMask);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Check that the mask begins with a valid anchor.
   *
   *********************************************************************
   */
  if (strncasecmp(pcMask, "all", 3) == 0)
  {
    psMask->ulMask = ~0;
    iLength = 3;
  }
  else if (strncasecmp(pcMask, "none", 4) == 0)
  {
    psMask->ulMask = 0;
    iLength = 4;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Prefix = [%s] != [all|none]: Invalid prefix.", acRoutine, pcMask);
    MaskFreeMask(psMask);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Continue parsing the mask. If there are no more fields, we're done.
   *
   *********************************************************************
   */
  switch (pcMask[iLength])
  {
  case '+':
  case '-':
    cLastAction = '?';
    cNextAction = pcMask[iLength++];
    break;
  case 0:
    psMask->ulMask &= ulAllMask;
    if (iType == MASK_RUNMODE_TYPE_CMP && psMask->ulMask == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: CompareMask = [%s]: Mask must include at least one field (e.g., none+md5).", acRoutine, psMask->pcMask);
      MaskFreeMask(psMask);
      return NULL;
    }
    return psMask;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Operator = [%c] != [+|-]: Invalid operator.", acRoutine, pcMask[iLength]);
    MaskFreeMask(psMask);
    return NULL;
    break;
  }

  /*-
   *********************************************************************
   *
   * Copy the remainder of the input to a scratch pad.
   *
   *********************************************************************
   */
  pcTemp = &pcMask[iLength];
  iLength = strlen(pcTemp); /* Calculate new length. */
  pcTempLine = calloc(iLength + 1, 1);
  if (pcTempLine == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    MaskFreeMask(psMask);
    return NULL;
  }
  strncpy(pcTempLine, pcTemp, iLength + 1);

  /*-
   *********************************************************************
   *
   * Remove EOL characters.
   *
   *********************************************************************
   */
  SupportChopEOLs(pcTempLine, 0, NULL);

  /*-
   *********************************************************************
   *
   * Scan through the string looking for tokens delimited by '+', '-',
   * or 0.
   *
   *********************************************************************
   */
  for (pcToken = pcTempLine, iOffset = 0, iDone = 0; !iDone;)
  {
    if (pcTempLine[iOffset] == '+' || pcTempLine[iOffset] == '-' || pcTempLine[iOffset] == 0)
    {
      if (pcTempLine[iOffset] == 0)
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
      cNextAction = pcTempLine[iOffset];

      /*-
       *****************************************************************
       *
       * Terminate the token.
       *
       *****************************************************************
       */
      pcTempLine[iOffset] = 0;

      /*-
       *****************************************************************
       *
       * Scan the table looking for this token. Add or subtract the
       * expanded token value (i.e., the time tokens count for more
       * than one mask bit each) from the mask depending on whether
       * '+' or '-' was given.
       *
       *****************************************************************
       */
      for (i = 0; i < iMaskTableLength; i++)
      {
        if (strcasecmp(pcToken, pasMaskTable[i].acName) == 0 && pasMaskTable[i].iBits2Set > 0)
        {
          for (j = 0; j < pasMaskTable[i].iBits2Set; j++)
          {
            PUTBIT(psMask->ulMask, ((cLastAction == '+') ? 1 : 0), (i + j));
          }
          break;
        }
      }
      if (i == iMaskTableLength)
      {
        /*-
         ***************************************************************
         *
         * Check to see if this is a group field before aborting.
         *
         ***************************************************************
         */
        if (strcasecmp(pcToken, "hashes") == 0 && (iType == MASK_RUNMODE_TYPE_CMP || iType == MASK_RUNMODE_TYPE_MAP))
        {
          if (cLastAction == '+')
          {
            psMask->ulMask |= (iType == MASK_RUNMODE_TYPE_CMP) ? CMP_HASHES_MASK : MAP_HASHES_MASK;
          }
          else
          {
            psMask->ulMask &= (iType == MASK_RUNMODE_TYPE_CMP) ? ~CMP_HASHES_MASK : ~MAP_HASHES_MASK;
          }
        }
        else if (strcasecmp(pcToken, "times") == 0 && (iType == MASK_RUNMODE_TYPE_CMP || iType == MASK_RUNMODE_TYPE_MAP))
        {
          if (cLastAction == '+')
          {
            psMask->ulMask |= (iType == MASK_RUNMODE_TYPE_CMP) ? CMP_TIMES_MASK : MAP_TIMES_MASK;
          }
          else
          {
            psMask->ulMask &= (iType == MASK_RUNMODE_TYPE_CMP) ? ~CMP_TIMES_MASK : ~MAP_TIMES_MASK;
          }
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Token = [%c%s]: Invalid value.", acRoutine, cLastAction, pcToken);
          MaskFreeMask(psMask);
          return NULL;
        }
      }
      if (!iDone)
      {
        iOffset++;
        pcToken = &pcTempLine[iOffset];
      }
    }
    else
    {
      iOffset++;
    }
  }

  /*-
   *********************************************************************
   *
   * Remove any extra bits, and send it back to the caller.
   *
   *********************************************************************
   */
  psMask->ulMask &= ulAllMask;

  return psMask;
}


/*-
 ***********************************************************************
 *
 * MaskSetDynamicString
 *
 ***********************************************************************
 */
int
MaskSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          acRoutine[] = "MaskSetDynamicString()";
  char               *pcTempValue = NULL;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Allocate additional memory as required. Preserve the user-supplied
   * pointer in case the operation fails. Then, copy the new value into
   * place. The caller is expected to free this memory.
   *
   *********************************************************************
   */
  iLength = strlen(pcNewValue);
  pcTempValue = realloc(*ppcValue, iLength + 1);
  if (pcTempValue == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
    return ER;
  }
  strncpy(pcTempValue, pcNewValue, iLength + 1);
  *ppcValue = pcTempValue;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MaskSetMask
 *
 ***********************************************************************
 */
int
MaskSetMask(MASK_USS_MASK *psMask, char *pcMask, char *pcError)
{
  const char          acRoutine[] = "MaskSetMask()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;

  if (psMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined mask.", acRoutine);
    return ER;
  }

  iError = MaskSetDynamicString(&psMask->pcMask, pcMask, acLocalError);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  return ER_OK;
}
