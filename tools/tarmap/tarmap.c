/*-
 ***********************************************************************
 *
 * $Id: tarmap.c,v 1.28 2013/02/14 16:55:23 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2013 The FTimes Project, All Rights Reserved.
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
 * TarMapNeuterString
 *
 ***********************************************************************
 */
char *
TarMapNeuterString(char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "TarMapNeuterString()";
  char               *pcNeutered;
  int                 i;
  int                 n;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcNeutered = malloc((3 * iLength) + 1);
  if (pcNeutered == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcNeutered[0] = 0;

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
    if (pcData[i] > '~' || pcData[i] < ' ')
    {
      n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
    }
    else
    {
      switch (pcData[i])
      {
      case '|':
      case '"':
      case '\'':
      case '`':
      case '%':
      case '+':
      case '#':
        n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
        break;
      case ' ':
        pcNeutered[n++] = '+';
        break;
      default:
        pcNeutered[n++] = pcData[i];
        break;
      }
    }
  }
  pcNeutered[n] = 0;

  return pcNeutered;
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
 * TarMapReadLongName
 *
 ***********************************************************************
 */
char *
TarMapReadLongName(FILE *pFile, APP_UI64 ui64Size, char *pcError)
{
  const char          acRoutine[] = "TarMapReadLongName()";
  char               *pcName = NULL;
  int                 iNRead = 0;
  int                 iNBlocks = 0;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Bail out if the length does not pass muster.
   *
   *********************************************************************
   */
  if (ui64Size <= 0 || ui64Size > TARMAP_MAX_PATH)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length (%llu) does not pass muster. That shouldn't happen!", acRoutine, (unsigned long long) ui64Size);
    return NULL;
  }
  iLength = (int) ui64Size;

  /*-
   *********************************************************************
   *
   * Determine the number of blocks to read, and allocate some memory
   * to hold the name. Be sure to handle the case where the length is
   * an exact multiple of TARMAP_UNIT_SIZE.
   *
   *********************************************************************
   */
  if ((iLength % TARMAP_UNIT_SIZE) == 0)
  {
    iNBlocks = (iLength / TARMAP_UNIT_SIZE);
  }
  else
  {
    iNBlocks = (iLength / TARMAP_UNIT_SIZE) + 1;
  }

  pcName = calloc(TARMAP_UNIT_SIZE, iNBlocks);
  if (pcName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Read the name, and terminate it one byte short. Based on tar
   * balls examined to date, it seems that the length is always two
   * bytes longer than it should be. This needs more investigation.
   *
   *********************************************************************
   */
  iNRead = fread(pcName, TARMAP_UNIT_SIZE, iNBlocks, pFile);
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fread(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  if (iNRead != iNBlocks)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fread(): Wanted %d blocks, but got %d instead.", acRoutine, iNBlocks, iNRead);
    return NULL;
  }
  pcName[iLength - 1] = 0;

  return pcName;
}


/*-
 ***********************************************************************
 *
 * TarMapReadPadding
 *
 ***********************************************************************
 */
int
TarMapReadPadding(FILE *pFile, APP_UI64 ui64Size, char *pcError)
{
  const char          acRoutine[] = "TarMapReadPadding()";
  char                aucPadding[TARMAP_UNIT_SIZE];
  int                 iNRead = 0;
  int                 iPad = 0;
  int                 iRemainder = (int) (ui64Size % TARMAP_UNIT_SIZE);

  /*-
   *********************************************************************
   *
   * Each file in a tar archive is padded out to a block boundary.
   * This padding needs to be burned off, so we can process the next
   * header. A read is done because the input stream may be stdin.
   *
   *********************************************************************
   */
  if (ui64Size > 0 && iRemainder > 0)
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
  char                acLink[TARMAP_NAME_SIZE + 1];
  char                acName[TARMAP_NAME_SIZE + 1];
  char                acPrefix[TARMAP_PREFIX_SIZE + 1];
  char               *pcFile = psProperties->pcTarFile;
  char               *pcLink = NULL;
  char               *pcName = NULL;
  char               *pcNeuteredName = NULL;
  char               *pcNeuteredPrefix = NULL;
  FILE               *pFile = NULL;
  int                 i = 0;
  int                 iEndOfArchive = 0;
  int                 iLastHeaderWasNull = 0;
//int                 iLinkLength = 0;
  int                 iLongLink = 0;
  int                 iLongName = 0;
  int                 iNameLength = 0;
  int                 iNeutered = 0;
  int                 iNRead = 0;
  int                 iNToRead = 0;
  int                 iPrefixLength = 0;
  APP_UI32            ui32Gid = 0;
  APP_UI32            ui32Mode = 0;
  APP_UI32            ui32Uid = 0;
  APP_UI64            ui64NLeft = 0;
  APP_UI64            ui64Size = 0;
  MD5_CONTEXT         sMd5 = { 0 };
  SHA1_CONTEXT        sSha1 = { 0 };
  TAR_HEADER         *psTar = NULL;
  unsigned char       aucData[TARMAP_READ_SIZE] = { 0 };
  unsigned char       aucMd5[MD5_HASH_SIZE] = { 0 };
  unsigned char       aucSha1[SHA1_HASH_SIZE] = { 0 };
  unsigned char      *pucHeader = psProperties->pucTarFileHeader;

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
  fprintf(stdout, "name|mode|uid|gid|size|md5|sha1\n");

  /*-
   *********************************************************************
   *
   * Loop through the records. Write out map data as you go.
   *
   *********************************************************************
   */
  while (!feof(pFile) && !iEndOfArchive)
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
      if (iLastHeaderWasNull)
      {
        iEndOfArchive = 1;
      }
      iLastHeaderWasNull = 1;
      continue;
    }
    iLastHeaderWasNull = 0;

    /*-
     *******************************************************************
     *
     * Check header magic. This should work for POSIX, STAR, GNUTAR,
     * and OLDGNUTAR. Other implementations are not yet supported.
     *
     *******************************************************************
     */
    psTar = (TAR_HEADER *) pucHeader;
    if (memcmp(psTar->magic, TAR_MAGIC, TAR_MAGIC_LENGTH) == 0)
    {
      psProperties->iTarType = TAR_TYPE_POSIX;
    }
    else if (memcmp(psTar->magic, TAR_OLD_GNU_MAGIC, TAR_OLD_GNU_MAGIC_LENGTH) == 0)
    {
      psProperties->iTarType = TAR_TYPE_OLD_GNU;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Invalid or unsupported header magic.", acRoutine, pcFile);
      return ER;
    }
#ifdef TARMAP_DEBUG
    fprintf(stderr, "Warning='Good Header: Wanted %d bytes -- got %d.'\n", TARMAP_UNIT_SIZE, iNRead);
#endif
    psProperties->iGoodHeaderCount++;

    /*-
     *******************************************************************
     *
     * Copy the prefix into a local buffer. Values exactly 155 bytes
     * long will not be null terminated in the tar header. To handle
     * this scenario, the destination buffer is 1 byte larger than
     * TARMAP_PREFIX_SIZE, and the extra byte is always set to zero.
     *
     *******************************************************************
     */
    if (psProperties->iTarType == TAR_TYPE_POSIX)
    {
      strncpy(acPrefix, psTar->prefix, TARMAP_PREFIX_SIZE);
      acPrefix[TARMAP_PREFIX_SIZE] = 0;
    }
    else
    {
      acPrefix[0] = 0;
    }
    iPrefixLength = strlen(acPrefix);
    pcNeuteredPrefix = TarMapNeuterString(acPrefix, iPrefixLength, acLocalError);
    if (pcNeuteredPrefix == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFile, acLocalError);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Copy the name into a local buffer. Values exactly 100 bytes
     * long will not be null terminated in the tar header. To handle
     * this scenario, the destination buffer is 1 byte larger than
     * TARMAP_NAME_SIZE, and the extra byte is always set to zero.
     *
     *******************************************************************
     */
    strncpy(acName, psTar->name, TARMAP_NAME_SIZE);
    acName[TARMAP_NAME_SIZE] = 0;
    pcName = acName;
    iNameLength = strlen(pcName);

    /*-
     *******************************************************************
     *
     * Copy the link into a local buffer. Values exactly 100 bytes
     * long will not be null terminated in the tar header. To handle
     * this scenario, the destination buffer is 1 byte larger than
     * TARMAP_NAME_SIZE, and the extra byte is always set to zero.
     *
     *******************************************************************
     */
    strncpy(acLink, psTar->linkname, TARMAP_NAME_SIZE);
    acLink[TARMAP_NAME_SIZE] = 0;
    pcLink = acLink;
//  iLinkLength = strlen(pcLink);

/* FIXME See if there's a way to remove the following code/label (see comment below). */
    iLongName = 0;
    iLongLink = 0;
    iNeutered = 0;
LONGLINK_TAKE2:
LONGNAME_TAKE2:

    /*-
     *******************************************************************
     *
     * Chop off trailing '/'s. This should only affect directories.
     *
     *******************************************************************
     */
    while (pcName[iNameLength - 1] == '/')
    {
      pcName[--iNameLength] = 0;
    }

    /*-
     *******************************************************************
     *
     * Neuter or reneuter (as the case may be) the given name.
     *
     *******************************************************************
     */
    if (iNeutered && pcNeuteredName)
    {
      free(pcNeuteredName);
    }
    pcNeuteredName = TarMapNeuterString(pcName, iNameLength, acLocalError);
    if (pcNeuteredName == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFile, acLocalError);
      return ER;
    }
    iNeutered = 1;

    /*-
     *******************************************************************
     *
     * Harvest and print the various attributes for this record.
     *
     *******************************************************************
     */
    /* mode */
    ui32Mode = strtoul(psTar->mode, NULL, 8);
    switch (psTar->typeflag)
    {
    case FIFOTYPE:
      ui32Mode |= 0010000;
      break;
    case CHRTYPE:
      ui32Mode |= 0020000;
      break;
    case DIRTYPE:
      ui32Mode |= 0040000;
      break;
    case BLKTYPE:
      ui32Mode |= 0060000;
      break;
    case REGTYPE:
      ui32Mode |= 0100000;
      break;
    case SYMTYPE:
      ui32Mode |= 0120000;
      break;
    }
    /* uid */
    ui32Uid = strtoul(psTar->gid, NULL, 8);
    /* gid */
    ui32Gid = strtoul(psTar->gid, NULL, 8);
    /* size */
    if ((unsigned char) psTar->size[0] == 0x80)
    {
      ui64Size = 0;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 4]) << 56;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 5]) << 48;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 6]) << 40;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 7]) << 32;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 8]) << 24;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[ 9]) << 16;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[10]) <<  8;
      ui64Size |= (APP_UI64) ((unsigned char) psTar->size[11]) <<  0;
    }
    else
    {
      ui64Size = strtoull(psTar->size, NULL, 8);
    }
    switch (psTar->typeflag)
    {
    case DIRTYPE:
      fprintf(stdout, "\"%s%s%s\"|%o|%d|%d|%llu|DIRECTORY|DIRECTORY\n",
        (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "",
        (pcNeuteredPrefix[0]) ? "/" : "",
        pcNeuteredName,
        ui32Mode,
        ui32Uid,
        ui32Gid,
        (unsigned long long) ui64Size
        );
      break;
    case LNKTYPE:
      fprintf(stdout, "\"%s%s%s\"|%o|%d|%d|%llu|HARDLINK to %s|HARDLINK to %s\n",
        (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "",
        (pcNeuteredPrefix[0]) ? "/" : "",
        pcNeuteredName,
        ui32Mode,
        ui32Uid,
        ui32Gid,
        (unsigned long long) ui64Size, pcLink, pcLink
        );
      break;
    case SYMTYPE:
      ui64Size = strlen(pcLink); /* The actual size for this type is the length of the link. */
      fprintf(stdout, "\"%s%s%s\"|%o|%d|%d|%llu|",
        (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "",
        (pcNeuteredPrefix[0]) ? "/" : "",
        pcNeuteredName,
        ui32Mode,
        ui32Uid,
        ui32Gid,
        (unsigned long long) ui64Size
        );
      MD5HashString((unsigned char *) pcLink, strlen(pcLink), aucMd5);
      for (i = 0; i < MD5_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucMd5[i]);
      }
      fprintf(stdout, "|");
      SHA1HashString((unsigned char *) pcLink, strlen(pcLink), aucSha1);
      for (i = 0; i < SHA1_HASH_SIZE; i++)
      {
        fprintf(stdout, "%02x", aucSha1[i]);
      }
      fprintf(stdout, "\n");
      break;
    case REGTYPE:
      ui64NLeft = ui64Size;
      MD5Alpha(&sMd5);
      SHA1Alpha(&sSha1);
      while (ui64NLeft > 0)
      {
        iNToRead = (ui64NLeft < TARMAP_READ_SIZE) ? (int) ui64NLeft : TARMAP_READ_SIZE;
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
        ui64NLeft -= iNRead;
      }
      MD5Omega(&sMd5, aucMd5);
      SHA1Omega(&sSha1, aucSha1);
      fprintf(stdout, "\"%s%s%s\"|%o|%d|%d|%llu|",
        (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "",
        (pcNeuteredPrefix[0]) ? "/" : "",
        pcNeuteredName,
        ui32Mode,
        ui32Uid,
        ui32Gid,
        (unsigned long long) ui64Size
        );
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
      if (TarMapReadPadding(pFile, ui64Size, acLocalError) == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
      break;
    case CHRTYPE:
    case BLKTYPE:
    case FIFOTYPE:
      fprintf(stdout, "\"%s%s%s\"|%o|%d|%d|%llu|SPECIAL|SPECIAL\n",
        (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "",
        (pcNeuteredPrefix[0]) ? "/" : "",
        pcNeuteredName,
        ui32Mode,
        ui32Uid,
        ui32Gid,
        (unsigned long long) ui64Size
        );
      break;
    case GNUTYPE_LONGLINK:
      pcLink = TarMapReadLongName(pFile, ui64Size, acLocalError);
      if (pcLink == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
//    iLinkLength = strlen(pcLink);
/* FIXME See if there's a way to do a continue here -- rather than jumping back to the middle of the loop. */
      iNRead = TarMapReadHeader(pFile, pucHeader, acLocalError);
      if (iNRead == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
      iLongLink = 1;
/* FIXME Need to validate the header before using it. */
      psTar = (TAR_HEADER *) pucHeader;
      if (iLongName == 0) /* If name was not set via LONGNAME, grab it now. */
      {
        strncpy(acName, psTar->name, TARMAP_NAME_SIZE);
        acName[TARMAP_NAME_SIZE] = 0;
        pcName = acName;
        iNameLength = strlen(pcName);
      }
      goto LONGLINK_TAKE2;
      break;
    case GNUTYPE_LONGNAME:
      pcName = TarMapReadLongName(pFile, ui64Size, acLocalError);
      if (pcName == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
      iNameLength = strlen(pcName);
/* FIXME See if there's a way to do a continue here -- rather than jumping back to the middle of the loop. */
      iNRead = TarMapReadHeader(pFile, pucHeader, acLocalError);
      if (iNRead == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], %s", acRoutine, pcFile, acLocalError);
        return ER;
      }
      iLongName = 1;
/* FIXME Need to validate the header before using it. */
      psTar = (TAR_HEADER *) pucHeader;
      if (iLongLink == 0) /* If linkname was not set via LONGLINK, grab it now. */
      {
        strncpy(acLink, psTar->linkname, TARMAP_NAME_SIZE);
        acLink[TARMAP_NAME_SIZE] = 0;
        pcLink = acLink;
//      iLinkLength = strlen(pcLink);
      }
      goto LONGNAME_TAKE2;
      break;

/* FIXME Figure out how to support these types. */
    case XHDTYPE:
    case XGLTYPE:
    case GNUTYPE_DUMPDIR:
    case GNUTYPE_MULTIVOL:
    case GNUTYPE_NAMES:
    case GNUTYPE_SPARSE:
    case GNUTYPE_VOLHDR:
    case SOLARIS_XHDTYPE:
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Unknown or unsupported type (%d/0x%02x) for %s%s%s.", acRoutine, pcFile, psTar->typeflag, psTar->typeflag, (pcNeuteredPrefix[0]) ? pcNeuteredPrefix : "", (pcNeuteredPrefix[0]) ? "/" : "", pcName);
      return ER;
      break;
    }

    /*-
     *******************************************************************
     *
     * Free up any memory that's no longer needed.
     *
     *******************************************************************
     */
    free(pcNeuteredPrefix);
    free(pcNeuteredName);
    if (iLongName == 1 && pcName != NULL)
    {
      free(pcName);
    }
    if (iLongLink == 1 && pcLink != NULL)
    {
      free(pcLink);
    }
  }

#ifdef TARMAP_DEBUG
  fprintf(stderr, "GoodHeaderCount=%d\n", psProperties->iGoodHeaderCount);
  fprintf(stderr, "NullHeaderCount=%d\n", psProperties->iNullHeaderCount);
  fprintf(stderr, "RuntHeaderCount=%d\n", psProperties->iRuntHeaderCount);
#endif

  return ER_OK;
}
