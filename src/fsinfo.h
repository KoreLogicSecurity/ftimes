/*-
 ***********************************************************************
 *
 * $Id: fsinfo.h,v 1.58 2019/06/25 22:49:39 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FSINFO_H_INCLUDED
#define _FSINFO_H_INCLUDED

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
  FSTYPE_APFS,
  FSTYPE_BTRFS,
  FSTYPE_CDFS,
  FSTYPE_CIFS,
  FSTYPE_CRAMFS,
  FSTYPE_DATAPLOW_ZFS,
  FSTYPE_DEVFS,
  FSTYPE_EXT2,
  FSTYPE_FAT,
  FSTYPE_FAT_REMOTE,
  FSTYPE_FFS,
  FSTYPE_FUSE,
  FSTYPE_GETDATAFS,
  FSTYPE_HFS,
  FSTYPE_JFS,
  FSTYPE_JFFS2,
  FSTYPE_MINIX,
  FSTYPE_NA,
  FSTYPE_NFS,
  FSTYPE_NFS3,
  FSTYPE_NTFS,
  FSTYPE_NTFS_REMOTE,
  FSTYPE_NWCOMPAT,
  FSTYPE_NWCOMPAT_REMOTE,
  FSTYPE_NWFS,
  FSTYPE_NWFS_REMOTE,
  FSTYPE_OVERLAYFS,
  FSTYPE_PTS,
  FSTYPE_RAMFS,
  FSTYPE_REISER,
  FSTYPE_SMB,
  FSTYPE_SMB2,
  FSTYPE_SQUASHFS,
  FSTYPE_TMPFS,
  FSTYPE_UBIFS,
  FSTYPE_UDF,
  FSTYPE_UFS,
  FSTYPE_UFS2,
  FSTYPE_VXFS,
  FSTYPE_VZFS,
  FSTYPE_XFS,
  FSTYPE_YAFFS,
  FSTYPE_ZFS,
  FSTYPE_AUTOFS
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
#ifndef MNT_VXFS
#define MNT_VXFS  0x20       /* Veritas file system            */
#endif
#endif

#ifdef FTimes_LINUX
#ifndef BTRFS_SUPER_MAGIC
#define BTRFS_SUPER_MAGIC 0x9123683e
#endif
#ifndef CIFS_SUPER_MAGIC
#define CIFS_SUPER_MAGIC 0xff534d42
#endif
#ifndef CRAMFS_SUPER_MAGIC
#define CRAMFS_SUPER_MAGIC 0x28cd3d45
#endif
#ifndef EXT2_OLD_SUPER_MAGIC
#define EXT2_OLD_SUPER_MAGIC  0xEF51
#endif
#ifndef EXT2_SUPER_MAGIC
#define EXT2_SUPER_MAGIC      0xEF53
#endif
#ifndef ISOFS_SUPER_MAGIC
#define ISOFS_SUPER_MAGIC     0x9660
#endif
#ifndef JFS_SUPER_MAGIC
#define JFS_SUPER_MAGIC   0x3153464a /* JFS1 */
#endif
#ifndef JFFS2_SUPER_MAGIC
#define JFFS2_SUPER_MAGIC     0x72b6
#endif
#ifndef MINIX_SUPER_MAGIC
#define MINIX_SUPER_MAGIC     0x138f
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
#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif
#ifndef OVERLAYFS_SUPER_MAGIC
#define OVERLAYFS_SUPER_MAGIC 0x794c7630
#endif
#ifndef PROC_SUPER_MAGIC
#define PROC_SUPER_MAGIC      0x9fa0
#endif
#ifndef PTS_SUPER_MAGIC
#define PTS_SUPER_MAGIC       0x1cd1
#endif
#ifndef UBIFS_SUPER_MAGIC
#define UBIFS_SUPER_MAGIC 0x24051905
#endif
#ifndef UFS_MAGIC
#define UFS_MAGIC         0x00011954
#endif
#ifndef UFS2_MAGIC
#define UFS2_MAGIC        0x19540119
#endif
#ifndef REISERFS_SUPER_MAGIC
#define REISERFS_SUPER_MAGIC 0x52654973
#endif
#ifndef SMB_SUPER_MAGIC
#define SMB_SUPER_MAGIC       0x517B
#endif
#ifndef SMB2_SUPER_MAGIC
#define SMB2_SUPER_MAGIC 0xfe534d42
#endif
#ifndef SQUASHFS_SUPER_MAGIC
#define SQUASHFS_SUPER_MAGIC 0x71736873
#endif
#ifndef TMPFS_SUPER_MAGIC
#define TMPFS_SUPER_MAGIC  0x1021994
#endif
#ifndef VXFS_SUPER_MAGIC
#define VXFS_SUPER_MAGIC  0xa501fcf5
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
#ifndef UDF_SUPER_MAGIC
#define UDF_SUPER_MAGIC   0x15013346
#endif
#ifndef YAFFS_SUPER_MAGIC
#define YAFFS_SUPER_MAGIC 0x5941ff53
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

#endif /* !_FSINFO_H_INCLUDED */
