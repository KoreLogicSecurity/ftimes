/*-
 ***********************************************************************
 *
 * $Id: time.c,v 1.9 2007/02/23 00:22:35 mavrik Exp $
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
