/*
 ***********************************************************************
 *
 * $Id: fsinfo.c,v 1.2 2002/01/29 14:58:04 mavrik Exp $
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
 * Platform independent list of file systems.
 *
 ***********************************************************************
 */
char                FSType[][FSINFO_MAX_STRING] =
{
  "UNSUPPORTED",
  "NA",
  "EXT2",
  "FAT",
  "FAT_Remote",
  "NFS",
  "NTFS",
  "NTFS_Remote",
  "TMP",
  "UFS",
  "AIX",
  "JFS",
  "NFS3",
  "FFS"
};


#ifdef FTimes_UNIX
#ifdef FTimes_AIX
/*-
 ***********************************************************************
 *
 * GetFileSystemType (AIX)
 *
 ***********************************************************************
 */
int
GetFileSystemType(char *pcPath, char *pcError)
{
  const char          cRoutine[] = "GetFileSystemType()";
  struct statfs       statFS;

  if (statfs(pcPath, &statFS) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER;
  }
  else
  {
    switch(statFS.f_vfstype)
    {
    case MNT_AIX:
      return FSTYPE_AIX;
      break;
    case MNT_JFS:
      return FSTYPE_JFS;
      break;
    case MNT_NFS:
      return FSTYPE_NFS;
      break;
    case MNT_NFS3:
      return FSTYPE_NFS3;
      break;
    case MNT_SFS:
    case MNT_CDROM:
    case MNT_CACHEFS:
    case MNT_AUTOFS:
    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: FileSystem = [0x%x]: Unsupported file system.", cRoutine, statFS.f_vfstype);
      return FSTYPE_UNSUPPORTED;
      break;
    }
  }
}
#endif /* FTimes_AIX */


#ifdef FTimes_LINUX
/*-
 ***********************************************************************
 *
 * GetFileSystemType (LINUX)
 *
 ***********************************************************************
 */
int
GetFileSystemType(char *pcPath, char *pcError)
{
  const char          cRoutine[] = "GetFileSystemType()";
  struct statfs       statFS;

  if (statfs(pcPath, &statFS) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER;
  }
  else
  {
    switch(statFS.f_type)
    {
    case MSDOS_SUPER_MAGIC:
      return FSTYPE_FAT;
      break;
    case UFS_MAGIC:
      return FSTYPE_UFS;
      break;
    case EXT2_SUPER_MAGIC:
    case EXT2_OLD_SUPER_MAGIC:
      return FSTYPE_EXT2;
      break;
    case NFS_SUPER_MAGIC:
      return FSTYPE_NFS;
      break;
    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: FileSystem = [0x%lx]: Unsupported file system.", cRoutine, (long) statFS.f_type);
      return FSTYPE_UNSUPPORTED;
      break;
    }
  }
}
#endif /* FTimes_LINUX */


#if defined(FTimes_SOLARIS) || defined(FTimes_BSD)
/*-
 ***********************************************************************
 *
 * GetFileSystemType (BSD or SOLARIS)
 *
 ***********************************************************************
 */
int
GetFileSystemType(char *pcPath, char *pcError)
{
  const char          cRoutine[] = "GetFileSystemType()";
  char                cFSName[FSINFO_MAX_STRING];
  int                 i;

#ifdef FTimes_SOLARIS
  struct statvfs      statVFS;

  if (statvfs(pcPath, &statVFS) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER;
  }
  else
  {
    for (i = 0; i < strlen(statVFS.f_basetype); i++)
    {
      cFSName[i] = toupper(statVFS.f_basetype[i]);
    }
    cFSName[i] = 0;
#else
  struct statfs       statFS;

  if (statfs(pcPath, &statFS) == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER;
  }
  else
  {
    for (i = 0; i < strlen(statFS.f_fstypename); i++)
    {
      cFSName[i] = toupper(statFS.f_fstypename[i]);
    }
    cFSName[i] = 0;
#endif

    if (strstr(cFSName, "NTFS") != NULL)
    {
      return FSTYPE_NTFS;
    }
    else if (strstr(cFSName, "DOS") != NULL)
    {
      return FSTYPE_FAT;
    }
    else if (strstr(cFSName, "FAT") != NULL)
    {
      return FSTYPE_FAT;
    }
    else if (strstr(cFSName, "UFS") != NULL)
    {
      return FSTYPE_UFS;
    }
    else if (strstr(cFSName, "EXT2") != NULL)
    {
      return FSTYPE_EXT2;
    }
    else if (strstr(cFSName, "NFS") != NULL)
    {
      return FSTYPE_NFS;
    }
    else if (strstr(cFSName, "TMP") != NULL)
    {
      return FSTYPE_TMP;
    }
    else if (strstr(cFSName, "FFS") != NULL)
    {
      return FSTYPE_FFS;
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: FileSystem = [%s]: Unsupported file system.", cRoutine, cFSName);
      return FSTYPE_UNSUPPORTED;
    }
  }
}
#endif /* FTimes_BSD || FTimes_SOLARIS */
#endif /* FTimes_UNIX */


#ifdef FTimes_WIN32
/*-
 ***********************************************************************
 *
 * GetFileSystemType (WIN32)
 *
 ***********************************************************************
 *
 * Assumptions: (1) the supplied path is at least 2 characters long
 * and (2) the first two characters of this path have the following
 * form "<DriveLetter>:" (e.g. c:).
 *
 ***********************************************************************
 */
int
GetFileSystemType(char *pcPath, char *pcError)
{
  const char          cRoutine[] = "GetFileSystemType()";
  char                cFSName[255];
  char                cRootPath[4];
  char               *pcMessage;
  int                 i;
  unsigned int        uiDriveType;

  cRootPath[0] = pcPath[0];
  cRootPath[1] = pcPath[1];
  cRootPath[2] = '\\';
  cRootPath[3] = 0;

  cFSName[0] = 0;

  uiDriveType = GetDriveType(cRootPath);
  if (uiDriveType == DRIVE_UNKNOWN)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Drive = [DRIVE_UNKNOWN]: Unsupported drive.", cRoutine);
    return ER;
  }
  else if (uiDriveType == DRIVE_NO_ROOT_DIR)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Path not recognized as a root directory.", cRoutine);
    return ER;
  }
  else
  {
    if (GetFileAttributes(pcPath) == 0xffffffff)
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, pcMessage);
      return ER;
    }

    if (!GetVolumeInformation(cRootPath, NULL, 0, NULL, NULL, NULL, cFSName, sizeof(cFSName) - 1))
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, ERRBUF_SIZE, "%s: RootPath = [%s]: %s", cRoutine, cRootPath, pcMessage);
      return ER;
    }

    for (i = 0; i < (int) strlen(cFSName); i++)
    {
      cFSName[i] = toupper(cFSName[i]);
    }

    if (strstr(cFSName, "NTFS") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_NTFS_REMOTE;
    }
    else if (strstr(cFSName, "NTFS") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_NTFS;
    }
    else if (strstr(cFSName, "FAT") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_FAT_REMOTE;
    }
    else if (strstr(cFSName, "FAT") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_FAT;
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: FileSystem = [%s]: Unsupported file system.", cRoutine, cFSName);
      return FSTYPE_UNSUPPORTED;
    }
  }
}
#endif /* FTimes_WIN32 */
