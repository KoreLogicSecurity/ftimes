/*-
 ***********************************************************************
 *
 * $Id: tarmap.c,v 1.4 2007/02/23 00:22:43 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2007 The FTimes Project, All Rights Reserved.
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
static TARMAP_PROPERTIES *gpsProperties;

/*-
 ***********************************************************************
 *
 * TarMapMain
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  const char          acRoutine[] = "Main()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
  TARMAP_PROPERTIES  *psProperties = NULL;

  /*-
   *********************************************************************
   *
   * Punch in and go to work.
   *
   *********************************************************************
   */
  iError = TarMapBootStrap(acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, acRoutine, acLocalError);
    TarMapShutdown(XER_BootStrap);
  }
  psProperties = (TARMAP_PROPERTIES *) TarMapGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Process command line arguments.
   *
   *********************************************************************
   */
  iError = TarMapProcessArguments(iArgumentCount, ppcArgumentVector, psProperties, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, acRoutine, acLocalError);
    TarMapShutdown(XER_ProcessArguments);
  }

  /*-
   *********************************************************************
   *
   * Answer the mail.
   *
   *********************************************************************
   */
  switch (psProperties->iRunMode)
  {
  case TARMAP_MAP:
    iError = TarMapWorkHorse(psProperties, acLocalError);
    if (iError != ER_OK)
    {
      fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, acRoutine, acLocalError);
      TarMapShutdown(XER_WorkHorse);
    }
    break;
  default:
    fprintf(stderr, "%s: %s: Invalid run mode (%d). That shouldn't happen!\n", PROGRAM_NAME, acRoutine, psProperties->iRunMode);
    TarMapShutdown(XER_RunMode);
    break;
  }

  /*-
   *********************************************************************
   *
   * Shutdown and go home.
   *
   *********************************************************************
   */
  TarMapShutdown(XER_OK);

  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * TarMapBootStrap
 *
 ***********************************************************************
 */
int
TarMapBootStrap(char *pcError)
{
  const char          acRoutine[] = "TarMapBootStrap()";
  char                acLocalError[MESSAGE_SIZE] = "";
  TARMAP_PROPERTIES  *psProperties = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and initialize the properties structure.
   *
   *********************************************************************
   */
  psProperties = (TARMAP_PROPERTIES *) TarMapNewProperties(acLocalError);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  TarMapSetPropertiesReference(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * TarMapFreeProperties
 *
 ***********************************************************************
 */
void
TarMapFreeProperties(TARMAP_PROPERTIES *psProperties)
{
  if (psProperties != NULL)
  {
    if (psProperties->pucTarFileHeader != NULL)
    {
      free(psProperties->pucTarFileHeader);
    }
    if (psProperties->pucTarNullHeader != NULL)
    {
      free(psProperties->pucTarNullHeader);
    }
    free(psProperties);
  }
}


/*-
 ***********************************************************************
 *
 * TarMapGetPropertiesReference
 *
 ***********************************************************************
 */
TARMAP_PROPERTIES *
TarMapGetPropertiesReference(void)
{
  return gpsProperties;
}


/*-
 ***********************************************************************
 *
 * TarMapNewProperties
 *
 ***********************************************************************
 */
TARMAP_PROPERTIES *
TarMapNewProperties(char *pcError)
{
  const char          acRoutine[] = "TarMapNewProperties()";
  TARMAP_PROPERTIES     *psProperties = NULL;

  psProperties = (TARMAP_PROPERTIES *) calloc(sizeof(TARMAP_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  psProperties->pucTarFileHeader = (unsigned char *) calloc(1, TARMAP_UNIT_SIZE);
  if (psProperties->pucTarFileHeader == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  psProperties->pucTarNullHeader = (unsigned char *) calloc(1, TARMAP_UNIT_SIZE);
  if (psProperties->pucTarNullHeader == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * TarMapProcessArguments
 *
 ***********************************************************************
 */
int
TarMapProcessArguments(int iArgumentCount, char *ppcArgumentVector[], TARMAP_PROPERTIES *psProperties, char *pcError)
{
  int                 iComplete = 0;

  if (iArgumentCount < 2)
  {
    TarMapUsage();
  }

  iArgumentCount -= 2;

  if (strcmp(ppcArgumentVector[1], "-m") == 0 || strcmp(ppcArgumentVector[1], "--map") == 0)
  {
    psProperties->iRunMode = TARMAP_MAP;
    if (iArgumentCount == 2)
    {
      if (strcmp(ppcArgumentVector[2], "-f") == 0 || strcmp(ppcArgumentVector[2], "--file") == 0)
      {
        psProperties->pcTarFile = ppcArgumentVector[3];
        iComplete = 1;
      }
    }
  }
  else if (strcmp(ppcArgumentVector[1], "-v") == 0 || strcmp(ppcArgumentVector[1], "--version") == 0)
  {
    TarMapVersion();
  }

  if (!iComplete)
  {
    TarMapUsage();
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * TarMapReadHeader
 *
 ***********************************************************************
 */
int
TarMapReadHeader(FILE *pFile, unsigned char *pucHeader, char *pcError)
{
  const char          acRoutine[] = "TarMapReadHeader()";
  int                 iNRead = 0;

  /*-
   *********************************************************************
   *
   * Read one header, and return the number of bytes read or an error.
   *
   *********************************************************************
   */
  memset(pucHeader, 0, TARMAP_UNIT_SIZE);
  iNRead = fread(pucHeader, 1, TARMAP_UNIT_SIZE, pFile);
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fread(): %s", acRoutine, strerror(errno));
    return ER;
  }

  return iNRead;
}


/*-
 ***********************************************************************
 *
 * TarMapReadPadding
 *
 ***********************************************************************
 */
int
TarMapReadPadding(FILE *pFile, unsigned long ulSize, char *pcError)
{
  const char          acRoutine[] = "TarMapReadPadding()";
  char                aucPadding[TARMAP_UNIT_SIZE];
  int                 iNRead = 0;
  int                 iPad = 0;
  int                 iRemainder = (ulSize % TARMAP_UNIT_SIZE);

  /*-
   *********************************************************************
   *
   * Each file in a tar archive is padded out to a block boundary.
   * This padding needs to be burned off, so we can process the next
   * header. A read is done because the input stream may be stdin.
   *
   *********************************************************************
   */
  if (ulSize > 0 && iRemainder > 0)
  {
    iPad = TARMAP_UNIT_SIZE - iRemainder;
    iNRead = fread(aucPadding, 1, iPad, pFile);
    if (ferror(pFile))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fread(): %s", acRoutine, strerror(errno));
      return ER;
    }
    if (iNRead != iPad)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fread(): Wanted %d bytes, but got %d instead.", acRoutine, iPad, iNRead);
      return ER;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * TarMapSetPropertiesReference
 *
 ***********************************************************************
 */
void
TarMapSetPropertiesReference(TARMAP_PROPERTIES *psProperties)
{
  gpsProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * TarMapShutdown
 *
 ***********************************************************************
 */
void
TarMapShutdown(int iError)
{
  TarMapFreeProperties(TarMapGetPropertiesReference());
  exit(iError);
}


/*-
 ***********************************************************************
 *
 * TarMapUsage
 *
 ***********************************************************************
 */
void
TarMapUsage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s {-m|--map} {-f|--file} {tar-file|-}\n", PROGRAM_NAME);
  fprintf(stderr, "       %s {-v|--version}\n", PROGRAM_NAME);
  fprintf(stderr, "\n");
  exit(XER_Usage);
}


/*-
 ***********************************************************************
 *
 * TarMapVersion
 *
 ***********************************************************************
 */
void
TarMapVersion(void)
{
  fprintf(stdout, "%s %s %d-bit\n", PROGRAM_NAME, VERSION, (int) (sizeof(&TarMapVersion) * 8));
  exit(XER_OK);
}


/*-
 ***********************************************************************
 *
 * TarMapWorkHorse
 *
 ***********************************************************************
 */
int
TarMapWorkHorse(TARMAP_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "TarMapWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcFile = psProperties->pcTarFile;
  FILE               *pFile = NULL;
  int                 i = 0;
  int                 iLength = 0;
  int                 iNLeft = 0;
  int                 iNRead = 0;
  int                 iNToRead = 0;
  MD5_CONTEXT         sMd5 = { 0 };
  SHA1_CONTEXT        sSha1 = { 0 };
  TAR_HEADER         *psTar = NULL;
  unsigned char       aucData[TARMAP_READ_SIZE] = { 0 };
  unsigned char       aucMd5[MD5_HASH_SIZE] = { 0 };
  unsigned char       aucSha1[SHA1_HASH_SIZE] = { 0 };
  unsigned char      *pucHeader = psProperties->pucTarFileHeader;
  unsigned long       ulSize = 0;

  /*-
   *********************************************************************
   *
   * Open the tar file, or prepare to read from stdin.
   *
   *********************************************************************
   */
  if (strcmp(pcFile, "-") == 0)
  {
    pFile = stdin;
  }
  else
  {
    pFile = fopen(pcFile, "rb");
    if (pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s], %s", acRoutine, pcFile, strerror(errno));
      return ER;
    }
  }

  /*-
   *********************************************************************
   *
   * Write out a header.
   *
   *********************************************************************
   */
  fprintf(stdout, "name|size|md5|sha1\n");

  /*-
   *********************************************************************
   *
   * Loop through the records. Write out map data as you go.
   *
   *********************************************************************
   */
  while (!feof(pFile))
  {
    /*-
     *******************************************************************
     *
     * Read the next header.
     *
     *******************************************************************
     */
    iNRead = TarMapReadHeader(pFile, pucHeader, acLocalError);
    if (iNRead == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Check for a runt header. This should not happen.
     *
     *******************************************************************
     */
    if (iNRead < TARMAP_UNIT_SIZE)
    {
#ifdef TARMAP_DEBUG
      fprintf(stderr, "Warning='Runt Header: Wanted %d bytes -- got %d.'\n", TARMAP_UNIT_SIZE, iNRead);
#endif
      psProperties->iRuntHeaderCount++;
      continue;
    }

    /*-
     *******************************************************************
     *
     * Check for a null header. This should signify that we're done or
     * very close to being done. There shouldn't be any more headers in
     * the input stream, but keep going just to be sure.
     *
     *******************************************************************
     */
    if (memcmp(pucHeader, psProperties->pucTarNullHeader, sizeof(TAR_HEADER)) == 0)
    {
#ifdef TARMAP_DEBUG
      fprintf(stderr, "Warning='Null Header: Wanted %d bytes -- got %d.'\n", TARMAP_UNIT_SIZE, iNRead);
#endif
      psProperties->iNullHeaderCount++;
      continue;
    }

    /*-
     *******************************************************************
     *
     * Check header magic. This should work for POSIX, STAR, GNUTAR,
     * and OLDGNUTAR. Other implementations are not yet supported.
     *
     *******************************************************************
     */
    psTar = (TAR_HEADER *) pucHeader;
    if (memcmp(psTar->magic, TAR_MAGIC, TAR_MAGIC_LENGTH - 1) != 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Header magic != \"ustar\"", acRoutine, pcFile);
      return ER;
    }
#ifdef TARMAP_DEBUG
    fprintf(stderr, "Warning='Good Header: Wanted %d bytes -- got %d.'\n", TARMAP_UNIT_SIZE, iNRead);
#endif
    psProperties->iGoodHeaderCount++;

    /*-
     *******************************************************************
     *
     * Chop off trailing '/'s. This should only affect directories.
     *
     *******************************************************************
     */
    iLength = strlen(psTar->name);
    while (psTar->name[iLength - 1] == '/')
    {
      psTar->name[--iLength] = 0;
    }

    /*-
     *******************************************************************
     *
     * Harvest and print the various attributes for this record.
     *
     *******************************************************************
     */
    ulSize = strtoul(psTar->size, NULL, 8);
    switch (psTar->typeflag)
    {
    case DIRTYPE:
      fprintf(stdout, "\"%s\"|%lu|DIRECTORY|DIRECTORY\n", psTar->name, ulSize);
      break;
    case LNKTYPE:
      fprintf(stdout, "\"%s\"|%lu|HARDLINK to %s|HARDLINK to %s\n", psTar->name, ulSize, psTar->linkname, psTar->linkname);
      break;
    case SYMTYPE:
      fprintf(stdout, "\"%s\"|%lu", psTar->name, ulSize);
      MD5HashString((unsigned char *) psTar->linkname, strlen(psTar->linkname), aucMd5);
      for (i = 0; i < MD5_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucMd5[i]);
      }
      fprintf(stdout, "|");
      SHA1HashString((unsigned char *) psTar->linkname, strlen(psTar->linkname), aucSha1);
      for (i = 0; i < SHA1_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucSha1[i]);
      }
      fprintf(stdout, "\n");
      break;
    case REGTYPE:
      iNLeft = ulSize;
      MD5Alpha(&sMd5);
      SHA1Alpha(&sSha1);
      while (iNLeft > 0)
      {
        iNToRead = (iNLeft < TARMAP_READ_SIZE) ? iNLeft : TARMAP_READ_SIZE;
        iNRead = fread(aucData, 1, iNToRead, pFile);
        if (ferror(pFile))
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: fread(): File = [%s], %s", acRoutine, pcFile, strerror(errno));
          return ER;
        }
        if (iNRead != iNToRead)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: fread(): File = [%s], Wanted %d bytes, but got %d instead.", acRoutine, pcFile, iNToRead, iNRead);
          return ER;
        }
        MD5Cycle(&sMd5, aucData, iNRead);
        SHA1Cycle(&sSha1, aucData, iNRead);
        iNLeft -= iNRead;
      }
      MD5Omega(&sMd5, aucMd5);
      SHA1Omega(&sSha1, aucSha1);
      fprintf(stdout, "\"%s\"|%lu|", psTar->name, ulSize);
      for (i = 0; i < MD5_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucMd5[i]);
      }
      fprintf(stdout, "|");
      for (i = 0; i < SHA1_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucSha1[i]);
      }
      fprintf(stdout, "\n");
      if (TarMapReadPadding(pFile, ulSize, acLocalError) == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
      break;
    case CHRTYPE:
    case BLKTYPE:
    case FIFOTYPE:
      fprintf(stdout, "\"%s\"|%lu|SPECIAL|SPECIAL\n", psTar->name, ulSize);
      break;
/* FIXME Figure out how to support these types. */
    case XHDTYPE:
    case XGLTYPE:
    case GNUTYPE_DUMPDIR:  
    case GNUTYPE_LONGLINK:   
    case GNUTYPE_LONGNAME:
    case GNUTYPE_MULTIVOL:
    case GNUTYPE_NAMES:
    case GNUTYPE_SPARSE:
    case GNUTYPE_VOLHDR:
    case SOLARIS_XHDTYPE:
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Unknown or unsupprted type (%d/0x%02x) for %s.", acRoutine, pcFile, psTar->typeflag, psTar->typeflag, psTar->name);
      return ER;
      break;
    }
  }

#ifdef TARMAP_DEBUG
  fprintf(stderr, "GoodHeaderCount=%d\n", psProperties->iGoodHeaderCount);
  fprintf(stderr, "NullHeaderCount=%d\n", psProperties->iNullHeaderCount);
  fprintf(stderr, "RuntHeaderCount=%d\n", psProperties->iRuntHeaderCount);
#endif

  return ER_OK;
}
