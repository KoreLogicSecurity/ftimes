/*-
 ***********************************************************************
 *
 * $Id: fsinfo.c,v 1.29 2007/03/23 03:09:48 mavrik Exp $
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
 * Platform independent list of file systems.
 *
 ***********************************************************************
 */
char                gaacFSType[][FSINFO_MAX_STRING] =
{
  "UNSUPPORTED",
  "AIX",
  "CDFS",
  "DATAPLOW_ZFS",
  "DEVFS",
  "EXT2",
  "FAT",
  "FAT_Remote",
  "FFS",
  "HFS",
  "JFS",
  "NA",
  "NFS",
  "NFS3",
  "NTFS",
  "NTFS3G",
  "NTFS_Remote",
  "NWCOMPAT",
  "NWCOMPAT_Remote",
  "NWFS",
  "NWFS_Remote",
  "RAMFS",
  "REISER",
  "SMB",
  "TMPFS",
  "UDF",
  "UFS",
  "VXFS",
  "VZFS",
  "XFS"
};


#ifdef UNIX
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
  const char          acRoutine[] = "GetFileSystemType()";
  struct statfs       sStatFS;

  if (statfs(pcPath, &sStatFS) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER;
  }
  else
  {
    switch(sStatFS.f_vfstype)
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
    case MNT_CDROM:
      return FSTYPE_CDFS;
      break;
    case MNT_SFS:
    case MNT_CACHEFS:
    case MNT_AUTOFS:
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: FileSystem = [0x%x]: Unsupported file system.", acRoutine, sStatFS.f_vfstype);
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
  const char          acRoutine[] = "GetFileSystemType()";
  struct statfs       sStatFS;

  if (statfs(pcPath, &sStatFS) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER;
  }
  else
  {
    switch(sStatFS.f_type)
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
    case NTFS_SUPER_MAGIC:
      return FSTYPE_NTFS;
      break;
    case NTFS3G_SUPER_MAGIC:
      return FSTYPE_NTFS3G;
      break;
    case REISERFS_SUPER_MAGIC:
      return FSTYPE_REISER;
      break;
    case SMB_SUPER_MAGIC:
      return FSTYPE_SMB;
      break;
    case ISOFS_SUPER_MAGIC:
      return FSTYPE_CDFS;
      break;
    case TMPFS_SUPER_MAGIC:
      return FSTYPE_TMPFS;
      break;
    case VZFS_SUPER_MAGIC:
      return FSTYPE_VZFS;
      break;
    case RAMFS_SUPER_MAGIC:
      return FSTYPE_RAMFS;
      break;
    case XFS_SUPER_MAGIC:
      return FSTYPE_XFS;
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: FileSystem = [0x%lx]: Unsupported file system.", acRoutine, (long) sStatFS.f_type);
      return FSTYPE_UNSUPPORTED;
      break;
    }
  }
}
#endif /* FTimes_LINUX */


#if defined(FTimes_SOLARIS) || defined(FTimes_BSD) || defined(FTimes_MACOS) || defined(FTimes_HPUX)
/*-
 ***********************************************************************
 *
 * GetFileSystemType (SOLARIS, BSD, and MACOS)
 *
 ***********************************************************************
 */
int
GetFileSystemType(char *pcPath, char *pcError)
{
  const char          acRoutine[] = "GetFileSystemType()";
  char                acFSName[FSINFO_MAX_STRING];
  int                 i;

#if defined(FTimes_SOLARIS) || defined(FTimes_HPUX)
  struct statvfs      statVFS;

  if (statvfs(pcPath, &statVFS) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER;
  }
  else
  {
    for (i = 0; i < strlen(statVFS.f_basetype); i++)
    {
      acFSName[i] = toupper(statVFS.f_basetype[i]);
    }
    acFSName[i] = 0;
#else
  struct statfs       sStatFS;

  if (statfs(pcPath, &sStatFS) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER;
  }
  else
  {
    for (i = 0; i < strlen(sStatFS.f_fstypename); i++)
    {
      acFSName[i] = toupper(sStatFS.f_fstypename[i]);
    }
    acFSName[i] = 0;
#endif

    if (strstr(acFSName, "NTFS") != NULL)
    {
      return FSTYPE_NTFS;
    }
    else if (strstr(acFSName, "NTFS3G") != NULL)
    {
      return FSTYPE_NTFS3G;
    }
    else if (strstr(acFSName, "DOS") != NULL)
    {
      return FSTYPE_FAT;
    }
    else if (strstr(acFSName, "FAT") != NULL)
    {
      return FSTYPE_FAT;
    }
    else if (strstr(acFSName, "UDF") != NULL)
    {
      return FSTYPE_UDF;
    }
    else if (strstr(acFSName, "UFS") != NULL)
    {
      return FSTYPE_UFS;
    }
    else if (strstr(acFSName, "EXT2") != NULL)
    {
      return FSTYPE_EXT2;
    }
    else if (strstr(acFSName, "NFS") != NULL)
    {
      return FSTYPE_NFS;
    }
    else if (strstr(acFSName, "TMP") != NULL)
    {
      return FSTYPE_TMPFS;
    }
    else if (strstr(acFSName, "FFS") != NULL)
    {
      return FSTYPE_FFS;
    }
    else if (strstr(acFSName, "HFS") != NULL)
    {
      return FSTYPE_HFS;
    }
    else if (strstr(acFSName, "VXFS") != NULL)
    {
      return FSTYPE_VXFS;
    }
    else if (strstr(acFSName, "SMBFS") != NULL)
    {
      return FSTYPE_SMB;
    }
    else if (strstr(acFSName, "CD9660") != NULL || strstr(acFSName, "HSFS"))
    {
      return FSTYPE_CDFS;
    }
    else if (strstr(acFSName, "DEVFS") != NULL)
    {
      return FSTYPE_DEVFS;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: FileSystem = [%s]: Unsupported file system.", acRoutine, acFSName);
      return FSTYPE_UNSUPPORTED;
    }
  }
}
#endif /* FTimes_SOLARIS || FTimes_BSD || FTimes_MACOS || FTimes_HPUX */
#endif /* UNIX */


#ifdef WIN32
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
  const char          acRoutine[] = "GetFileSystemType()";
  char                acFSName[255];
  char                acRootPath[4];
  char               *pcMessage;
  int                 i;
  unsigned int        uiDriveType;

  acRootPath[0] = pcPath[0];
  acRootPath[1] = pcPath[1];
  acRootPath[2] = '\\';
  acRootPath[3] = 0;

  acFSName[0] = 0;

  uiDriveType = GetDriveType(acRootPath);
  if (uiDriveType == DRIVE_UNKNOWN)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Drive = [DRIVE_UNKNOWN]: Unsupported drive.", acRoutine);
    return ER;
  }
  else if (uiDriveType == DRIVE_NO_ROOT_DIR)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Path not recognized as a root directory.", acRoutine);
    return ER;
  }
  else
  {
    if (GetFileAttributes(pcPath) == 0xffffffff)
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, pcMessage);
      return ER;
    }

    if (!GetVolumeInformation(acRootPath, NULL, 0, NULL, NULL, NULL, acFSName, sizeof(acFSName) - 1))
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: RootPath = [%s]: %s", acRoutine, acRootPath, pcMessage);
      return ER;
    }

    for (i = 0; i < (int) strlen(acFSName); i++)
    {
      acFSName[i] = toupper(acFSName[i]);
    }

    if (strstr(acFSName, "NTFS") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_NTFS_REMOTE;
    }
    else if (strstr(acFSName, "NTFS") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_NTFS;
    }
    else if (strstr(acFSName, "NTFS3G") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_NTFS3G;
    }
    else if (strstr(acFSName, "FAT") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_FAT_REMOTE;
    }
    else if (strstr(acFSName, "FAT") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_FAT;
    }
    else if (strstr(acFSName, "CDFS") != NULL)
    {
      return FSTYPE_CDFS;
    }
    else if (strstr(acFSName, "DATAPLOW_ZFS") != NULL)
    {
      return FSTYPE_DATAPLOW_ZFS;
    }
    else if (strstr(acFSName, "NWCOMPAT") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_NWCOMPAT_REMOTE;
    }
    else if (strstr(acFSName, "NWCOMPAT") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_NWCOMPAT;
    }
    else if (strstr(acFSName, "NWFS") != NULL && uiDriveType == DRIVE_REMOTE)
    {
      return FSTYPE_NWFS_REMOTE;
    }
    else if (strstr(acFSName, "NWFS") != NULL && uiDriveType != DRIVE_REMOTE)
    {
      return FSTYPE_NWFS;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: FileSystem = [%s]: Unsupported file system.", acRoutine, acFSName);
      return FSTYPE_UNSUPPORTED;
    }
  }
}
#endif /* WIN32 */
