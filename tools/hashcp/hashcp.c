/*-
 ***********************************************************************
 *
 * $Id: hashcp.c,v 1.8 2012/01/04 03:12:39 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * HashCpMain
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  APP_UI64            ui64TotalWritten = 0;
  char                acMd5Hash[HASHCP_MAX_MD5_LENGTH] = { 0 };
  char                acSha1Hash[HASHCP_MAX_SHA1_LENGTH] = { 0 };
  char               *pcTargetFile = NULL;
  char               *pcSourceFile = NULL;
  FILE               *pTargetFile = NULL;
  FILE               *pSourceFile = NULL;
  int                 iNRead = 0;
  int                 iNWritten = 0;
  MD5_CONTEXT         sMd5Context = { 0 };
  SHA1_CONTEXT        sSha1Context = { 0 };
  unsigned char       aucData[HASHCP_READ_SIZE] = { 0 };
  unsigned char       aucMd5[MD5_HASH_SIZE] = { 0 };
  unsigned char       aucSha1[SHA1_HASH_SIZE] = { 0 };

  if (iArgumentCount != 3)
  {
    fprintf(stderr, "\nUsage: %s {source|-} target\n\n", PROGRAM_NAME);
    return XER_Usage;
  }

  pcSourceFile = ppcArgumentVector[1];
  pcTargetFile = ppcArgumentVector[2];

  if (strcmp(pcSourceFile, "-") == 0)
  {
    pcSourceFile = HASHCP_STDIN;
    pSourceFile = stdin;
  }
  else
  {
    if ((pSourceFile = fopen(pcSourceFile, "rb")) == NULL)
    {
      fprintf(stderr, "%s: Error='Unable to open \"%s\" (%s).'\n", PROGRAM_NAME, pcSourceFile, strerror(errno));
      goto ABORT;
    }
  }

  if (strcmp(pcTargetFile, "-") == 0)
  {
    fprintf(stderr, "%s: Error='Target file can not be stdout (i.e., \"-\").'\n", PROGRAM_NAME);
    goto ABORT;
  }

  if ((pTargetFile = fopen(pcTargetFile, "wb")) == NULL)
  {
    fprintf(stderr, "%s: Error='Unable to open \"%s\" (%s).'\n", PROGRAM_NAME, pcTargetFile, strerror(errno));
    goto ABORT;
  }

  MD5Alpha(&sMd5Context);
  SHA1Alpha(&sSha1Context);

  while ((iNRead = fread(aucData, 1, HASHCP_READ_SIZE, pSourceFile)) == HASHCP_READ_SIZE)
  {
    MD5Cycle(&sMd5Context, aucData, iNRead);
    SHA1Cycle(&sSha1Context, aucData, iNRead);
    iNWritten = fwrite(aucData, 1, iNRead, pTargetFile);
    ui64TotalWritten += (APP_UI64) iNWritten;
    if (iNWritten != iNRead)
    {
      fprintf(stderr, "%s: Error='Unable to write \"%s\" (%s).'\n", PROGRAM_NAME, pcTargetFile, strerror(errno));
      goto ABORT_UNLINK;
    }
  }

  if (iNRead < HASHCP_READ_SIZE && ferror(pSourceFile))
  {
    fprintf(stderr, "%s: Error='Unable to read \"%s\" (%s).'\n", PROGRAM_NAME, pcSourceFile, strerror(errno));
    goto ABORT_UNLINK;
  }
  else
  {
    MD5Cycle(&sMd5Context, aucData, iNRead);
    SHA1Cycle(&sSha1Context, aucData, iNRead);
    iNWritten = fwrite(aucData, 1, iNRead, pTargetFile);
    if (iNWritten != iNRead)
    {
      fprintf(stderr, "%s: Error='Unable to write \"%s\" (%s)'\n", PROGRAM_NAME, pcTargetFile, strerror(errno));
      goto ABORT_UNLINK;
    }
    ui64TotalWritten += (APP_UI64) iNWritten;
  }
  fclose(pSourceFile);
  fclose(pTargetFile);

  MD5Omega(&sMd5Context, aucMd5);
  MD5HashToHex(aucMd5, acMd5Hash);

  SHA1Omega(&sSha1Context, aucSha1);
  SHA1HashToHex(aucSha1, acSha1Hash);

  fprintf(stdout, "name|size|md5|sha1\n");
  fprintf(stdout, "\"%s\"|%llu|%s|%s\n",
    pcTargetFile,
    (unsigned long long) ui64TotalWritten,
    acMd5Hash,
    acSha1Hash
    );

  return XER_OK;

ABORT_UNLINK:
  unlink(pcTargetFile);
ABORT:
  return XER_Abort;
}
