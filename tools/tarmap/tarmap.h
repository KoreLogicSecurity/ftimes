/*-
 ***********************************************************************
 *
 * $Id: tarmap.h,v 1.34 2019/03/14 16:07:45 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define PROGRAM_NAME "tarmap"
#define VERSION "1.1.15"

#define ER -1
#define ER_OK 0

#define XER_OK 0
#define XER_Usage 1
#define XER_BootStrap 2
#define XER_ProcessArguments 3
#define XER_WorkHorse 4
#define XER_RunMode 5

#define TARMAP_NAME_SIZE 100
#define TARMAP_PREFIX_SIZE 155
#define TARMAP_READ_SIZE 8192
#define TARMAP_UNIT_SIZE 512

#define TARMAP_MAP 0x00000001

#define TARMAP_MAX_PATH 4096

#define TAR_TYPE_POSIX 0
#define TAR_TYPE_OLD_GNU 1

#define TAR_MAGIC "ustar\0"
#define TAR_MAGIC_LENGTH 6
#define TAR_OLD_GNU_MAGIC "ustar "
#define TAR_OLD_GNU_MAGIC_LENGTH 6
#define TAR_VERSION "00" /* 00 and no null */
#define TAR_VERSION_LENGTH 2

#define REGTYPE  '0' /* Regular file */
#define LNKTYPE  '1' /* Hard link */
#define SYMTYPE  '2' /* Symbolic link */
#define CHRTYPE  '3' /* Character special */
#define BLKTYPE  '4' /* Block special */
#define DIRTYPE  '5' /* Directory */
#define FIFOTYPE '6' /* FIFO special */
#define CONTTYPE '7' /* Reserved */
#define XHDTYPE  'x' /* Extended header referring to the next file in the archive */
#define XGLTYPE  'g' /* Global extended header */

#define GNUTYPE_DUMPDIR  'D' /* Names of files in the directory at the time the dump was made. */
#define GNUTYPE_LONGLINK 'K' /* Identifies the next file as having a long linkname. */
#define GNUTYPE_LONGNAME 'L' /* Identifies the next file as having a long name. */
#define GNUTYPE_MULTIVOL 'M' /* Continuation of a file that began in a previous volume. */
#define GNUTYPE_NAMES    'N' /* For storing filenames that do not fit into the main header. */
#define GNUTYPE_SPARSE   'S' /* This is for sparse files. */
#define GNUTYPE_VOLHDR   'V' /* This file is a tape/volume header. Ignore it on extraction. */
#define SOLARIS_XHDTYPE  'X' /* Solaris extended header. */

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _TAR_HEADER
{                                              /* byte offset */
  char                name[100];               /*           0 */
  char                mode[8];                 /*         100 */
  char                uid[8];                  /*         108 */
  char                gid[8];                  /*         116 */
  char                size[12];                /*         124 */
  char                mtime[12];               /*         136 */
  char                chksum[8];               /*         148 */
  char                typeflag;                /*         156 */
  char                linkname[100];           /*         157 */
  char                magic[6];                /*         257 */
  char                version[2];              /*         263 */
  char                uname[32];               /*         265 */
  char                gname[32];               /*         297 */
  char                devmajor[8];             /*         329 */
  char                devminor[8];             /*         337 */
  char                prefix[155];             /*         345 */
                                               /*         500 */
} TAR_HEADER;

typedef struct _TARMAP_PROPERTIES
{
  char               *pcTarFile;
  char              **ppcPayloadVector;
  int                 iGoodHeaderCount;
  int                 iNullHeaderCount;
  int                 iRuntHeaderCount;
  int                 iRunMode;
  int                 iTarType;
  unsigned char      *pucTarFileHeader;
  unsigned char      *pucTarNullHeader;
} TARMAP_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 TarMapBootStrap(char *pcError);
void                TarMapFreeProperties(TARMAP_PROPERTIES *psProperties);
TARMAP_PROPERTIES  *TarMapGetPropertiesReference(void);
TARMAP_PROPERTIES  *TarMapNewProperties(char *pcError);
char               *TarMapNeuterString(char *pcData, int iLength, char *pcError);
int                 TarMapProcessArguments(int iArgumentCount, char *ppcArgumentVector[], TARMAP_PROPERTIES *psProperties, char *pcError);
int                 TarMapReadHeader(FILE *pFile, unsigned char *pucHeader, char *pcError);
char               *TarMapReadLongName(FILE *pFile, APP_UI64 ui64Size, char *pcError);
int                 TarMapReadPadding(FILE *pFile, APP_UI64 ui64Size, char *pcError);
void                TarMapSetPropertiesReference(TARMAP_PROPERTIES *psProperties);
void                TarMapShutdown(int iError);
void                TarMapUsage(void);
void                TarMapVersion(void);
int                 TarMapWorkHorse(TARMAP_PROPERTIES *psProperties, char *pcError);
