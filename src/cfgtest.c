/*
 ***********************************************************************
 *
 * $Id: cfgtest.c,v 1.1.1.1 2002/01/18 03:17:20 mavrik Exp $
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
 * CfgTestProcessArguments
 *
 ***********************************************************************
 */
int
CfgTestProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "CfgTestProcessArguments()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iLength;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Process arguments.
   *
   *********************************************************************
   */
  if (iArgumentCount >= 2 && iArgumentCount <= 3)
  {
    iLength = strlen(ppcArgumentVector[0]);
    if (iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, ppcArgumentVector[0], iLength, FTIMES_MAX_PATH - 1);
      return ER_Length;
    }
    strncpy(psProperties->cConfigFile, ppcArgumentVector[0], FTIMES_MAX_PATH);
    if (strcasecmp(ppcArgumentVector[1], "digauto") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGAUTO;
    }
    else if (strcasecmp(ppcArgumentVector[1], "digfull") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGFULL;
    }
    else if (strcasecmp(ppcArgumentVector[1], "getmode") == 0)
    {
      psProperties->iTestRunMode = FTIMES_GETMODE;
    }
    else if (strcasecmp(ppcArgumentVector[1], "mapfull") == 0)
    {
      psProperties->iTestRunMode = FTIMES_MAPFULL;
    }
    else if (strcasecmp(ppcArgumentVector[1], "putmode") == 0)
    {
      psProperties->iTestRunMode = FTIMES_PUTMODE;
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Mode = [%s]: Mode must be one of {digauto|digfull|getmode|mapfull|putmode}.", cRoutine, ppcArgumentVector[1]);
      return ER_BadValue;
    }
    if (iArgumentCount == 3 && strcmp(ppcArgumentVector[2], "-s") == 0)
    {
      psProperties->iTestLevel = FTIMES_TEST_STRICT;
    }
  }
  else
  {
    return ER_Usage;
  }

  return ER_OK;
}
