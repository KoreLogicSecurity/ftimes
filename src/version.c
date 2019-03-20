/*-
 ***********************************************************************
 *
 * $Id: version.c,v 1.6 2012/04/14 19:01:21 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2011-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * VersionGetVersion
 *
 ***********************************************************************
 */
char *
VersionGetVersion(void)
{
  static char         acMyVersion[VERSION_MAX_VERSION_LENGTH] = "NA";
  static char         acMyState[3] = "";
  int                 iCount = 0;
  int                 iIndex = 0;
  int                 iSize = VERSION_MAX_VERSION_LENGTH;

  /*-
   *********************************************************************
   *
   *  Version Scheme:
   *
   *    The version scheme allocates 4 bits to the major number, 8
   *    bits to the minor number, 8 bits to the patch number, 2 bits
   *    to the state number, and 10 bits to the build number. This
   *    yields the following breakout.
   *
   *  +----+--------+--------+--+----------+
   *  |3322|22222222|11111111|11|          |
   *  |1098|76543210|98765432|10|9876543210|
   *  |----+--------+--------|--|----------+
   *  |MMMM|mmmmmmmm|pppppppp|ss|bbbbbbbbbb|
   *  +----+--------+--------|--+----------+
   *   ^^^^ ^^^^^^^^ ^^^^^^^^ ^^ ^^^^^^^^^^
   *      |        |       |   |          |
   *      |        |       |   |          +-----> b - build (0...1023)
   *      |        |       |   +----------------> s - state (0......3)
   *      |        |       +--------------------> p - patch (0....255)
   *      |        +----------------------------> m - minor (0....255)
   *      +-------------------------------------> M - major (0.....15)
   *
   *  State Numbers:
   *
   *    00 = ds --> Development Snapshot
   *    01 = rc --> Release Candidate
   *    10 = sr --> Standard Release
   *    11 = xs --> eXtended Snapshot
   *
   *********************************************************************
   */
  switch ((VERSION >> 10) & 0x03)
  {
    case 0:
      acMyState[0] = 'd';
      acMyState[1] = 's';
      break;
    case 1:
      acMyState[0] = 'r';
      acMyState[1] = 'c';
      break;
    case 2:
      acMyState[0] = 's';
      acMyState[1] = 'r';
      break;
    case 3:
      acMyState[0] = 'x';
      acMyState[1] = 's';
      break;
  }
  acMyState[2] = 0;

  /*-
   *********************************************************************
   *
   * If this is a standard release with a build number of zero, omit
   * state from the version string.
   *
   *********************************************************************
   */
  if (((VERSION >> 10) & 0x03) == 2 && ((VERSION & 0x3ff) == 0))
  {
    iIndex = snprintf(acMyVersion, VERSION_MAX_VERSION_LENGTH, "%s %d.%d.%d %d-bit",
      PROGRAM_NAME,
      (VERSION >> 28) & 0x0f,
      (VERSION >> 20) & 0xff,
      (VERSION >> 12) & 0xff,
      (int) (sizeof(&VersionGetVersion) * 8)
      );
  }
  else
  {
    iIndex = snprintf(acMyVersion, VERSION_MAX_VERSION_LENGTH, "%s %d.%d.%d (%s%d) %d-bit",
      PROGRAM_NAME,
      (VERSION >> 28) & 0x0f,
      (VERSION >> 20) & 0xff,
      (VERSION >> 12) & 0xff,
      acMyState,
      VERSION & 0x3ff,
      (int) (sizeof(&VersionGetVersion) * 8)
      );
  }
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;

  /*-
   *********************************************************************
   *
   * Tack on any relevant application-specific capabilities.
   *
   *********************************************************************
   */
#if defined(USE_FILE_HOOKS)
  iIndex += snprintf(&acMyVersion[iIndex], iSize, "%sklel(%s)", (iCount++ == 0) ? " " : ",", KlelGetRelease());
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;
#endif
#ifdef USE_PCRE
  iIndex += snprintf(&acMyVersion[iIndex], iSize, "%spcre(%d.%d)", (iCount++ == 0) ? " " : ",", PCRE_MAJOR, PCRE_MINOR);
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;
#endif
#ifdef USE_EMBEDDED_PERL
  iIndex += snprintf(&acMyVersion[iIndex], iSize, "%sperl(%d.%d.%d)", (iCount++ == 0) ? " " : ",", PERL_REVISION, PERL_VERSION, PERL_SUBVERSION);
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;
#endif
#ifdef USE_SSL
  iIndex += snprintf(&acMyVersion[iIndex], iSize, "%s%s", (iCount++ == 0) ? " " : ",", SslGetVersion());
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;
#endif
#ifdef USE_XMAGIC
  iIndex += snprintf(&acMyVersion[iIndex], iSize, "%sxmagic", (iCount++ == 0) ? " " : ",");
  iSize = ((VERSION_MAX_VERSION_LENGTH - iIndex) <= 0) ? 0 : VERSION_MAX_VERSION_LENGTH - iIndex;
#endif

  return acMyVersion;
}
