/*-
 ***********************************************************************
 *
 * $Id: hook.c,v 1.13 2014/07/18 18:01:22 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2011-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * HookAddHook
 *
 ***********************************************************************
 */
int
HookAddHook(char *pcExpression, HOOK_LIST **psHead, char *pcError)
{
  const char          acRoutine[] = "HookAddHook()";
  char                acLocalError[MESSAGE_SIZE] = "";
  HOOK_LIST          *psCurrent = NULL;
  HOOK_LIST          *psHook = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new hook.
   *
   *********************************************************************
   */
  psHook = HookNewHook(pcExpression, acLocalError);
  if (psHook == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * If the head is NULL, insert the new hook and return. Otherwise,
   * append the new hook to the end of the list. Warn the user about
   * duplicate expressions.
   *
   *********************************************************************
   */
  if (*psHead == NULL)
  {
    *psHead = psHook;
  }
  else
  {
    psCurrent = *psHead;
    while (psCurrent != NULL)
    {
      if (psHook->uiId == psCurrent->uiId)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: The expression for hook known as \"%s\" is equivalent to the one for \"%s\", but duplicate expressions are not allowed.", acRoutine, psHook->pcName, psCurrent->pcName);
        HookFreeHook(psHook);
        return ER;
      }
      if (psCurrent->psNext == NULL)
      {
        psCurrent->psNext = psHook;
        break;
      }
      psCurrent = psCurrent->psNext;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * HookFreeHook
 *
 ***********************************************************************
 */
void
HookFreeHook(HOOK_LIST *psHook)
{
  if (psHook != NULL)
  {
// NOTE: This string is owned by KL-EL and should not be freed.
//  if (psHook->pcName != NULL)
//  {
//    free(psHook->pcName);
//  }
    if (psHook->pcExpression != NULL)
    {
      free(psHook->pcExpression);
    }
// NOTE: This string is owned by KL-EL and should not be freed.
//  if (psHook->pcInterpreter != NULL)
//  {
//    free(psHook->pcInterpreter);
//  }
// NOTE: This string is owned by KL-EL and should not be freed.
//  if (psHook->pcProgram != NULL)
//  {
//    free(psHook->pcProgram);
//  }
    KlelFreeContext(psHook->psContext);
    free(psHook);
  }
}


/*-
 ***********************************************************************
 *
 * HookGetTypeOfVar
 *
 ***********************************************************************
 */
KLEL_EXPR_TYPE
HookGetTypeOfVar(const char *pcName, void *pvContext)
{
  int                 i = 0;
  HOOK_TYPE_SPEC      asTypes[] =
  {
    { "f_atime",       KLEL_EXPR_INTEGER }, /* file atime */
    { "f_ctime",       KLEL_EXPR_INTEGER }, /* file ctime */
    { "f_dev",         KLEL_EXPR_INTEGER }, /* file dev */
    { "f_exists",      KLEL_EXPR_BOOLEAN }, /* file exists */
    { "f_fields",      KLEL_EXPR_INTEGER }, /* file fields */
    { "f_fstype",      KLEL_EXPR_STRING  }, /* file system type */
    { "f_gid",         KLEL_EXPR_INTEGER }, /* file group ID */
    { "f_inode",       KLEL_EXPR_INTEGER }, /* file inode */
    { "f_magic",       KLEL_EXPR_STRING  }, /* file type (XMagic) */
    { "f_md5",         KLEL_EXPR_STRING  }, /* file hash (MD5) */
    { "f_mode",        KLEL_EXPR_INTEGER }, /* file mode */
    { "f_mode_gr",     KLEL_EXPR_BOOLEAN }, /* file mode permission: group read */
    { "f_mode_gw",     KLEL_EXPR_BOOLEAN }, /* file mode permission: group write */
    { "f_mode_gx",     KLEL_EXPR_BOOLEAN }, /* file mode permission: group execute */
    { "f_mode_or",     KLEL_EXPR_BOOLEAN }, /* file mode permission: other read */
    { "f_mode_ow",     KLEL_EXPR_BOOLEAN }, /* file mode permission: other write */
    { "f_mode_ox",     KLEL_EXPR_BOOLEAN }, /* file mode permission: other execute */
    { "f_mode_sg",     KLEL_EXPR_BOOLEAN }, /* file mode SGID (set group id on execution) bit */
    { "f_mode_st",     KLEL_EXPR_BOOLEAN }, /* file mode sticky bit */
    { "f_mode_su",     KLEL_EXPR_BOOLEAN }, /* file mode SUID (set user id on execution) bit */
    { "f_mode_tb",     KLEL_EXPR_BOOLEAN }, /* file mode type: block special */
    { "f_mode_tc",     KLEL_EXPR_BOOLEAN }, /* file mode type: character special */
    { "f_mode_td",     KLEL_EXPR_BOOLEAN }, /* file mode type: directory */
    { "f_mode_tl",     KLEL_EXPR_BOOLEAN }, /* file mode type: symbolic link */
    { "f_mode_tp",     KLEL_EXPR_BOOLEAN }, /* file mode type: named pipe (fifo) */
    { "f_mode_tr",     KLEL_EXPR_BOOLEAN }, /* file mode type: regular file */
    { "f_mode_ts",     KLEL_EXPR_BOOLEAN }, /* file mode type: socket */
    { "f_mode_tw",     KLEL_EXPR_BOOLEAN }, /* file mode type: whiteout */
    { "f_mode_ur",     KLEL_EXPR_BOOLEAN }, /* file mode permission: user read */
    { "f_mode_uw",     KLEL_EXPR_BOOLEAN }, /* file mode permission: user write */
    { "f_mode_ux",     KLEL_EXPR_BOOLEAN }, /* file mode permission: user execute */
    { "f_mtime",       KLEL_EXPR_INTEGER }, /* file mtime */
    { "f_name",        KLEL_EXPR_STRING  }, /* file name (full path) */
    { "f_nlink",       KLEL_EXPR_INTEGER }, /* file nlink */
    { "f_rdev",        KLEL_EXPR_INTEGER }, /* file rdev */
    { "f_sha1",        KLEL_EXPR_STRING  }, /* file hash (SHA1) */
    { "f_sha256",      KLEL_EXPR_STRING  }, /* file hash (SHA256) */
    { "f_size",        KLEL_EXPR_INTEGER }, /* file size */
    { "f_uid",         KLEL_EXPR_INTEGER }, /* file user ID */
    { "j_egid",        KLEL_EXPR_INTEGER }, /* job effective group ID */
    { "j_euid",        KLEL_EXPR_INTEGER }, /* job effective user ID */
    { "j_field_mask",  KLEL_EXPR_STRING  }, /* job field mask */
    { "j_log_level",   KLEL_EXPR_INTEGER }, /* job log level */
    { "j_rgid",        KLEL_EXPR_INTEGER }, /* job real group ID */
    { "j_ruid",        KLEL_EXPR_INTEGER }, /* job real user ID */
    { "p_exists",      KLEL_EXPR_BOOLEAN }, /* parent exists */
    { "p_fstype",      KLEL_EXPR_STRING  }, /* parent file system type */
    { "p_mode",        KLEL_EXPR_INTEGER }, /* parent mode */
    { "p_mode_gr",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: group read */
    { "p_mode_gw",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: group write */
    { "p_mode_gx",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: group execute */
    { "p_mode_or",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: other read */
    { "p_mode_ow",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: other write */
    { "p_mode_ox",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: other execute */
    { "p_mode_sg",     KLEL_EXPR_BOOLEAN }, /* parent mode SGID (set group id on execution) bit */
    { "p_mode_st",     KLEL_EXPR_BOOLEAN }, /* parent mode sticky bit */
    { "p_mode_su",     KLEL_EXPR_BOOLEAN }, /* parent mode SUID (set user id on execution) bit */
    { "p_mode_tb",     KLEL_EXPR_BOOLEAN }, /* parent mode type: block special */
    { "p_mode_tc",     KLEL_EXPR_BOOLEAN }, /* parent mode type: character special */
    { "p_mode_td",     KLEL_EXPR_BOOLEAN }, /* parent mode type: directory */
    { "p_mode_tl",     KLEL_EXPR_BOOLEAN }, /* parent mode type: symbolic link */
    { "p_mode_tp",     KLEL_EXPR_BOOLEAN }, /* parent mode type: named pipe (fifo) */
    { "p_mode_tr",     KLEL_EXPR_BOOLEAN }, /* parent mode type: regular file */
    { "p_mode_ts",     KLEL_EXPR_BOOLEAN }, /* parent mode type: socket */
    { "p_mode_tw",     KLEL_EXPR_BOOLEAN }, /* parent mode type: whiteout */
    { "p_mode_ur",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: user read */
    { "p_mode_uw",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: user write */
    { "p_mode_ux",     KLEL_EXPR_BOOLEAN }, /* parent mode permission: user execute */
    { "p_name",        KLEL_EXPR_STRING  }, /* parent name (full path) */
  };

  for (i = 0; i < sizeof(asTypes) / sizeof(asTypes[0]); i++)
  {
    if (strcmp(asTypes[i].pcName, pcName) == 0)
    {
/* FIXME Need to return KLEL_EXPRESSION_UNKNOWN for fields not enabled. */
      return asTypes[i].iType;
    }
  }

  return KLEL_TYPE_UNKNOWN; /* This causes KL-EL to retrieve the type of the specified variable, should it exist in the standard library. */
}


/*-
 ***********************************************************************
 *
 * HookGetValueOfVar
 *
 ***********************************************************************
 */
KLEL_VALUE *
HookGetValueOfVar(const char *pcName, void *pvContext)
{
  FTIMES_FILE_DATA   *psFTFileData = (FTIMES_FILE_DATA *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  FTIMES_PROPERTIES  *psProperties = FTimesGetPropertiesReference();
#ifdef UNIX
  struct stat        *psFStatEntry = &psFTFileData->sStatEntry;
  struct stat        *psPStatEntry = (psFTFileData->psParent != NULL) ? &psFTFileData->psParent->sStatEntry : NULL;
#endif

  if (pcName[0] == 'f' && pcName[1] == '_')
  {
    if (strcmp(pcName, "f_name") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_STRING, strlen(psFTFileData->pcNeuteredPath), psFTFileData->pcNeuteredPath);
    }
    if (strcmp(pcName, "f_exists") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFTFileData->iFileExists);
    }
    if (strcmp(pcName, "f_fields") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)psFTFileData->ulAttributeMask);
    }
    if (strcmp(pcName, "f_fstype") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_STRING, strlen(gaacFSType[psFTFileData->iFSType]), gaacFSType[psFTFileData->iFSType]);
    }
    if (strcmp(pcName, "f_magic") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_STRING, strlen(psFTFileData->acType), psFTFileData->acType);
    }
    if (strcmp(pcName, "f_md5") == 0)
    {
      char ac[MD5_HASH_SIZE*2+1];
      MD5HashToHex(psFTFileData->aucFileMd5, ac);
      return KlelCreateValue(KLEL_EXPR_STRING, MD5_HASH_SIZE*2, ac);
    }
    if (strcmp(pcName, "f_sha1") == 0)
    {
      char ac[SHA1_HASH_SIZE*2+1];
      SHA1HashToHex(psFTFileData->aucFileSha1, ac);
      return KlelCreateValue(KLEL_EXPR_STRING, SHA1_HASH_SIZE*2, ac);
    }
    if (strcmp(pcName, "f_sha256") == 0)
    {
      char ac[SHA256_HASH_SIZE*2+1];
      SHA256HashToHex(psFTFileData->aucFileSha256, ac);
      return KlelCreateValue(KLEL_EXPR_STRING, SHA256_HASH_SIZE*2, ac);
    }
  }

  if (pcName[0] == 'j' && pcName[1] == '_')
  {
    if (strcmp(pcName, "j_field_mask") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_STRING, strlen(psProperties->psFieldMask->pcMask), psProperties->psFieldMask->pcMask);
    }
    if (strcmp(pcName, "j_log_level") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psProperties->iLogLevel));
    }
  }

  if (pcName[0] == 'p' && pcName[1] == '_')
  {
    if (strcmp(pcName, "p_name") == 0)
    {
      if (psFTFileData->psParent != NULL)
      {
        return KlelCreateValue(KLEL_EXPR_STRING, strlen(psFTFileData->psParent->pcNeuteredPath), psFTFileData->psParent->pcNeuteredPath);
      }
      return NULL;
    }
    if (strcmp(pcName, "p_exists") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, (psFTFileData->psParent != NULL) ? 1 : 0);
    }
    if (strcmp(pcName, "p_fstype") == 0)
    {
      if (psFTFileData->psParent != NULL)
      {
        return KlelCreateValue(KLEL_EXPR_STRING, strlen(gaacFSType[psFTFileData->psParent->iFSType]), gaacFSType[psFTFileData->psParent->iFSType]);
      }
      return NULL;
    }
  }

#ifdef UNIX
  if (pcName[0] == 'f' && pcName[1] == '_')
  {
    if (strcmp(pcName, "f_atime") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_atime));
    }
    if (strcmp(pcName, "f_ctime") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_ctime));
    }
    if (strcmp(pcName, "f_dev") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_dev));
    }
    if (strcmp(pcName, "f_gid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_gid));
    }
    if (strcmp(pcName, "f_inode") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_ino));
    }
    if (strcmp(pcName, "f_mode") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_gr") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IRGRP);
    }
    if (strcmp(pcName, "f_mode_gw") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IWGRP);
    }
    if (strcmp(pcName, "f_mode_gx") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IXGRP);
    }
    if (strcmp(pcName, "f_mode_or") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IROTH);
    }
    if (strcmp(pcName, "f_mode_ow") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IWOTH);
    }
    if (strcmp(pcName, "f_mode_ox") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IXOTH);
    }
    if (strcmp(pcName, "f_mode_sg") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_ISGID);
    }
    if (strcmp(pcName, "f_mode_st") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_ISVTX);
    }
    if (strcmp(pcName, "f_mode_su") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_ISUID);
    }
    if (strcmp(pcName, "f_mode_tb") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISBLK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tc") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISCHR(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_td") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISDIR(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tl") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISLNK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tp") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISFIFO(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tr") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISREG(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_ts") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISSOCK(psFStatEntry->st_mode));
    }
    if (strcmp(pcName, "f_mode_tw") == 0)
    {
#ifdef S_ISWHT
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISWHT(psFStatEntry->st_mode));
#else
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, (((psFStatEntry->st_mode) & 0170000) == 0160000));
#endif
    }
    if (strcmp(pcName, "f_mode_ur") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IRUSR);
    }
    if (strcmp(pcName, "f_mode_uw") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IWUSR);
    }
    if (strcmp(pcName, "f_mode_ux") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psFStatEntry->st_mode & S_IXUSR);
    }
    if (strcmp(pcName, "f_mtime") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_mtime));
    }
    if (strcmp(pcName, "f_nlink") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_nlink));
    }
    if (strcmp(pcName, "f_rdev") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_rdev));
    }
    if (strcmp(pcName, "f_size") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_size));
    }
    if (strcmp(pcName, "f_uid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psFStatEntry->st_uid));
    }
  }

  if (pcName[0] == 'j' && pcName[1] == '_')
  {
    if (strcmp(pcName, "j_egid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(getegid()));
    }
    if (strcmp(pcName, "j_euid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(geteuid()));
    }
    if (strcmp(pcName, "j_rgid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(getgid()));
    }
    if (strcmp(pcName, "j_ruid") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(getuid()));
    }
  }

  if (pcName[0] == 'p' && pcName[1] == '_')
  {
  if (psPStatEntry != NULL)
  {
    if (strcmp(pcName, "p_mode") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_INTEGER, (long long)(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_gr") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IRGRP);
    }
    if (strcmp(pcName, "p_mode_gw") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IWGRP);
    }
    if (strcmp(pcName, "p_mode_gx") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IXGRP);
    }
    if (strcmp(pcName, "p_mode_or") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IROTH);
    }
    if (strcmp(pcName, "p_mode_ow") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IWOTH);
    }
    if (strcmp(pcName, "p_mode_ox") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IXOTH);
    }
    if (strcmp(pcName, "p_mode_sg") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_ISGID);
    }
    if (strcmp(pcName, "p_mode_st") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_ISVTX);
    }
    if (strcmp(pcName, "p_mode_su") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_ISUID);
    }
    if (strcmp(pcName, "p_mode_tb") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISBLK(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_tc") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISCHR(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_td") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISDIR(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_tl") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISLNK(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_tp") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISFIFO(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_tr") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISREG(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_ts") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISSOCK(psPStatEntry->st_mode));
    }
    if (strcmp(pcName, "p_mode_tw") == 0)
    {
#ifdef S_ISWHT
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, S_ISWHT(psPStatEntry->st_mode));
#else
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, (((psFStatEntry->st_mode) & 0170000) == 0160000));
#endif
    }
    if (strcmp(pcName, "p_mode_ur") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IRUSR);
    }
    if (strcmp(pcName, "p_mode_uw") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IWUSR);
    }
    if (strcmp(pcName, "p_mode_ux") == 0)
    {
      return KlelCreateValue(KLEL_EXPR_BOOLEAN, psPStatEntry->st_mode & S_IXUSR);
    }
  }
  else
  {
    return NULL;
  }
  }
#endif

  return KlelCreateUnknown(); /* This causes KL-EL to retrieve the value of the specified variable, should it exist in the standard library. */
}


/*-
 ***********************************************************************
 *
 * HookMatchHook
 *
 ***********************************************************************
 */
HOOK_LIST *
HookMatchHook(HOOK_LIST *psHookList, FTIMES_FILE_DATA *psFTFileData)
{
  const char          acRoutine[] = "HookMatchHook()";
  char                acLocalError[MESSAGE_SIZE] = "";
  HOOK_LIST          *psHook = NULL;
  KLEL_VALUE         *psResult = NULL;

  for (psHook = psHookList; psHook != NULL; psHook = psHook->psNext, KlelFreeResult(psResult))
  {
    KlelSetPrivateData(psHook->psContext, (void *)psFTFileData);
    psResult = KlelExecute(psHook->psContext);
    if (psResult == NULL)
    {
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: KlelExecute(): Hook (%s) failed to execute expression (%s).", acRoutine, psFTFileData->pcNeuteredPath, psHook->pcName, KlelGetError(psHook->psContext));
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
      continue;
    }
    if (psResult->bBoolean)
    {
      KlelFreeResult(psResult);
      return psHook;
    }
  }

  return NULL;
}


/*-
 ***********************************************************************
 *
 * HookNewHook
 *
 ***********************************************************************
 */
HOOK_LIST *
HookNewHook(char *pcExpression, char *pcError)
{
  const char          acRoutine[] = "HookNewHook()";
#ifdef USE_EMBEDDED_PYTHON
  char                acLocalError[MESSAGE_SIZE] = "";
#endif
  HOOK_LIST          *psHook = NULL;
#if defined(USE_EMBEDDED_PERL) || defined(USE_EMBEDDED_PYTHON)
  int                 iError = 0;
#endif
  int                 iLength = 0;
#ifdef USE_EMBEDDED_PERL
//SV                 *psScalarValue = NULL;
#endif
  unsigned long       ulFlags = 0;

  /*-
   *********************************************************************
   *
   * Check that the hook is not NULL and that it has length.
   *
   *********************************************************************
   */
  if (pcExpression == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NULL input. That shouldn't happen.", acRoutine);
    return NULL;
  }

  iLength = strlen(pcExpression);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length must be greater than zero.", acRoutine, iLength);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory for a new hook. The caller should free this
   * memory with HookFreeHook().
   *
   *********************************************************************
   */
  psHook = calloc(sizeof(HOOK_LIST), 1);
  if (psHook == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Compile the hook expression.
   *
   *********************************************************************
   */
  ulFlags = KLEL_MUST_BE_NAMED | KLEL_MUST_BE_GUARDED_COMMAND_WITH_RETURN_CODES;
  psHook->psContext = KlelCompile(pcExpression, ulFlags, HookGetTypeOfVar, HookGetValueOfVar, NULL);
  if (!KlelIsValid(psHook->psContext))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: KlelCompile(): Hook failed to compile (%s).", acRoutine, KlelGetError(psHook->psContext));
    HookFreeHook(psHook);
    return NULL;
  }
  psHook->uiId = KlelGetChecksum(psHook->psContext, KLEL_EXPRESSION_ONLY);
  psHook->pcName = KlelGetName(psHook->psContext);
  psHook->pcExpression = KlelExpressionToString(psHook->psContext, KLEL_EXPRESSION_PLUS_EVERYTHING);
  psHook->pcInterpreter = KlelGetCommandInterpreter(psHook->psContext);
  psHook->pcProgram = KlelGetCommandProgram(psHook->psContext);
  psHook->psNext = NULL;

  /*-
   *********************************************************************
   *
   * Make sure hook values are defined.
   *
   *********************************************************************
   */
  if (psHook->pcName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: KlelGetName(): Hook failed to yield a name value.", acRoutine);
    HookFreeHook(psHook);
    return NULL;
  }
  if (psHook->pcExpression == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: KlelExpressionToString(): Hook failed to yield an expression value.", acRoutine);
    HookFreeHook(psHook);
    return NULL;
  }
  if (psHook->pcInterpreter == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: KlelGetCommandInterpreter(): Hook failed to yield an interpreter value.", acRoutine);
    HookFreeHook(psHook);
    return NULL;
  }
  if (psHook->pcProgram == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: KlelGetCommandProgram(): Hook failed to yield a program value.", acRoutine);
    HookFreeHook(psHook);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Conditionally load the hook program into an embedded interpreter.
   * This serves two purposes: 1) if the script fails to parse, we'll
   * catch it here and avoid map/dig issues that could arise later and
   * 2) the loaded script will be cached, which means it won't have to
   * be loaded each time the associated hook is activated.
   *
   *********************************************************************
   */
  if (strcmp(psHook->pcInterpreter, "exec") == 0)
  {
    /* Empty */
  }
#ifdef USE_EMBEDDED_PERL
  else if (strcmp(psHook->pcInterpreter, "perl") == 0)
  {
    char *ppcArgumentVector[] = { psHook->pcProgram, NULL };
    dSP;
    ENTER;
//  SAVETMPS; /* This should not be needed since mortal variables are not created/used. */
    call_argv("Embed::Persistent::LoadScript", G_EVAL | G_KEEPERR | G_SCALAR, ppcArgumentVector); /* Do not use G_DISCARD here so that Perl stack items are preserved. */
    SPAGAIN;
//  psScalarValue = POPs; /* This should be the package name of the just-loaded script. */
    PUTBACK;
    if (SvTRUE(ERRSV))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: call_argv(): Hook failed to load \"%s\" (%s).", acRoutine, psHook->pcProgram, SvPV_nolen(ERRSV));
      iError = ER;
    }
//  FREETMPS; /* This should not be needed since mortal variables are not created/used. */
    LEAVE;
    if (iError != ER_OK)
    {
      HookFreeHook(psHook);
      return NULL;
    }
  }
#endif
#ifdef USE_EMBEDDED_PYTHON
  else if (strcmp(psHook->pcInterpreter, "python") == 0)
  {
    if (HookLoadPythonScript(psHook, acLocalError) != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: HookLoadPythonScript(): Hook failed to load \"%s\" (%s).", acRoutine, psHook->pcProgram, acLocalError);
      HookFreeHook(psHook);
      return NULL;
    }
  }
#endif
  else if (strcmp(psHook->pcInterpreter, "system") == 0)
  {
    /* Empty */
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Hook requires an unsupported interpreter (%s).", acRoutine, psHook->pcInterpreter);
    HookFreeHook(psHook);
    return NULL;
  }

  return psHook;
}


#ifdef USE_EMBEDDED_PYTHON
/*-
 ***********************************************************************
 *
 * HookLoadPythonScript
 *
 ***********************************************************************
 */
int
HookLoadPythonScript(HOOK_LIST *psHook, char *pcError)
{
  struct stat         sStatEntry = {};
  char               *pcPyScript = NULL;
  FILE               *pFile = NULL;
  size_t              szNRead = 0;

  /*-
   *********************************************************************
   *
   * Open the Python script and load it into memory.
   *
   *********************************************************************
   */
  pFile = fopen(psHook->pcProgram, "r");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return ER;
  }
  if (fstat(fileno(pFile), &sStatEntry) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    fclose(pFile);
    return ER;
  }
  pcPyScript = malloc(sStatEntry.st_size + 1);
  if (pcPyScript == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    fclose(pFile);
    return ER;
  }
  pcPyScript[sStatEntry.st_size] = 0;

  szNRead = fread(pcPyScript, 1, sStatEntry.st_size, pFile);
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    free(pcPyScript);
    fclose(pFile);
    return ER;
  }
  if (szNRead != sStatEntry.st_size || strlen(pcPyScript) != sStatEntry.st_size)
  {
    snprintf(pcError, MESSAGE_SIZE, "byte count mismatch");
    free(pcPyScript);
    fclose(pFile);
    return ER;
  }
  fclose(pFile);

  /*-
   *********************************************************************
   *
   * Compile the Python script.
   *
   *********************************************************************
   */
  psHook->psPyScript = Py_CompileString(pcPyScript, psHook->pcProgram, Py_file_input);
  if (psHook->psPyScript == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "compile error");
    free(pcPyScript);
    return ER;
  }
  free(pcPyScript);

  return ER_OK;
}
#endif
