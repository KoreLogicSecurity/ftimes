/*-
 ***********************************************************************
 *
 * $Id: error.c,v 1.17 2013/02/14 16:55:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static int            giWarnings = 0;
static int            giFailures = 0;

/*-
 ***********************************************************************
 *
 * ErrorGetWarnings
 *
 ***********************************************************************
 */
int
ErrorGetWarnings()
{
  return giWarnings;
}


/*-
 ***********************************************************************
 *
 * ErrorGetFailures
 *
 ***********************************************************************
 */
int
ErrorGetFailures()
{
  return giFailures;
}


/*-
 ***********************************************************************
 *
 * ErrorHandler
 *
 ***********************************************************************
 */
void
ErrorHandler(int iError, char *pcError, int iSeverity)
{
  switch (iSeverity)
  {
  case ERROR_WARNING:
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_WARNING, MESSAGE_WARNING_STRING, pcError);
    giWarnings++;
    break;
  case ERROR_FAILURE:
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_FAILURE, MESSAGE_FAILURE_STRING, pcError);
    giFailures++;
    break;
  case ERROR_CRITICAL:
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_CRITICAL, MESSAGE_CRITICAL_STRING, pcError);
    exit((iError >= 0 && iError < XER_MaxExternalErrorCode) ? iError : XER_Abort);
    break;
  default:
    ErrorHandler(ER_InvalidSeverity, "ErrorHandler(): Invalid Severity", ERROR_CRITICAL);
    break;
  }
}


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * ErrorFormatWinxError
 *
 ***********************************************************************
 */
void
ErrorFormatWinxError(DWORD dwError, TCHAR **pptcMessage)
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
  FormatMessage
  (
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
#endif
