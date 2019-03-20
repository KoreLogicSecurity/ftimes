/*-
 ***********************************************************************
 *
 * $Id: rtimes.c,v 1.22 2013/02/14 16:55:23 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2008-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Global Variables.
 *
 ***********************************************************************
 */
RTIMES_PROPERTIES *gpsProperties = NULL;

RTIMES_TODO_LIST gsTodoList[] =
{
  { HKEY_CLASSES_ROOT, _T("HKEY_CLASSES_ROOT"), _T("") },
  { HKEY_LOCAL_MACHINE, _T("HKEY_LOCAL_MACHINE"), _T("") },
  { HKEY_USERS, _T("HKEY_USERS"), _T("") },
  { HKEY_CURRENT_USER, _T("HKEY_CURRENT_USER"), _T("") },
  { HKEY_CURRENT_CONFIG, _T("HKEY_CURRENT_CONFIG"), _T("") },
};
DWORD gdwTodoSize = sizeof(gsTodoList)/sizeof(gsTodoList[0]);

DWORD gdwDataLimit = 0;

/*-
 ***********************************************************************
 *
 * RTimesMain
 *
 ***********************************************************************
 */
int
_tmain(int iArgumentCount, TCHAR *pptcArgumentVector[])
{
  int iError = 0;
  RTIMES_PROPERTIES *psProperties = NULL;
  TCHAR atcLocalError[MESSAGE_SIZE];

  /*-
   *********************************************************************
   *
   * Punch in and go to work.
   *
   *********************************************************************
   */
  iError = RTimesBootStrap(atcLocalError);
  if (iError != ER_OK)
  {
    _ftprintf(stderr, _T("%s: main(): %s\n"), _T(PROGRAM), atcLocalError);
    return XER_BootStrap;
  }
  psProperties = RTimesGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Process command line arguments.
   *
   *********************************************************************
   */
  iError = RTimesProcessArguments(iArgumentCount, pptcArgumentVector, psProperties, atcLocalError);
  if (iError != ER_OK)
  {
    _ftprintf(stderr, _T("%s: main(): %s\n"), _T(PROGRAM), atcLocalError);
    return XER_ProcessArguments;
  }

  /*-
   *********************************************************************
   *
   * Do some work.
   *
   *********************************************************************
   */
  iError = RTimesWorkHorse(psProperties, atcLocalError);
  if (iError != ER_OK)
  {
    _ftprintf(stderr, _T("%s: main(): %s\n"), _T(PROGRAM), atcLocalError);
    return XER_WorkHorse;
  }

  /*-
   *********************************************************************
   *
   * Shutdown and go home.
   *
   *********************************************************************
   */
  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * FormatWinxError
 *
 ***********************************************************************
 */
void
FormatWinxError(DWORD dwError, TCHAR **pptcMessage)
{
  static TCHAR        atcMessage[MESSAGE_SIZE / 2];
  int                 i = 0;
  int                 j = 0;
  LPVOID              lpvMessage = NULL;
  TCHAR              *ptc = NULL;

  /*-
   *********************************************************************
   *
   * Initialize the result buffer/pointer.
   *
   *********************************************************************
   */
  atcMessage[0] = 0;

  *pptcMessage = atcMessage;

  /*-
   *********************************************************************
   *
   * Format the message that corresponds to the specified error code.
   *
   *********************************************************************
   */
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dwError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* default language */
    (LPTSTR) &lpvMessage,
    0,
    NULL
    );

  /*-
   *********************************************************************
   *
   * Replace linefeeds with spaces. Eliminate carriage returns.
   *
   *********************************************************************
   */
  for (i = 0, j = 0, ptc = (TCHAR *) lpvMessage; (i < (int) _tcslen(lpvMessage)) && (i < (MESSAGE_SIZE / 2)); i++, j++)
  {
    if (ptc[i] == _T('\n'))
    {
      atcMessage[j] = _T(' ');
    }
    else if (ptc[i] == _T('\r'))
    {
      j--;
      continue;
    }
    else
    {
      atcMessage[j] = ptc[i];
    }
  }
  atcMessage[j] = 0;

  /*-
   *********************************************************************
   *
   * Removing trailing spaces.
   *
   *********************************************************************
   */
  while (j > 1 && atcMessage[j - 1] == _T(' '))
  {
    atcMessage[--j] = 0;
  }

  LocalFree(lpvMessage);
}


/*-
 ***********************************************************************
 *
 * RTimesBootStrap
 *
 ***********************************************************************
 */
int
RTimesBootStrap(TCHAR *ptcError)
{
  TCHAR               atcLocalError[MESSAGE_SIZE] = { 0 };
  RTIMES_PROPERTIES  *psProperties;

#ifdef WIN32
  /*-
   *********************************************************************
   *
   * Suppress critical-error-handler message boxes.
   *
   *********************************************************************
   */
  SetErrorMode(SEM_FAILCRITICALERRORS);
#endif

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new properties structure.
   *
   *********************************************************************
   */
  psProperties = RTimesNewProperties(atcLocalError);
  if (psProperties == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesBootStrap(): %s"), atcLocalError);
    return ER;
  }
  RTimesSetPropertiesReference(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesEnumerateKeys
 *
 ***********************************************************************
 */
int
RTimesEnumerateKeys(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError)
{
  DWORD dwIndex = 0;
  DWORD dwKeyNameSize = RTIMES_KEYNAME_SIZE;
  DWORD dwReturnCode = 0;
  HKEY hKeyNew = { 0 };
  int iDone = 0;
  int iError = 0;
  TCHAR atcKeyName[RTIMES_KEYNAME_SIZE];
  TCHAR atcLocalError[MESSAGE_SIZE];
  TCHAR *ptcNewPath = NULL;
  TCHAR *ptcWinxError = NULL;

  for (dwIndex = 0; iDone == 0; dwIndex++)
  {
    dwKeyNameSize = RTIMES_KEYNAME_SIZE; /* Need to reset the size each time through. */
    atcKeyName[0] = 0;
    dwReturnCode = RegEnumKeyEx(
      hKey,
      dwIndex,
      atcKeyName,
      &dwKeyNameSize,
      0,
      NULL,
      NULL,
      NULL
      );
    if (dwReturnCode != ERROR_SUCCESS)
    {
      if (dwReturnCode == ERROR_NO_MORE_ITEMS)
      {
        iDone = 1;
        continue;
      }
      else
      {
        FormatWinxError(dwReturnCode, &ptcWinxError);
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateKeys(): RegEnumKeyEx(): %s"), ptcWinxError);
        return ER;
      }
    }

    ptcNewPath = RTimesNewPath(ptcPath, atcKeyName, _T("\\"), atcLocalError);
    if (ptcNewPath == NULL)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateKeys(): %s"), atcLocalError);
      return ER;
    }

    dwReturnCode = RegOpenKeyEx(
      hKey,
      atcKeyName,
      0,
      KEY_READ,
      &hKeyNew
      );
    if (dwReturnCode != ERROR_SUCCESS)
    {
      if (dwReturnCode == ERROR_FILE_NOT_FOUND || dwReturnCode == ERROR_ACCESS_DENIED)
      {
//FIXME Log it ...
        FormatWinxError(dwReturnCode, &ptcWinxError);
        _ftprintf(stderr, _T("%s: Main(): RTimesWorkHorse(): RTimesMapKey(%s): RTimesEnumerateKeys(): RegOpenKeyEx(%s): %s\n"), _T(PROGRAM), ptcPath, atcKeyName, ptcWinxError);
        continue;
      }
      else
      {
        FormatWinxError(dwReturnCode, &ptcWinxError);
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateKeys(): RegOpenKeyEx(%s): %s"), atcKeyName, ptcWinxError);
        return ER;
      }
      continue;
    }
    iError = RTimesMapKey(pFile, hKeyNew, ptcNewPath, atcLocalError);
    if (iError != ER_OK)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateKeys(): %s"), atcLocalError);
    }
    free(ptcNewPath);
    RegCloseKey(hKeyNew);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesEnumerateValues
 *
 ***********************************************************************
 */
int
RTimesEnumerateValues(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError)
{
  static BYTE *pbyteData = NULL;
  static DWORD dwFullDataSize = 0;
  static DWORD dwFullValueNameSize = 0;
  static TCHAR *ptcValueName = NULL;
  DWORD dwDataSize = 0;
  DWORD dwFlags = 0;
  DWORD dwType = 0;
  DWORD dwReturnCode = 0;
  DWORD dwValueNameSize = 0;
  DWORD dwIndex = 0;
  int iDone = 0;
  int iTries = 0;
  TCHAR *ptcWinxError = NULL;

  /*-
   *********************************************************************
   *
   * Initialize some memory on the first go 'round.
   *
   *********************************************************************
   */
  if (pbyteData == NULL)
  {
    ptcValueName = (TCHAR *) malloc((RTIMES_DEFAULT_NAME_SIZE * sizeof(TCHAR)));
    if (ptcValueName == NULL)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateValues(): malloc(): %s"), strerror(errno));
      return ER;
    }
    dwFullValueNameSize = RTIMES_DEFAULT_NAME_SIZE; /* This size is measured in characters, not bytes. */
    pbyteData = (BYTE *) malloc(RTIMES_DEFAULT_DATA_SIZE);
    if (pbyteData == NULL)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateValues(): malloc(): %s"), strerror(errno));
      return ER;
    }
    dwFullDataSize = RTIMES_DEFAULT_DATA_SIZE;
  }

  /*-
   *********************************************************************
   *
   * Loop over the values. The "Registry Element Size Limits" man page
   * states that value names can be 16,383 characters long. On Windows
   * 2K platforms, names can be 260 ANSI or 16,383 Unicode characters.
   *
   * NOTE: The "RegEnumValue Function" man page states:
   *
   *   Registry value names are limited to 32,767 bytes. The ANSI
   *   version of this function treats this param as a USHORT value.
   *   Therefore, if you specify a value greater than 32,767 bytes,
   *   there is an overflow and the function may return
   *   ERROR_MORE_DATA.
   *
   *********************************************************************
   */
  for (dwIndex = 0; iDone == 0; dwIndex++)
  {
    iTries = 0;
    dwFlags = RTIMES_FLAG_VALUE;
    dwValueNameSize = dwFullValueNameSize; /* Need to reset the size each time through. */
    dwDataSize = dwFullDataSize; /* Need to reset the size each time through. */
    ptcValueName[0] = 0;
RTIMES_DO_OVER:
    dwReturnCode = RegEnumValue(
      hKey,
      dwIndex,
      ptcValueName,
      &dwValueNameSize,
      0,
      &dwType,
      pbyteData,
      &dwDataSize
      );
    if (dwReturnCode != ERROR_SUCCESS)
    {
      if (dwReturnCode == ERROR_NO_MORE_ITEMS)
      {
        iDone = 1;
        continue;
      }
      else if (dwReturnCode == ERROR_MORE_DATA)
      {
        if (dwValueNameSize < dwFullValueNameSize && dwDataSize < dwFullDataSize)
        {
          _ftprintf(stderr, _T("%s: RTimesEnumerateValues(): The current name/data buffer sizes (%d/%d) are adequate, yet ERROR_MORE_DATA was returned wanting %d/%d bytes, respectively. That should not happen!\n"), _T(PROGRAM), (int) dwFullValueNameSize, (int) dwFullDataSize, (int) dwValueNameSize, (int) dwDataSize);
          exit(XER_Abort);
        }
        if (iTries++ <= RTIMES_MAX_DO_OVERS) /* Prevent an endless loop. */
        {
          int iDoOver = 0;
          if (dwValueNameSize > dwFullValueNameSize)
          {
            if (dwValueNameSize > 16383) /* Don't exceed the limit documented above. */
            {
              dwFlags |= RTIMES_FLAG_MANDATORY_NAME_TRUNCATE;
              ptcValueName[dwFullValueNameSize] = 0;
            }
            else
            {
              TCHAR *ptcTemp = (TCHAR *) realloc(ptcValueName, (dwValueNameSize * sizeof(TCHAR)));
              if (ptcTemp == NULL)
              {
                dwFlags |= RTIMES_FLAG_MANDATORY_NAME_TRUNCATE;
                ptcValueName[dwFullValueNameSize] = 0;
              }
              else
              {
                ptcValueName = ptcTemp;
                dwFullValueNameSize = dwValueNameSize;
                iDoOver = 1;
              }
            }
          }
          if (dwDataSize > dwFullDataSize)
          {
            BYTE *pbyteTemp = (BYTE *) realloc(pbyteData, dwDataSize);
            if (pbyteTemp == NULL)
            {
              dwFlags |= RTIMES_FLAG_MANDATORY_TRUNCATE;
            }
            else
            {
              pbyteData = pbyteTemp;
              dwFullDataSize = dwDataSize;
              iDoOver = 1;
            }
          }
          if (iDoOver)
          {
            goto RTIMES_DO_OVER;
          }
        }
        else
        {
          dwFlags |= RTIMES_FLAG_MANDATORY_TRUNCATE; /* Too many tries. Set the truncated flag and keep going. */
        }
      }
      else
      {
        FormatWinxError(dwReturnCode, &ptcWinxError);
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesEnumerateValues(): RegEnumValue(%s): %s"), ptcValueName, ptcWinxError);
        return ER;
      }
    }
    RTimesWriteValueRecord(pFile, dwFlags, dwType, pbyteData, dwDataSize, ptcPath, ptcValueName);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesGetPropertiesReference
 *
 ***********************************************************************
 */
RTIMES_PROPERTIES *
RTimesGetPropertiesReference(void)
{
  return gpsProperties;
}


/*-
 ***********************************************************************
 *
 * RTimesMapKey
 *
 ***********************************************************************
 */
int
RTimesMapKey(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError)
{
  static TCHAR *ptcClass = NULL;
  static DWORD dwFullClassSize = 0;
  DWORD dwClassSize = 0;
  DWORD dwFlags = RTIMES_FLAG_KEY;
  DWORD dwReturnCode = 0;
  FILETIME sLastWriteTime = { 0 };
  int iError = 0;
  int iTries = 0;
  SECURITY_DESCRIPTOR *psSd = NULL;
  SID *psSidOwner = NULL;
  SID *psSidGroup = NULL;
  TCHAR atcLocalError[MESSAGE_SIZE];
  TCHAR atcTime[RTIMES_TIME_FORMAT_LENGTH];
  TCHAR *ptcAclDacl = NULL;
  TCHAR *ptcNeuteredClass = NULL;
  TCHAR *ptcNeuteredPath = NULL;
  TCHAR *ptcSidGroup = NULL;
  TCHAR *ptcSidOwner = NULL;
  TCHAR *ptcWinxError = NULL;

  /*-
   *********************************************************************
   *
   * Initialize some memory on the first go 'round.
   *
   *********************************************************************
   */
  if (ptcClass == NULL)
  {
    ptcClass = (TCHAR *) malloc(RTIMES_DEFAULT_CLASS_STRING_SIZE);
    if (ptcClass == NULL)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesMapKey(%s): malloc(): %s"), ptcPath, strerror(errno));
      return ER;
    }
    dwFullClassSize = RTIMES_DEFAULT_CLASS_STRING_SIZE;
  }

  /*-
   *********************************************************************
   *
   * Run a query to get various attributes for the specified key.
   *
   *********************************************************************
   */
RTIMES_DO_OVER:
  dwClassSize = dwFullClassSize;
  dwReturnCode = RegQueryInfoKey(
    hKey,            // __in         HKEY hKey,
    ptcClass,        // __out        LPTSTR lpClass,
    &dwClassSize,    // __inout_opt  LPDWORD lpcClass,
    0,               // __reserved   LPDWORD lpReserved,
    NULL,            // __out_opt    LPDWORD lpcSubKeys,
    NULL,            // __out_opt    LPDWORD lpcMaxSubKeyLen,
    NULL,            // __out_opt    LPDWORD lpcMaxClassLen,
    NULL,            // __out_opt    LPDWORD lpcValues,
    NULL,            // __out_opt    LPDWORD lpcMaxValueNameLen,
    NULL,            // __out_opt    LPDWORD lpcMaxValueLen,
    NULL,            // __out_opt    LPDWORD lpcbSecurityDescriptor,
    &sLastWriteTime  // __out_opt    PFILETIME lpftLastWriteTime
    );
  if (dwReturnCode != ERROR_SUCCESS)
  {
    if (dwReturnCode == ERROR_MORE_DATA)
    {
      DWORD dw = dwFullClassSize << 1;
      if (iTries++ <= RTIMES_MAX_DO_OVERS) /* Prevent an endless loop. */
      {
        TCHAR *ptcTemp = (TCHAR *) realloc(ptcClass, dw);
        if (ptcTemp == NULL)
        {
          dwFlags |= RTIMES_FLAG_MANDATORY_TRUNCATE;
        }
        else
        {
          ptcClass = ptcTemp;
          dwFullClassSize = dw;
          goto RTIMES_DO_OVER;
        }
      }
      else
      {
        dwFlags |= RTIMES_FLAG_MANDATORY_TRUNCATE; /* Too many tries. Set the truncated flag and keep going. */
      }
    }
    else
    {
//FIXME Log it ...
      FormatWinxError(dwReturnCode, &ptcWinxError);
      _ftprintf(stderr, _T("%s: Main(): RTimesWorkHorse(): RTimesMapKey(%s): RegQueryInfoKey(): %s\n"), _T(PROGRAM), ptcPath, ptcWinxError);
      if (dwReturnCode == ERROR_ACCESS_DENIED)
      {
//FIXME
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Harvest security information (owner/group SIDs and DACL).
   *
   *********************************************************************
   */
  dwReturnCode = GetSecurityInfo(
    hKey,
    SE_REGISTRY_KEY,
    OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
    (PSID) &psSidOwner,
    (PSID) &psSidGroup,
    NULL, /* This pointer is not required to obtain DACL information. */
    NULL,
    &psSd
    );
  if (dwReturnCode != ERROR_SUCCESS)
  {
    FormatWinxError(dwReturnCode, &ptcWinxError);
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesMapKey(%s): GetSecurityInfo(): %s"), ptcPath, ptcWinxError);
    return ER;
  }
  if (!IsValidSecurityDescriptor(psSd))
  {
    dwFlags |= RTIMES_FLAG_OWNER_INVALID | RTIMES_FLAG_GROUP_MISSING | RTIMES_FLAG_DACL_INVALID;
  }
  else
  {
#ifdef USE_SDDL
    ConvertSidToStringSid(psSidOwner, &ptcSidOwner);
#else
    ptcSidOwner = RTimesSidToString(psSidOwner, atcLocalError);
#endif
    if (ptcSidOwner == NULL)
    {
      dwFlags |= RTIMES_FLAG_OWNER_MISSING;
    }
#ifdef USE_SDDL
    ConvertSidToStringSid(psSidGroup, &ptcSidGroup);
#else
    ptcSidGroup = RTimesSidToString(psSidGroup, atcLocalError);
#endif
    if (ptcSidGroup == NULL)
    {
      dwFlags |= RTIMES_FLAG_GROUP_MISSING;
    }
#ifdef USE_SDDL
    ConvertSecurityDescriptorToStringSecurityDescriptor(psSd, SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &ptcAclDacl, NULL);
    if (ptcAclDacl == NULL)
    {
      dwFlags |= RTIMES_FLAG_DACL_MISSING;
    }
#else
    dwFlags |= RTIMES_FLAG_DACL_OMITTED;
#endif
  }

  /*-
   *********************************************************************
   *
   * Neuter the path.
   *
   *********************************************************************
   */
  ptcNeuteredPath = RTimesNeuterEncodeData((unsigned char *) ptcPath, _tcslen(ptcPath), NEUTER_ENCODING_RTIMES, atcLocalError);
  if (ptcNeuteredPath == NULL)
  {
//FIXME Log it ...
    _ftprintf(stderr, _T("%s: Error encoding the specified path: %s\n"), _T(PROGRAM), atcLocalError);
    LocalFree(ptcSidOwner);
    LocalFree(ptcSidGroup);
#ifdef USE_SDDL
    LocalFree(ptcAclDacl);
#endif
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Neuter the class.
   *
   *********************************************************************
   */
  ptcNeuteredClass = RTimesNeuterEncodeData((unsigned char *) ptcClass, _tcslen(ptcClass), NEUTER_ENCODING_RTIMES, atcLocalError);
  if (ptcNeuteredClass == NULL)
  {
    dwFlags |= RTIMES_FLAG_CLASS_MISSING;
//FIXME Log it ...
    _ftprintf(stderr, _T("%s: Error encoding the specified class: %s\n"), _T(PROGRAM), atcLocalError);
  }

  /*-
   *********************************************************************
   *
   * Convert the wtime to a string.
   *
   *********************************************************************
   */
  iError = RTimesWTimeToString(&sLastWriteTime, atcTime);
  if (iError != ER_OK)
  {
    dwFlags |= RTIMES_FLAG_WTIME_MISSING;
  }

  /*-
   *********************************************************************
   *
   * Write a record. Format:
   *
   *   name|flags|osid|gsid|dacl|class|wtime|type|size|md5|data
   *
   *********************************************************************
   */
//FIXME Check return value...
  _ftprintf(pFile, _T("\"%s\"|0x%x|%s|%s|%s|%s|%s||||\n"),
    ptcNeuteredPath,
    (unsigned int) dwFlags,
    (ptcSidOwner) ? ptcSidOwner : "",
    (ptcSidGroup) ? ptcSidGroup : "",
    (ptcAclDacl) ? ptcAclDacl : "",
    (ptcNeuteredClass) ? ptcNeuteredClass : "",
    atcTime
    );

  /*-
   *********************************************************************
   *
   * Enumerate values for this key.
   *
   *********************************************************************
   */
  iError = RTimesEnumerateValues(pFile, hKey, ptcPath, atcLocalError);
  if (iError != ER_OK)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesMapKey(%s): %s"), ptcPath, atcLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Enumerate subkeys for this key.
   *
   *********************************************************************
   */
  iError = RTimesEnumerateKeys(pFile, hKey, ptcPath, atcLocalError);
  if (iError != ER_OK)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesMapKey(%s): %s"), ptcPath, atcLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Cleanup and return.
   *
   *********************************************************************
   */
  free(ptcNeuteredPath);
  LocalFree(ptcSidOwner);
  LocalFree(ptcSidGroup);
#ifdef USE_SDDL
  LocalFree(ptcAclDacl);
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesMd5ToHex
 *
 ***********************************************************************
 */
int
RTimesMd5ToHex(unsigned char *pucMd5, TCHAR *ptcHexHash)
{
  int n = 0;
  unsigned int *pui = (unsigned int *) pucMd5;

  n += _sntprintf(&ptcHexHash[n], ((MD5_HASH_SIZE * 2 + 1) * sizeof(TCHAR)), _T("%08x%08x%08x%08x"),
    (unsigned int) htonl(*(pui    )),
    (unsigned int) htonl(*(pui + 1)),
    (unsigned int) htonl(*(pui + 2)),
    (unsigned int) htonl(*(pui + 3))
    );
  ptcHexHash[n] = 0;

  return n;
}


/*-
 ***********************************************************************
 *
 * RTimesNeuterEncodeData
 *
 ***********************************************************************
 */
TCHAR *
RTimesNeuterEncodeData(unsigned char *pucData, int iLength, int iOptions, TCHAR *ptcError)
{
  int i = 0;
  int n = 0;
  TCHAR *ptcNeutered = NULL;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  ptcNeutered = (TCHAR *) malloc(((3 * iLength) + 1) * sizeof(TCHAR));
  if (ptcNeutered == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("%s"), strerror(errno));
    return NULL;
  }
  ptcNeutered[0] = 0;

  if ((iOptions & NEUTER_ENCODING_FULL) == NEUTER_ENCODING_FULL)
  {
    for (i = n = 0; i < iLength; i++)
    {
      n += _stprintf(&ptcNeutered[n], _T("%%%02x"), (unsigned char) pucData[i]);
    }
    ptcNeutered[n] = 0;
    return ptcNeutered;
  }

  /*-
   *********************************************************************
   *
   * Neuter non-printables and [|"'`%+#]. Convert spaces to '+'. Avoid
   * isprint() here because it has led to unexpected results on WINX
   * platforms. In the past, isprint() on certain WINX systems has
   * decided that several characters in the range 0x7f - 0xff are
   * printable.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    if (pucData[i] > _T('~') || pucData[i] < _T(' '))
    {
      n += _stprintf(&ptcNeutered[n], _T("%%%02x"), (unsigned char) pucData[i]);
    }
    else
    {
      switch (pucData[i])
      {
      case _T('|'):
      case _T('"'):
      case _T('\''):
      case _T('`'):
      case _T('%'):
      case _T('+'):
      case _T('#'):
        n += _stprintf(&ptcNeutered[n], _T("%%%02x"), (unsigned char) pucData[i]);
        break;
      case _T(' '):
        ptcNeutered[n++] = _T('+');
        break;
      default:
        ptcNeutered[n++] = pucData[i];
        break;
      }
    }
  }
  ptcNeutered[n] = 0;

  return ptcNeutered;
}


/*-
 ***********************************************************************
 *
 * RTimesNewPath
 *
 ***********************************************************************
 */
TCHAR *
RTimesNewPath(TCHAR *ptcPath, TCHAR *ptcName, TCHAR *ptcDelimiter, TCHAR *ptcError)
{
  int iPathSize = 0;
  TCHAR *ptcNewPath = NULL;

  /*-
   *********************************************************************
   *
   * Compute and allocate memory to hold the new path.
   *
   *********************************************************************
   */
  if (ptcPath != NULL)
  {
    iPathSize = _tcslen(ptcPath);
  }
  if (ptcName != NULL)
  {
    iPathSize += _tcslen(ptcName);
  }
  if (ptcDelimiter != NULL)
  {
    iPathSize += _tcslen(ptcDelimiter);
  }
  iPathSize++; /* Add one for the terminator. */
  ptcNewPath = malloc(iPathSize);
  if (ptcNewPath == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesNewPath(): malloc(): %s"), strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * If the path is NULL, just use the name (if any). If the name is
   * NULL, just use the path (if any). If neither is NULL, combine
   * them using the delimiter (if any). Depending on the inputs, the
   * output may be an empty string.
   *
   *********************************************************************
   */
  if (ptcPath == NULL)
  {
    _sntprintf(ptcNewPath, iPathSize, _T("%s"), (ptcName) ? ptcName : _T(""));
  }
  else if (ptcName == NULL)
  {
    _sntprintf(ptcNewPath, iPathSize, _T("%s"), (ptcPath) ? ptcPath : _T(""));
  }
  else
  {
    _sntprintf(ptcNewPath, iPathSize, _T("%s%s%s"), ptcPath, (ptcDelimiter) ? ptcDelimiter : _T(""), ptcName);
  }

  return ptcNewPath;
}


/*-
 ***********************************************************************
 *
 * RTimesNewProperties
 *
 ***********************************************************************
 */
RTIMES_PROPERTIES *
RTimesNewProperties(TCHAR *ptcError)
{
  RTIMES_PROPERTIES  *psProperties = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (RTIMES_PROPERTIES *) calloc(sizeof(RTIMES_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesNewProperties(): calloc(): %s"), strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * RTimesOptionHandler
 *
 ***********************************************************************
 */
int
RTimesOptionHandler(OPTIONS_TABLE *psOption, TCHAR *ptcValue, RTIMES_PROPERTIES *psProperties, TCHAR *ptcError)
{
  int                 iLength = 0;

  iLength = (ptcValue == NULL) ? 0 : _tcslen(ptcValue);

  switch (psOption->iId)
  {
  case OPT_MaxData:
    if (iLength < 1)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesOptionHandler(): option=[%s], value=[%s]: Value is null."), psOption->atcFullName, ptcValue);
      return ER;
    }
    while (iLength > 0)
    {
      if (!isdigit((int) ptcValue[iLength - 1]))
      {
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesOptionHandler(): option=[%s], value=[%s]: Value must be an integer."), psOption->atcFullName, ptcValue);
        return ER;
      }
      iLength--;
    }
    psProperties->dwMaxData = (int) strtol(ptcValue, NULL, 10);
    if (errno == ERANGE)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesOptionHandler(): option=[%s], value=[%s]: %s"), psOption->atcFullName, ptcValue, strerror(errno));
      return ER;
    }
    if (psProperties->dwMaxData < RTIMES_MIN_DATA_SIZE || psProperties->dwMaxData > RTIMES_MAX_DATA_SIZE)
    {
      _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesOptionHandler(): option=[%s], value=[%s]: Value must be in the range [%d-%d]."), psOption->atcFullName, ptcValue, RTIMES_MIN_DATA_SIZE, RTIMES_MAX_DATA_SIZE);
      return ER;
    }
    gdwDataLimit = psProperties->dwMaxData;
    break;
  default:
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesOptionHandler(): Invalid option (%d). That shouldn't happen."), psOption->iId);
    return ER;
    break;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesProcessArguments
 *
 ***********************************************************************
 */
int
RTimesProcessArguments(int iArgumentCount, TCHAR *pptcArgumentVector[], RTIMES_PROPERTIES *psProperties, TCHAR *ptcError)
{
  TCHAR               atcLocalError[MESSAGE_SIZE] = { 0 };
  TCHAR              *ptcHive = NULL;
  TCHAR              *ptcKey = NULL;
  TCHAR              *ptcMode = NULL;
  int                 iError = 0;
  int                 iOperandCount = 0;
  OPTIONS_CONTEXT    *psOptionsContext = NULL;
  OPTIONS_TABLE       asMapOptions[] =
  {
    { OPT_MaxData, "-d", "--max-data",  0, 0, 1, 0, RTimesOptionHandler },
  };

  /*-
   *********************************************************************
   *
   * Initialize the options context.
   *
   *********************************************************************
   */
  psOptionsContext = OptionsNewOptionsContext(iArgumentCount, pptcArgumentVector, atcLocalError);
  if (psOptionsContext == NULL)
  {
    snprintf(ptcError, MESSAGE_SIZE, "RTimesProcessArguments(): %s", atcLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Determine the run mode.
   *
   *********************************************************************
   */
  ptcMode = OptionsGetFirstArgument(psOptionsContext);
  if (ptcMode == NULL)
  {
    RTimesUsage();
  }
  else
  {
    if (_tcscmp(ptcMode, _T("-m")) == 0 || _tcscmp(ptcMode, _T("--map")) == 0)
    {
      psProperties->iRunMode = RTIMES_MAP_MODE;
      OptionsSetOptions(psOptionsContext, asMapOptions, (sizeof(asMapOptions) / sizeof(asMapOptions[0])));
    }
    else if (_tcscmp(ptcMode, _T("-v")) == 0 || _tcscmp(ptcMode, _T("--version")) == 0)
    {
      RTimesVersion();
    }
    else
    {
      RTimesUsage();
    }
  }

  /*-
   *********************************************************************
   *
   * Process options.
   *
   *********************************************************************
   */
  iError = OptionsProcessOptions(psOptionsContext, (void *) psProperties, atcLocalError);
  switch (iError)
  {
  case OPTIONS_ER:
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesProcessArguments(): %s"), atcLocalError);
    return ER;
    break;
  case OPTIONS_OK:
    break;
  case OPTIONS_USAGE:
  default:
    RTimesUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * Handle any special cases and/or remaining arguments.
   *
   *********************************************************************
   */
  iOperandCount = OptionsGetArgumentsLeft(psOptionsContext);
  switch (psProperties->iRunMode)
  {
  case RTIMES_MAP_MODE:
    if (iOperandCount == 0)
    {
      /* Map all hives. */
    }
    else if (iOperandCount >= 1 && iOperandCount <= 2)
    {
      ptcHive = OptionsGetNextArgument(psOptionsContext);
      if (ptcHive == NULL)
      {
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesProcessArguments(): Unable to get hive argument."));
        return ER;
      }
      if (_tcsicmp(ptcHive, _T("HKEY_CLASSES_ROOT")) == 0 || _tcsicmp(ptcHive, _T("HKCR")) == 0)
      {
        gsTodoList[0].dwHive = HKEY_CLASSES_ROOT;
        _sntprintf(gsTodoList[0].atcHive, 256, _T("HKEY_CLASSES_ROOT"));
        _sntprintf(gsTodoList[0].atcKey, 256, _T(""));
        gdwTodoSize = 1;
      }
      else if (_tcsicmp(ptcHive, _T("HKEY_LOCAL_MACHINE")) == 0 || _tcsicmp(ptcHive, _T("HKLM")) == 0)
      {
        gsTodoList[0].dwHive = HKEY_LOCAL_MACHINE;
        _sntprintf(gsTodoList[0].atcHive, 256, _T("HKEY_LOCAL_MACHINE"));
        _sntprintf(gsTodoList[0].atcKey, 256, _T(""));
        gdwTodoSize = 1;
      }
      else if (_tcsicmp(ptcHive, _T("HKEY_USERS")) == 0 || _tcsicmp(ptcHive, _T("HKU")) == 0)
      {
        gsTodoList[0].dwHive = HKEY_USERS;
        _sntprintf(gsTodoList[0].atcHive, 256, _T("HKEY_USERS"));
        _sntprintf(gsTodoList[0].atcKey, 256, _T(""));
        gdwTodoSize = 1;
      }
      else if (_tcsicmp(ptcHive, _T("HKEY_CURRENT_USER")) == 0 || _tcsicmp(ptcHive, _T("HKCU")) == 0)
      {
        gsTodoList[0].dwHive = HKEY_CURRENT_USER;
        _sntprintf(gsTodoList[0].atcHive, 256, _T("HKEY_CURRENT_USER"));
        _sntprintf(gsTodoList[0].atcKey, 256, _T(""));
        gdwTodoSize = 1;
      }
      else if (_tcsicmp(ptcHive, _T("HKEY_CURRENT_CONFIG")) == 0 || _tcsicmp(ptcHive, _T("HKCC")) == 0)
      {
        gsTodoList[0].dwHive = HKEY_CURRENT_CONFIG;
        _sntprintf(gsTodoList[0].atcHive, 256, _T("HKEY_CURRENT_CONFIG"));
        _sntprintf(gsTodoList[0].atcKey, 256, _T(""));
        gdwTodoSize = 1;
      }
      else
      {
        _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesProcessArguments(): Invalid hive (%s)."), ptcHive);
        return ER;
      }
      ptcKey = OptionsGetNextArgument(psOptionsContext);
      if (ptcKey != NULL)
      {
//FIXME Check path length.
        _sntprintf(gsTodoList[0].atcKey, 256, _T(ptcKey));
        gsTodoList[0].atcKey[255] = 0;
      }
    }
    else
    {
      RTimesUsage();
    }
    break;
  default:
    if (iOperandCount > 0)
    {
      RTimesUsage();
    }
    break;
  }

  /*-
   *********************************************************************
   *
   * If any required arguments are missing, it's an error.
   *
   *********************************************************************
   */
  if (OptionsHaveRequiredOptions(psOptionsContext) == 0)
  {
    RTimesUsage();
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesSetPropertiesReference
 *
 ***********************************************************************
 */
void
RTimesSetPropertiesReference(RTIMES_PROPERTIES *psProperties)
{
  gpsProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * RTimesSidToString
 *
 ***********************************************************************
 */
TCHAR *
RTimesSidToString(SID *psSid, TCHAR *ptcError)
{
  APP_UI64 ui64SidIdentifierAuthority = 0;
  DWORD dwCounter = 0;
  DWORD dwIndex = 0;
  DWORD dwSidStringSize = 0;
  DWORD dwSubAuthorities = 0;
  SID_IDENTIFIER_AUTHORITY *psSia = NULL;
  TCHAR *ptcSid = NULL;
  TCHAR *ptcWinxError = NULL;

  /*-
   *********************************************************************
   *
   * Check to see if the SID is valid.
   *
   *********************************************************************
   */
  if (!IsValidSid(psSid))
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesSidToString(): SID does not pass muster."));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Compute and allocate memory to hold the SID in string form. Use
   * the maximum length for each component as the basis for computing
   * the required size. The string format used to represent a SID is
   * as follows:
   *
   *   P-R-I-S[-S]{0,14}
   *
   * where
   *
   *   P - Prefix ('S' by convention)
   *   R - Revision
   *   I - Identifier Authority Value
   *   S - Subauthority Value
   *
   * The maximum length, therefore, should not exceed the length of
   * the following expression:
   *
   *   S-255-18446744073709551615-4294967295[-4294967295]{0,14}
   *
   * Note that SID_MAX_SUB_AUTHORITIES is defined to be 15 in winnt.h.
   *
   *********************************************************************
   */
  dwSubAuthorities = *GetSidSubAuthorityCount(psSid);
  dwSidStringSize = (1 + (1 + 3) + (1 + 20) + ((1 + 10) * dwSubAuthorities) + 1) * sizeof(TCHAR);
  ptcSid = (TCHAR *) LocalAlloc(LMEM_FIXED, dwSidStringSize);
  if (ptcSid == NULL)
  {
    FormatWinxError(GetLastError(), &ptcWinxError);
    _sntprintf(ptcError, MESSAGE_SIZE, _T("RTimesSidToString(): LocalAlloc(): %s."), ptcWinxError);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Fill in the blanks.
   *
   *********************************************************************
   */
  psSia = GetSidIdentifierAuthority(psSid);
  ui64SidIdentifierAuthority  = (APP_UI64) (((APP_UI64) psSia->Value[0]) << 40);
  ui64SidIdentifierAuthority |= (APP_UI64) (((APP_UI64) psSia->Value[1]) << 32);
  ui64SidIdentifierAuthority |= (APP_UI64) (((APP_UI64) psSia->Value[2]) << 24);
  ui64SidIdentifierAuthority |= (APP_UI64) (((APP_UI64) psSia->Value[3]) << 16);
  ui64SidIdentifierAuthority |= (APP_UI64) (((APP_UI64) psSia->Value[4]) <<  8);
  ui64SidIdentifierAuthority |= (APP_UI64) (((APP_UI64) psSia->Value[5])      );
  dwIndex = _stprintf(ptcSid, _T("S-%lu-%I64u"), (unsigned long) psSid->Revision, (APP_UI64) ui64SidIdentifierAuthority);
  for(dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++)
  {
    dwIndex += _stprintf(&ptcSid[dwIndex], _T("-%lu"), *GetSidSubAuthority(psSid, (unsigned long) dwCounter));
  }

  return ptcSid;
}


/*-
 ***********************************************************************
 *
 * RTimesUsage
 *
 ***********************************************************************
 */
void
RTimesUsage(void)
{
  _ftprintf(stderr, _T("\n"));
  _ftprintf(stderr, _T("Usage: %s {-m|--map} [{-d|--max-data} bytes] [{HKCR|HKLM|HKU|HKCU|HKCC} [key]]\n"), _T(PROGRAM));
  _ftprintf(stderr, _T("       %s {-v|--version}\n"), _T(PROGRAM));
  _ftprintf(stderr, _T("\n"));
  exit(XER_Usage);
}


/*-
 ***********************************************************************
 *
 * RTimesVersion
 *
 ***********************************************************************
 */
void
RTimesVersion(void)
{
  _ftprintf(stdout,
    _T("%s %s %d-bit\n"),
    _T(PROGRAM),
    _T(VERSION),
    (int) (sizeof(&RTimesVersion) * 8)
    );
  exit(XER_OK);
}


/*-
 ***********************************************************************
 *
 * RTimesWorkHorse
 *
 ***********************************************************************
 */
int
RTimesWorkHorse(RTIMES_PROPERTIES *psProperties, TCHAR *ptcError)
{
  DWORD dwItem = 0;
  DWORD dwReturnCode = 0;
  FILE *pFile = stdout;
  HKEY hKey;
  int iError = 0;
  TCHAR atcLocalError[MESSAGE_SIZE];
  TCHAR *ptcPath = NULL;
  TCHAR *ptcWinxError = NULL;

//FIXME Check return value ...
  _ftprintf(pFile, _T("name|flags|osid|gsid|dacl|class|wtime|type|size|md5|data\n"));

  for (dwItem = 0; dwItem < gdwTodoSize; dwItem++)
  {
    dwReturnCode = RegOpenKeyEx(
      gsTodoList[dwItem].dwHive,
      (_tcslen(gsTodoList[dwItem].atcKey)) ? gsTodoList[dwItem].atcKey : NULL,
      0,
      KEY_READ,
      &hKey
      );
    if (dwReturnCode != ERROR_SUCCESS)
    {
//FIXME Do we handle ERROR_FILE_NOT_FOUND and ERROR_ACCESS_DENIED differently?
      FormatWinxError(dwReturnCode, &ptcWinxError);
//FIXME Log it ...
      _ftprintf(stderr, _T("%s: Main(): RTimesWorkHorse(): RegOpenKeyEx(%s): %s\n"), _T(PROGRAM), (_tcslen(gsTodoList[dwItem].atcKey)) ? gsTodoList[dwItem].atcKey : _T(""), ptcWinxError);
      continue;
    }
    ptcPath = RTimesNewPath(gsTodoList[dwItem].atcHive, (_tcslen(gsTodoList[dwItem].atcKey)) ? gsTodoList[dwItem].atcKey : NULL, _T("\\"), atcLocalError);
    if (ptcPath == NULL)
    {
//FIXME Log it ...
      _ftprintf(stderr, _T("%s: Main(): RTimesWorkHorse(): %s\n"), _T(PROGRAM), atcLocalError);
      RegCloseKey(hKey);
      continue;
    }
    iError = RTimesMapKey(pFile, hKey, ptcPath, atcLocalError);
    if (iError != ER_OK)
    {
//FIXME Log it ...
      _ftprintf(stderr, _T("%s: Main(): RTimesWorkHorse(): %s\n"), _T(PROGRAM), atcLocalError);
    }
    free(ptcPath);
    RegCloseKey(hKey);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * RTimesWriteValueRecord
 *
 ***********************************************************************
 */
void
RTimesWriteValueRecord(FILE *pFile, DWORD dwFlags, DWORD dwType, BYTE *pbyteData, DWORD dwDataSize, TCHAR *ptcPath, TCHAR *ptcName)
{
  int iNeuterOptions = NEUTER_ENCODING_RTIMES;
  TCHAR atcHexMd5[(((MD5_HASH_SIZE * 2) + 1) * sizeof(TCHAR))];
  TCHAR atcLocalError[MESSAGE_SIZE];
  TCHAR atcType[RTIMES_TYPE_SIZE];
  TCHAR *ptcNeuteredData = NULL;
  TCHAR *ptcNeuteredPath = NULL;
  TCHAR *ptcNeuteredName = NULL;
  unsigned char aucMd5[MD5_HASH_SIZE];

  /*-
   *********************************************************************
   *
   * Check the value type. The type also determines the data encoding.
   *
   *********************************************************************
   */
  switch (dwType)
  {
  case REG_BINARY:
    _tcscpy(atcType, _T("REG_BINARY"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_DWORD:
    _tcscpy(atcType, _T("REG_DWORD"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_DWORD_BIG_ENDIAN:
    _tcscpy(atcType, _T("REG_DWORD_BIG_ENDIAN"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_EXPAND_SZ:
    _tcscpy(atcType, _T("REG_EXPAND_SZ"));
    break;
  case REG_FULL_RESOURCE_DESCRIPTOR:
    _tcscpy(atcType, _T("REG_FULL_RESOURCE_DESCRIPTOR"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_LINK:
    _tcscpy(atcType, _T("REG_LINK"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_MULTI_SZ:
    _tcscpy(atcType, _T("REG_MULTI_SZ"));
    break;
  case REG_NONE:
    _tcscpy(atcType, _T("REG_NONE"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_RESOURCE_LIST:
    _tcscpy(atcType, _T("REG_RESOURCE_LIST"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_RESOURCE_REQUIREMENTS_LIST:
    _tcscpy(atcType, _T("REG_RESOURCE_REQUIREMENTS_LIST"));
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  case REG_SZ:
    _tcscpy(atcType, _T("REG_SZ"));
    break;
  default:
    _stprintf(atcType, _T("%d"), (int) dwType);
    iNeuterOptions = NEUTER_ENCODING_FULL;
    break;
  }

  /*-
   *********************************************************************
   *
   * Hash all available data. Note that the type of truncation matters
   * (i.e. mandatory vs. requested). If the truncate is mandatory,
   * then only that data returned to the process can be hashed. If the
   * truncate is requested, then all data are hashed prior to the
   * user-impoded truncate operation that follows.
   *
   *********************************************************************
   */
  MD5HashString(pbyteData, dwDataSize, aucMd5);
  RTimesMd5ToHex(aucMd5, atcHexMd5); /* Note: This version does it the TCHAR way. */

  /*-
   *********************************************************************
   *
   * Conditionally truncate, and then, neuter the data.
   *
   *********************************************************************
   */
  if (gdwDataLimit && dwDataSize > gdwDataLimit)
  {
    dwFlags |= RTIMES_FLAG_REQUESTED_TRUNCATE;
    ptcNeuteredData = RTimesNeuterEncodeData((unsigned char *) pbyteData, gdwDataLimit, iNeuterOptions, atcLocalError);
  }
  else
  {
    ptcNeuteredData = RTimesNeuterEncodeData((unsigned char *) pbyteData, dwDataSize, iNeuterOptions, atcLocalError);
  }
  if (ptcNeuteredData == NULL)
  {
//FIXME Log it ...
    _ftprintf(stderr, _T("%s: Error encoding the specified data: %s\n"), _T(PROGRAM), atcLocalError);
    dwFlags |= RTIMES_FLAG_DATA_MISSING;
  }

  /*-
   *********************************************************************
   *
   * Neuter the path and name. If the name is an empty string, assume
   * that this is the default value.
   *
   *********************************************************************
   */
  ptcNeuteredPath = RTimesNeuterEncodeData((unsigned char *) ptcPath, _tcslen(ptcPath), NEUTER_ENCODING_RTIMES, atcLocalError);
  if (ptcNeuteredPath == NULL)
  {
//FIXME Log it ...
    _ftprintf(stderr, _T("%s: Error encoding the specified path: %s\n"), _T(PROGRAM), atcLocalError);
    if (ptcNeuteredData != NULL)
    {
      free(ptcNeuteredData);
    }
    return;
  }

  if (ptcName[0] == 0)
  {
    dwFlags |= RTIMES_FLAG_DEFAULT_VALUE;
    ptcNeuteredName = RTimesNeuterEncodeData((unsigned char *) _T("(Default)"), _tcslen(_T("(Default)")), NEUTER_ENCODING_RTIMES, atcLocalError);
  }
  else
  {
    ptcNeuteredName = RTimesNeuterEncodeData((unsigned char *) ptcName, _tcslen(ptcName), NEUTER_ENCODING_RTIMES, atcLocalError);
  }
  if (ptcNeuteredName == NULL)
  {
//FIXME Log it ...
    _ftprintf(stderr, _T("%s: Error encoding the specified name: %s\n"), _T(PROGRAM), atcLocalError);
    if (ptcNeuteredData != NULL)
    {
      free(ptcNeuteredData);
    }
    if (ptcNeuteredPath != NULL)
    {
      free(ptcNeuteredPath);
    }
    return;
  }

  /*-
   *********************************************************************
   *
   * Write a record. Format:
   *
   *  name|flags|osid|gsid|dacl|class|wtime|type|size|md5|data
   *
   *********************************************************************
   */
//FIXME Check return value ...
  _ftprintf(pFile, _T("\"%s\\%s\"|0x%x||||||%s|%d|%s|%s\n"),
    ptcNeuteredPath,
    ptcNeuteredName,
    (unsigned int) dwFlags,
    atcType,
    (int) dwDataSize,
    atcHexMd5,
    (ptcNeuteredData == NULL) ? "" : ptcNeuteredData
    );

  /*-
   *********************************************************************
   *
   * Cleanup and return.
   *
   *********************************************************************
   */
  if (ptcNeuteredData != NULL)
  {
    free(ptcNeuteredData);
  }

  if (ptcNeuteredPath != NULL)
  {
    free(ptcNeuteredPath);
  }

  if (ptcNeuteredName != NULL)
  {
    free(ptcNeuteredName);
  }

  return;
}


/*-
 ***********************************************************************
 *
 * RTimesWTimeToString
 *
 ***********************************************************************
 */
int
RTimesWTimeToString(FILETIME *pFileTime, TCHAR *ptcTime)
{
  int iCount = 0;
  SYSTEMTIME systemTime = { 0 };

  ptcTime[0] = 0;

  if (!FileTimeToSystemTime(pFileTime, &systemTime))
  {
    return ER;
  }

  iCount = _sntprintf(ptcTime, RTIMES_TIME_FORMAT_LENGTH, RTIMES_TIME_FORMAT,
    systemTime.wYear,
    systemTime.wMonth,
    systemTime.wDay,
    systemTime.wHour,
    systemTime.wMinute,
    systemTime.wSecond,
    systemTime.wMilliseconds
    );
  if (iCount < RTIMES_TIME_FORMAT_LENGTH - 3 || iCount > RTIMES_TIME_FORMAT_LENGTH - 1)
  {
    return ER;
  }

  return ER_OK;
}
