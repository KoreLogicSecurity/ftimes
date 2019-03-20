/*-
 ***********************************************************************
 *
 * $Id: time.c,v 1.21 2019/03/14 16:07:43 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * TimeGetTimeValue
 *
 ***********************************************************************
 */
int
TimeGetTimeValue(struct timeval *psTimeValue)
{
#ifdef WIN32
  FILETIME            sFileTimeNow = { 0 };
  __int64             i64Now = 0;
#endif

  psTimeValue->tv_sec  = 0;
  psTimeValue->tv_usec = 0;
#ifdef WIN32
  GetSystemTimeAsFileTime(&sFileTimeNow);
  i64Now = (((__int64) sFileTimeNow.dwHighDateTime) << 32) | sFileTimeNow.dwLowDateTime;
  i64Now -= UNIX_EPOCH_IN_NT_TIME;
  i64Now /= 10;
  psTimeValue->tv_sec  = (long) (i64Now / 1000000);
  psTimeValue->tv_usec = (long) (i64Now % 1000000);
  return (ER_OK);
#else
  return gettimeofday(psTimeValue, NULL);
#endif
}


/*-
 ***********************************************************************
 *
 * TimeGetTimeValueAsDouble
 *
 ***********************************************************************
 */
double
TimeGetTimeValueAsDouble(void)
{
  struct timeval      sTimeValue;

  TimeGetTimeValue(&sTimeValue);
  return (double) sTimeValue.tv_sec + (double) sTimeValue.tv_usec * 0.000001;
}


/*-
 ***********************************************************************
 *
 * TimeGetTime
 *
 ***********************************************************************
 */
time_t
TimeGetTime(char *pcDate, char *pcTime, char *pcZone, char *pcDateTime)
{
  int                 iCount;
  time_t              timeValue;

  if ((timeValue = time(NULL)) < 0)
  {
    return ER;
  }

  if (pcTime != NULL)
  {
    iCount = strftime(pcTime, FTIMES_TIME_SIZE, FTIMES_RUNTIME_FORMAT, localtime(&timeValue));
    if (iCount < 0 || iCount > FTIMES_TIME_SIZE - 1)
    {
      return ER;
    }
  }

  if (pcDate != NULL)
  {
    iCount = strftime(pcDate, FTIMES_TIME_SIZE, FTIMES_RUNDATE_FORMAT, localtime(&timeValue));
    if (iCount < 0 || iCount > FTIMES_TIME_SIZE - 1)
    {
      return ER;
    }
  }

  if (pcZone != NULL)
  {
    iCount = strftime(pcZone, FTIMES_ZONE_SIZE, FTIMES_RUNZONE_FORMAT, localtime(&timeValue));
    if (iCount < 0 || iCount > FTIMES_ZONE_SIZE - 1)
    {
      return ER;
    }
  }

  /*-
   *********************************************************************
   *
   * As a matter of convention, datetime values are in GMT. This
   * prevents confusion with filenames and Integrity Server
   * transactions. If localtime was used instead, then issues with
   * things like Daylight Savings Time can arise. For example, when
   * Daylight Savings Time falls back, it is possible to clobber
   * old output files because there would be an hour of time for
   * which output filenames could match existing names.
   *
   *********************************************************************
   */
  if (pcDateTime != NULL)
  {
    iCount = strftime(pcDateTime, FTIMES_TIME_SIZE, "%Y%m%d%H%M%S", gmtime(&timeValue));
    if (iCount < 0 || iCount > FTIMES_TIME_SIZE - 1)
    {
      return ER;
    }
  }

  return timeValue;
}


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * TimeFormatTime
 *
 ***********************************************************************
 */
int
TimeFormatTime(time_t *pTimeValue, char *pcTime)
{
  int                 iCount;

  /*-
   *********************************************************************
   *
   * Constraint all time stamps are relative to GMT. In practice, this
   * means we use gmtime instead of localtime.
   *
   *********************************************************************
   */

  pcTime[0] = 0;

  iCount = strftime(pcTime, FTIMES_TIME_SIZE, FTIMES_TIME_FORMAT, gmtime(pTimeValue));

  if (iCount != FTIMES_TIME_FORMAT_SIZE - 1)
  {
    return ER;
  }

  return ER_OK;
}
#endif


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * TimeFormatTime
 *
 ***********************************************************************
 */
int
TimeFormatTime(FILETIME *psFileTime, char *pcTime)
{
  int                 iCount;
  SYSTEMTIME          sSystemTime;

  pcTime[0] = 0;

  if (!FileTimeToSystemTime(psFileTime, &sSystemTime))
  {
    return ER;
  }

  iCount = snprintf(pcTime, FTIMES_TIME_FORMAT_SIZE, FTIMES_TIME_FORMAT,
                   sSystemTime.wYear, sSystemTime.wMonth, sSystemTime.wDay,
                   sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond,
                   sSystemTime.wMilliseconds);

  if (iCount < FTIMES_TIME_FORMAT_SIZE - 3 || iCount > FTIMES_TIME_FORMAT_SIZE - 1)
  {
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * TimeFormatOutOfBandTime
 *
 ***********************************************************************
 */
int
TimeFormatOutOfBandTime(FILETIME *psFileTime, char *pcTime)
{
  int                 iCount;
  SYSTEMTIME          sSystemTime;

  pcTime[0] = 0;

  if (!FileTimeToSystemTime(psFileTime, &sSystemTime))
  {
    return ER;
  }

  iCount = snprintf(pcTime, FTIMES_OOB_TIME_FORMAT_SIZE, FTIMES_OOB_TIME_FORMAT,
                   sSystemTime.wYear, sSystemTime.wMonth, sSystemTime.wDay,
                   sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond,
                   sSystemTime.wMilliseconds);

  if (iCount < FTIMES_OOB_TIME_FORMAT_SIZE - 3 || iCount > FTIMES_OOB_TIME_FORMAT_SIZE - 1)
  {
    return ER;
  }

  return ER_OK;
}
#endif
