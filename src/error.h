/*-
 ***********************************************************************
 *
 * $Id: error.h,v 1.13 2007/02/23 00:22:35 mavrik Exp $
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
#define MESSAGE_SIZE                  1024

#define XER_OK                           0
#define XER_Abort                        1
#define XER_Usage                        2
#define XER_BootStrap                    3
#define XER_ProcessArguments             4
#define XER_Initialize                   5
#define XER_CheckDependencies            6
#define XER_Finalize                     7
#define XER_WorkHorse                    8
#define XER_FinishUp                     9
#define XER_FinalStage                  10
#define XER_MaxExternalErrorCode       255

#define ER                              -1
#define ER_OK                            0

enum InternalErrors
{
  ER_BadHandle = 256,
  ER_BadMagicOperator,
  ER_BadMagicType,
  ER_BadValue,
  ER_DoDig,
  ER_DoDigest,
  ER_DoXMagic,
  ER_CreateFile,
  ER_Failure,
  ER_FileSystem,
  ER_FindFirstFile,
  ER_GetFileAttrs,
  ER_GetFileAttrsEx,
  ER_GetFileInfo,
  ER_Header,
  ER_InvalidSeverity,
  ER_Length,
  ER_MapCountNamedStreams,
  ER_MissingControl,
  ER_NQIF,
  ER_NeuterPathname,
  ER_NothingToDo,
  ER_NullFields,
  ER_Overflow,
  ER_ReadFile,
  ER_ReadPropertiesFile,
  ER_Special,
  ER_URLGetRequest,
  ER_URLPingRequest,
  ER_URLPutRequest,
  ER_Usage,
  ER_Warning,
  ER_XMagic,
  ER_execlp,
  ER_fgets,
  ER_fopen,
  ER_fread,
  ER_lstat,
  ER_opendir,
  ER_readdir,
  ER_readlink
};

enum ErrorLevels
{
  ERROR_WARNING,
  ERROR_FAILURE,
  ERROR_CRITICAL
};

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
void                ErrorFormatWin32Error(char **ppmsg);
int                 ErrorGetWarnings(void);
int                 ErrorGetFailures(void);
int                 ErrorGetWipeouts(void);
void                ErrorHandler(int iError, char *pcError, int iSeverity);
#ifdef WIN32
void                ErrorFormatWin32Error(char **ppcMessage);
#endif
