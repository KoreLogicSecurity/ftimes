/*-
 ***********************************************************************
 *
 * $Id: cfgtest.c,v 1.8 2005/04/02 18:08:23 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2005 Klayton Monroe, All Rights Reserved.
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
  const char          acRoutine[] = "CfgTestProcessArguments()";
  int                 iLength;

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
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, ppcArgumentVector[0], iLength, FTIMES_MAX_PATH - 1);
      return ER_Length;
    }
    strncpy(psProperties->acConfigFile, ppcArgumentVector[0], FTIMES_MAX_PATH);
    if (strcasecmp(ppcArgumentVector[1], "digauto") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGAUTO;
    }
    else if (strcasecmp(ppcArgumentVector[1], "digfull") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGFULL;
    }
    else if (strcasecmp(ppcArgumentVector[1], "diglean") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGLEAN;
    }
    else if (strcasecmp(ppcArgumentVector[1], "getmode") == 0)
    {
      psProperties->iTestRunMode = FTIMES_GETMODE;
    }
    else if (strcasecmp(ppcArgumentVector[1], "mapfull") == 0)
    {
      psProperties->iTestRunMode = FTIMES_MAPFULL;
    }
    else if (strcasecmp(ppcArgumentVector[1], "maplean") == 0)
    {
      psProperties->iTestRunMode = FTIMES_MAPLEAN;
    }
    else if (strcasecmp(ppcArgumentVector[1], "putmode") == 0)
    {
      psProperties->iTestRunMode = FTIMES_PUTMODE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Mode = [%s]: Mode must be one of {digauto|digfull|diglean|getmode|mapfull|maplean|putmode}.", acRoutine, ppcArgumentVector[1]);
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
