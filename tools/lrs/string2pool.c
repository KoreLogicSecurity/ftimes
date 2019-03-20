/*-
 ***********************************************************************
 *
 * $Id: string2pool.c,v 1.5 2007/02/23 00:22:36 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2002-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#define PROGRAM           "string2pool"
#define STRING2POOL_MAX_TRIES       10
#define STRING2POOL_POOL_SIZE   0x8000
#define STRING2POOL_PREFIX_SIZE     32
#define STRING2POOL_SUFFIX_SIZE     32


/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define STRING2POOL_NEW_POOL(aucPool, iLength, ulState)\
{\
  int i, j;\
  unsigned long ul = ulState;\
  for (i = 0; i < iLength; i++)\
  {\
    for (j = 0, aucPool[i] = 0; j < 8; j++)\
    {\
      aucPool[i] |= (ul & 1) << j;\
      ul = ((((ul >> 7) ^ (ul >> 6) ^ (ul >> 2) ^ (ul >> 0)) & 1) << 31) | (ul >> 1);\
    }\
  }\
  ulState = ul; /* Return the last register state -- allows the caller to continue the sequence. */\
}


/*-
 ***********************************************************************
 *
 * ConstructIncludeFile
 *
 ***********************************************************************
 */
void
ConstructIncludeFile(FILE *pFile, char *pcPoolType, unsigned long ulFinal, int *piTaps, int iTapsCount)
{
  char                acPrefix[STRING2POOL_PREFIX_SIZE];
  char                acSuffix[STRING2POOL_SUFFIX_SIZE];
  int                 i;

  for (i = 0; i < (int)strlen(pcPoolType); i++)
  {
    acPrefix[i] = tolower((int)pcPoolType[i]);
  }
  acPrefix[i] = 0;

  if (strcasecmp(pcPoolType, "POOL2STRING") == 0)
  {
    acSuffix[0] = 0;
  }
  else
  {
    strcpy(acSuffix, "-pool");
  }

  fprintf(pFile, "/*\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " * $%s: %s%s.h,v custom unknown Exp $\n", "Id", acPrefix, acSuffix);
  fprintf(pFile, " *\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " * Copyright 2003-2007 Klayton Monroe, All Rights Reserved.\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " */\n");
  fprintf(pFile, "\n");
  fprintf(pFile, "/*\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " * Defines\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " */\n");
  fprintf(pFile, "#define %s_POOL_SEED 0x%08lx\n", pcPoolType, ulFinal);
  fprintf(pFile, "#define %s_POOL_SIZE 0x%08x\n", pcPoolType, STRING2POOL_POOL_SIZE);
  fprintf(pFile, "#define %s_POOL_TAPS (%d) + (1)\n", pcPoolType, iTapsCount);
  fprintf(pFile, "\n\n");
  fprintf(pFile, "/*\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " * Macros\n");
  fprintf(pFile, " *\n");
  fprintf(pFile, " ***********************************************************************\n");
  fprintf(pFile, " */\n");
  fprintf(pFile, "#define %s_NEW_POOL(aucPool, iLength, ulState)\\\n", pcPoolType);
  fprintf(pFile, "{\\\n");
  fprintf(pFile, "  int i, j;\\\n");
  fprintf(pFile, "  unsigned long ul = ulState;\\\n");
  fprintf(pFile, "  for (i = 0; i < iLength; i++)\\\n");
  fprintf(pFile, "  {\\\n");
  fprintf(pFile, "    for (j = 0, aucPool[i] = 0; j < 8; j++)\\\n");
  fprintf(pFile, "    {\\\n");
  fprintf(pFile, "      aucPool[i] |= (ul & 1) << j;\\\n");
  fprintf(pFile, "      ul = ((((ul >> 7) ^ (ul >> 6) ^ (ul >> 2) ^ (ul >> 0)) & 1) << 31) | (ul >> 1);\\\n");
  fprintf(pFile, "    }\\\n");
  fprintf(pFile, "  }\\\n");
  fprintf(pFile, "}\n");
  fprintf(pFile, "\n");
  fprintf(pFile, "#define %s_TAP_POOL(aucTaps, aucPool)\\\n", pcPoolType);
  fprintf(pFile, "{\\\n");
  if (iTapsCount > 0)
  {
    fprintf(pFile, "  if (%s_POOL_SEED == 0x%08lx && %s_POOL_SIZE == 0x%08x && %s_POOL_TAPS == %d)\\\n",
            pcPoolType,
            ulFinal,
            pcPoolType,
            STRING2POOL_POOL_SIZE,
            pcPoolType,
            iTapsCount + 1
           );
    fprintf(pFile, "  {\\\n");
    for (i = 0; piTaps && i < iTapsCount; i++)
    {
      fprintf(pFile, "    aucTaps[%d] = aucPool[%d];\\\n", i, piTaps[i]);
      fprintf(pFile, "    aucPool[%d] = 0;\\\n", i);
    }
    fprintf(pFile, "    aucTaps[%d] = 0;\\\n", i);
    fprintf(pFile, "  }\\\n");
    fprintf(pFile, "  else\\\n");
    fprintf(pFile, "  {\\\n");
    fprintf(pFile, "    aucTaps[0] = 0;\\\n");
    fprintf(pFile, "  }\\\n");
  }
  else
  {
    fprintf(pFile, "  aucTaps[0] = 0;\\\n");
  }
  fprintf(pFile, "}\n");
  return;
}


/*-
 ***********************************************************************
 *
 * Usage
 *
 ***********************************************************************
 */
void
Usage()
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s --default prefix\n", PROGRAM);
  fprintf(stderr, "       %s --defined prefix seed string\n", PROGRAM);
  fprintf(stderr, "\n");
  exit(1);
}


/*-
 ***********************************************************************
 *
 * Main
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  const char          acRoutine[] = "Main()";
  char                acPrefix[STRING2POOL_PREFIX_SIZE];
  char               *pcEnd;
  char               *pcString;
  int                 i;
  int                 iDone;
  int                 iStringLength;
  int                 iStringOffset;
  int                 iTries;
  int                *piTaps;
  unsigned char       aucPool[STRING2POOL_POOL_SIZE];
  unsigned long       ulSeed;
  unsigned long       ulFinal;
  unsigned long       ulState;

  /*-
   *********************************************************************
   *
   * Seed the random number generator.
   *
   *********************************************************************
   */
#ifdef WIN32
  ulSeed = (unsigned long) GetTickCount() ^ (unsigned long) time(NULL);
  srand(ulSeed);
#else
  ulSeed = (((unsigned long) getpid()) << 16) ^ (unsigned long) time(NULL);
  srandom(ulSeed);
#endif

  /*-
   *********************************************************************
   *
   * Process arguments.
   *
   *********************************************************************
   */
  if (iArgumentCount == 3)
  {
    if (strcasecmp(ppcArgumentVector[1], "--default") == 0)
    {
      if (strcasecmp(ppcArgumentVector[2], "HTTP") == 0)
      {
        strcpy(acPrefix, "HTTP");
      }
      else if (strcasecmp(ppcArgumentVector[2], "POOL") == 0)
      {
        strcpy(acPrefix, "POOL2STRING");
      }
      else if (strcasecmp(ppcArgumentVector[2], "SSL") == 0)
      {
        strcpy(acPrefix, "SSL");
      }
      else
      {
        fprintf(stderr, "%s: Prefix='%s': Prefix must be one of 'HTTP', 'POOL', or 'SSL'.\n", acRoutine, ppcArgumentVector[2]);
        return 2;
      }
      ConstructIncludeFile(stdout, acPrefix, 0, NULL, 0);
      return 0;
    }
    else
    {
      Usage();
    }
  }
  else if (iArgumentCount == 5)
  {
    if (strcasecmp(ppcArgumentVector[1], "--defined") == 0)
    {
      if (strcasecmp(ppcArgumentVector[2], "HTTP") == 0)
      {
        strcpy(acPrefix, "HTTP");
      }
      else if (strcasecmp(ppcArgumentVector[2], "POOL") == 0)
      {
        strcpy(acPrefix, "POOL2STRING");
      }
      else if (strcasecmp(ppcArgumentVector[2], "SSL") == 0)
      {
        strcpy(acPrefix, "SSL");
      }
      else
      {
        fprintf(stderr, "%s: Prefix='%s': Prefix must be one of 'HTTP', 'POOL', or 'SSL'.\n", acRoutine, ppcArgumentVector[2]);
        return 2;
      }
      if (ppcArgumentVector[3][0] != 0)
      {
        ulState = strtoul(ppcArgumentVector[3], &pcEnd, 16);
        if (*pcEnd != 0 || errno == ERANGE)
        {
          fprintf(stderr, "%s: Seed='%s': Value could not be converted to a 32 bit hex number.\n", acRoutine, ppcArgumentVector[3]);
          return 3;
        }
        if (ulState == 0)
        {
#ifdef WIN32
          ulState = (unsigned long) ((GetTickCount() << 16) ^ rand());
#else
          ulState = (unsigned long) random();
#endif
        }
      }
      else
      {
        fprintf(stderr, "%s: Seed='%s': Value must not be null.\n", acRoutine, ppcArgumentVector[3]);
        return 4;
      }
      pcString = ppcArgumentVector[4];
      iStringLength = strlen(pcString);
      if (iStringLength > (STRING2POOL_POOL_SIZE / 1024))
      {
        fprintf(stderr, "%s: PoolSize='%d' String='%s' Length='%d': Required Pool size is (Length * 1024).\n", acRoutine, STRING2POOL_POOL_SIZE, ppcArgumentVector[4], iStringLength);
        return 5;
      }
      piTaps = malloc(iStringLength * sizeof(int));
      if (piTaps == NULL)
      {
        fprintf(stderr, "%s: malloc(): %s\n", acRoutine, strerror(errno));
        return 6;
      }
    }
    else
    {
      Usage();
    }
  }
  else
  {
    Usage();
  }

  /*-
   *********************************************************************
   *
   * Locate a pool that contains the specified string.
   *
   *********************************************************************
   */
  for (iDone = iTries = iStringOffset = 0; !iDone && iTries < STRING2POOL_MAX_TRIES; iTries++, iStringOffset = 0)
  {
    ulFinal = ulState;
    STRING2POOL_NEW_POOL(aucPool, STRING2POOL_POOL_SIZE, ulState);
    for (i = 0; i < STRING2POOL_POOL_SIZE && iStringOffset < iStringLength; i++)
    {
      if (aucPool[i] == pcString[iStringOffset])
      {
        piTaps[iStringOffset++] = i;
      }
    }
    iDone = (iStringOffset == iStringLength) ? 1 : 0;
  }
  if (iTries >= STRING2POOL_MAX_TRIES)
  {
    fprintf(stderr, "%s: Tries='%d': Failed to locate a suitable pool within the given number of tries.\n", acRoutine, iTries);
    return 7;
  }

  /*-
   *********************************************************************
   *
   * Construct an include file and write it out.
   *
   *********************************************************************
   */
  ConstructIncludeFile(stdout, acPrefix, ulFinal, piTaps, iStringLength);

  return 0;
}
