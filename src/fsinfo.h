/*-
 ***********************************************************************
 *
 * $Id: fsinfo.h,v 1.25 2007/03/23 03:09:48 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
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
#define FSINFO_MAX_STRING 255

enum eFSTypes
{
  FSTYPE_UNSUPPORTED = 0,
  FSTYPE_AIX,
  FSTYPE_CDFS,
  FSTYPE_DATAPLOW_ZFS,
  FSTYPE_DEVFS,
  FSTYPE_EXT2,
  FSTYPE_FAT,
  FSTYPE_FAT_REMOTE,
  FSTYPE_FFS,
  FSTYPE_HFS,
  FSTYPE_JFS,
  FSTYPE_NA,
  FSTYPE_NFS,
  FSTYPE_NFS3,
  FSTYPE_NTFS,
  FSTYPE_NTFS3G,
  FSTYPE_NTFS_REMOTE,
  FSTYPE_NWCOMPAT,
  FSTYPE_NWCOMPAT_REMOTE,
  FSTYPE_NWFS,
  FSTYPE_NWFS_REMOTE,
  FSTYPE_RAMFS,
  FSTYPE_REISER,
  FSTYPE_SMB,
  FSTYPE_TMPFS,
  FSTYPE_UDF,
  FSTYPE_UFS,
  FSTYPE_VXFS,
  FSTYPE_VZFS,
  FSTYPE_XFS
};


/*-
 ***********************************************************************
 *
 * Platform Specific Defines
 *
 ***********************************************************************
 */
#ifdef FTimes_AIX
#ifndef MNT_AIX
#define MNT_AIX      0       /* AIX physical fs "oaix"         */
#endif
#ifndef MNT_NFS
#define MNT_NFS      2       /* SUN Network File System "nfs"  */
#endif
#ifndef MNT_JFS
#define MNT_JFS      3       /* AIX R3 physical fs "jfs"       */
#endif
#ifndef MNT_CDROM
#define MNT_CDROM    5       /* CDROM File System "cdrom"      */
#endif
#ifndef MNT_SFS
#define MNT_SFS     16       /* AIX Special FS (STREAM mounts) */
#endif
#ifndef MNT_CACHEFS
#define MNT_CACHEFS 17       /* Cachefs file system            */
#endif
#ifndef MNT_NFS3
#define MNT_NFS3    18       /* NFSv3 file system              */
#endif
#ifndef MNT_AUTOFS
#define MNT_AUTOFS  19       /* Automount file system          */
#endif
#endif

#ifdef FTimes_LINUX
#ifndef EXT2_OLD_SUPER_MAGIC
#define EXT2_OLD_SUPER_MAGIC  0xEF51
#endif
#ifndef EXT2_SUPER_MAGIC
#define EXT2_SUPER_MAGIC      0xEF53
#endif
#ifndef ISOFS_SUPER_MAGIC
#define ISOFS_SUPER_MAGIC     0x9660
#endif
#ifndef MSDOS_SUPER_MAGIC
#define MSDOS_SUPER_MAGIC     0x4d44
#endif
#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC       0x6969
#endif
#ifndef NTFS_SUPER_MAGIC
#define NTFS_SUPER_MAGIC  0x5346544e
#endif
#ifndef NTFS3G_SUPER_MAGIC
#define NTFS3G_SUPER_MAGIC 0x65735546
#endif
#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC      0x9fa0
#endif
#ifndef UFS_MAGIC
#define UFS_MAGIC         0x00011954
#endif
#ifndef REISERFS_SUPER_MAGIC
#define REISERFS_SUPER_MAGIC 0x52654973
#endif
#ifndef SMB_SUPER_MAGIC
#define SMB_SUPER_MAGIC       0x517B
#endif
#ifndef TMPFS_SUPER_MAGIC
#define TMPFS_SUPER_MAGIC  0x1021994
#endif
#ifndef VZFS_SUPER_MAGIC
#define VZFS_SUPER_MAGIC  0x565a4653
#endif
#ifndef RAMFS_SUPER_MAGIC
#define RAMFS_SUPER_MAGIC 0x858458f6
#endif
#ifndef XFS_SUPER_MAGIC
#define XFS_SUPER_MAGIC   0x58465342
#endif
#endif


/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 GetFileSystemType(char *pcPath, char *pcError);


/*-
 ***********************************************************************
 *
 * External Variables
 *
 ***********************************************************************
 */
extern char         gaacFSType[][FSINFO_MAX_STRING];
