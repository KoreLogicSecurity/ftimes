/*-
 ***********************************************************************
 *
 * $Id: native.h,v 1.2 2003/02/23 17:40:09 mavrik Exp $
 *
 ***********************************************************************
 */

typedef LONG        NTSTATUS, *PNTSTATUS;

typedef enum _FILE_INFORMATION_CLASS
{
   FileDirectoryInformation = 1,
   FileFullDirectoryInformation,
   FileBothDirectoryInformation,
   FileBasicInformation,
   FileStandardInformation,
   FileInternalInformation,
   FileEaInformation,
   FileAccessInformation,
   FileNameInformation,
   FileRenameInformation,
   FileLinkInformation,
   FileNamesInformation,
   FileDispositionInformation,
   FilePositionInformation,
   FileFullEaInformation,
   FileModeInformation,
   FileAlignmentInformation,
   FileAllInformation,
   FileAllocationInformation,
   FileEndOfFileInformation,
   FileAlternateNameInformation,
   FileStreamInformation,
   FilePipeInformation,
   FilePipeLocalInformation,
   FilePipeRemoteInformation,
   FileMailslotQueryInformation,
   FileMailslotSetInformation,
   FileCompressionInformation,
   FileCopyOnWriteInformation,
   FileCompletionInformation,
   FileMoveClusterInformation,
   FileOleClassIdInformation,
   FileOleStateBitsInformation,
   FileNetworkOpenInformation,
   FileObjectIdInformation,
   FileOleAllInformation,
   FileOleDirectoryInformation,
   FileContentIndexInformation,
   FileInheritContentIndexInformation,
   FileOleInformation,
   FileMaximumInformation
}                   FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION
{
   LARGE_INTEGER       CreationTime;
   LARGE_INTEGER       LastAccessTime;
   LARGE_INTEGER       LastWriteTime;
   LARGE_INTEGER       ChangeTime;
   ULONG               FileAttributes;
}                   FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION
{
   LARGE_INTEGER       AllocationSize;
   LARGE_INTEGER       EndOfFile;
   ULONG               NumberOfLinks;
   BOOLEAN             DeletePending;
   BOOLEAN             Directory;
}                   FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION
{
   LARGE_INTEGER       CurrentByteOffset;
}                   FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION
{
   ULONG               AlignmentRequirement;
}                   FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
   LARGE_INTEGER       CreationTime;
   LARGE_INTEGER       LastAccessTime;
   LARGE_INTEGER       LastWriteTime;
   LARGE_INTEGER       ChangeTime;
   LARGE_INTEGER       AllocationSize;
   LARGE_INTEGER       EndOfFile;
   ULONG               FileAttributes;
}                   FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION
{
   BOOLEAN             DeleteFile;
}                   FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION
{
   LARGE_INTEGER       EndOfFile;
}                   FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;


typedef struct _FILE_FULL_EA_INFORMATION
{
   ULONG               NextEntryOffset;
   UCHAR               Flags;
   UCHAR               EaNameLength;
   USHORT              EaValueLength;
   CHAR                EaName[1];
}                   FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;


typedef struct _FILE_STREAM_INFORMATION
{
   ULONG               NextEntryOffset;
   ULONG               StreamNameLength;
   LARGE_INTEGER       StreamSize;
   LARGE_INTEGER       StreamAllocationSize;
   WCHAR               StreamName[1];
}                   FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

/*
 * Define the base asynchronous I/O argument types
 */
typedef struct _IO_STATUS_BLOCK
{
   NTSTATUS            Status;
   ULONG               Information;
}                   IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


DWORD __stdcall NtQueryInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

typedef DWORD(__stdcall *NQIF) (
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

ULONG NTAPI LsaNtStatusToWinError(NTSTATUS Status);
