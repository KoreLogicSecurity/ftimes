/*-
 ***********************************************************************
 *
 * $Id: map.c,v 1.110 2014/07/30 08:13:33 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static int giDirectories;
static int giFiles;
static int giSpecial;
#ifdef WINNT
static int giStreams;
#endif

static int giRecords;
static int giIncompleteRecords;

#ifdef USE_EMBEDDED_PYTHON
/*-
 ***********************************************************************
 *
 * MapConvertPythonArguments
 *
 ***********************************************************************
 */
wchar_t **
MapConvertPythonArguments(size_t szArgumentCount, char **ppcArgumentVector)
{
  size_t              szArgument = 0;
  size_t              szLength = 0;
  wchar_t            *pwcArgument = NULL;
  wchar_t           **ppwcArgumentVector = NULL;

  /*-
   *********************************************************************
   *
   * Make sure the caller has provided valid arguments.
   *
   *********************************************************************
   */
  if (szArgumentCount < 1 || ppcArgumentVector == NULL)
  {
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate and initialize memory for a new argument vector.
   *
   *********************************************************************
   */
  ppwcArgumentVector = calloc(szArgumentCount, sizeof(wchar_t *));
  if (ppwcArgumentVector == NULL)
  {
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Convert each multibyte argument to a wide-character argument.
   *
   *********************************************************************
   */
  for (szArgument = 0; szArgument < szArgumentCount; szArgument++)
  {
    szLength = mbstowcs(NULL, ppcArgumentVector[szArgument], 0);
    if (szLength < 0)
    {
      MapFreePythonArguments(szArgumentCount, ppwcArgumentVector);
      return NULL;
    }
    pwcArgument = calloc(szLength + 1, sizeof(wchar_t));
    if (pwcArgument == NULL)
    {
      MapFreePythonArguments(szArgumentCount, ppwcArgumentVector);
      return NULL;
    }
    szLength = mbstowcs(pwcArgument, ppcArgumentVector[szArgument], szLength);
    if (szLength < 0)
    {
      MapFreePythonArguments(szArgumentCount, ppwcArgumentVector);
      return NULL;
    }
    ppwcArgumentVector[szArgument] = pwcArgument;
  }

  return ppwcArgumentVector;
}
#endif


/*-
 ***********************************************************************
 *
 * MapDirHashAlpha
 *
 ***********************************************************************
 */
void
MapDirHashAlpha(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData)
{
  /*-
   *********************************************************************
   *
   * Conditionally start directory hashes.
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    MD5Alpha(&psFTHashData->sMd5Context);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    SHA1Alpha(&psFTHashData->sSha1Context);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    SHA256Alpha(&psFTHashData->sSha256Context);
  }
}


/*-
 ***********************************************************************
 *
 * MapDirHashCycle
 *
 ***********************************************************************
 */
void
MapDirHashCycle(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData, FTIMES_FILE_DATA *psFTFileData)
{
  /*-
   *********************************************************************
   *
   * Conditionally update directory hashes. If the current file was
   * not hashed (e.g., because it's a special file or it could not be
   * opened), then its default hash value (all zeros) is folded into
   * the aggregate directory hash.
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    MD5Cycle(&psFTHashData->sMd5Context, psFTFileData->aucFileMd5, MD5_HASH_SIZE);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    SHA1Cycle(&psFTHashData->sSha1Context, psFTFileData->aucFileSha1, SHA1_HASH_SIZE);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    SHA256Cycle(&psFTHashData->sSha256Context, psFTFileData->aucFileSha256, SHA256_HASH_SIZE);
  }
}


/*-
 ***********************************************************************
 *
 * MapDirHashOmega
 *
 ***********************************************************************
 */
void
MapDirHashOmega(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData, FTIMES_FILE_DATA *psFTFileData)
{
  /*-
   *********************************************************************
   *
   * Conditionally complete directory hashes.
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    MD5Omega(&psFTHashData->sMd5Context, psFTFileData->aucFileMd5);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    SHA1Omega(&psFTHashData->sSha1Context, psFTFileData->aucFileSha1);
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    SHA256Omega(&psFTHashData->sSha256Context, psFTFileData->aucFileSha256);
  }
}


/*-
 ***********************************************************************
 *
 * MapDirname
 *
 ***********************************************************************
 */
char *
MapDirname(char *pcPath)
{
  static char         acDirname[FTIMES_MAX_PATH] = "";
  int                 n = 0;
  int                 iLength = 0;
  int                 iIndex = 0;

  /*-
   *********************************************************************
   *
   * Return "." for NULL or empty paths.
   *
   *********************************************************************
   */
  if (pcPath == NULL || pcPath[0] == 0 || iLength > FTIMES_MAX_PATH)
  {
    acDirname[n++] = '.';
    acDirname[n] = 0;
    return acDirname;
  }

  /*-
   *********************************************************************
   *
   * Set errno and return NULL for paths that are too long.
   *
   *********************************************************************
   */
  iLength = iIndex = strlen(pcPath);
  if (iLength > FTIMES_MAX_PATH)
  {
    errno = ENAMETOOLONG;
    return NULL;
  }
  iIndex--;

  /*-
   *********************************************************************
   *
   * Backup over trailing slashes.
   *
   *********************************************************************
   */
  while (iIndex > 0 && pcPath[iIndex] == FTIMES_SLASHCHAR)
  {
    iIndex--;
  }

  /*-
   *********************************************************************
   *
   * Backup until the next slash is found or nothing is left.
   *
   *********************************************************************
   */
  while (iIndex > 0 && pcPath[iIndex] != FTIMES_SLASHCHAR)
  {
    iIndex--;
  }

  /*-
   *********************************************************************
   *
   * Return "." if the index is zero and the path does not start with
   * a drive letter or a slash. Otherwise, return the drive letter or
   * slash. If the index is greater than zero, keep backing up until
   * there are no more trailing slashes.
   *
   *********************************************************************
   */
  if (iIndex == 0)
  {
#ifdef WIN32
    if (iLength >= 2 && isalpha(pcPath[0]) && pcPath[1] == ':')
    {
      acDirname[n++] = pcPath[0];
      acDirname[n++] = pcPath[1];
      acDirname[n++] = FTIMES_SLASHCHAR;
    }
    else
#endif
    if (pcPath[iIndex] == FTIMES_SLASHCHAR)
    {
      acDirname[n++] = FTIMES_SLASHCHAR;
    }
    else
    {
      acDirname[n++] = '.';
    }
    acDirname[n] = 0;
    return acDirname;
  }
  else
  {
    while (--iIndex > 0 && pcPath[iIndex] == FTIMES_SLASHCHAR);
  }
  iLength = iIndex + 1;

  /*-
   *********************************************************************
   *
   * Return anything that's left.
   *
   *********************************************************************
   */
  strncpy(acDirname, pcPath, iLength);
#ifdef WIN32
  if (iLength == 2)
  {
    acDirname[iLength++] = FTIMES_SLASHCHAR;
  }
#endif
  acDirname[iLength] = 0;

  return acDirname;
}


#ifdef USE_FILE_HOOKS
/*-
 ***********************************************************************
 *
 * MapExecuteHook
 *
 ***********************************************************************
 */
int
MapExecuteHook(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "MapExecuteHook()";
#define PIPE_READ_SIZE 8192
  char                acData[PIPE_READ_SIZE] = "";
  char                acMessage[MESSAGE_SIZE] = "";
  fd_set              sFdReadSet;
  fd_set              sFdSaveSet;
  HOOK_LIST          *psHook = NULL;
  int                 i = 0;
  int                 iFd = 0;
  int                 iError = 0;
  int                 iKidPid = 0;
  int                 iKidReturn = 0;
  int                 iKidStatus = 0;
//int                 iKidSignal = 0;
//int                 iKidDumped = 0;
  int                 iNRead = 0;
  int                 iNReady = 0;
  int                 iNToWatch = 0;
  int                 iNWritten = 0;
#define PIPE_STDIN_INDEX 0
#define PIPE_STDOUT_INDEX 1
#define PIPE_STDERR_INDEX 2
#define PIPE_READER_INDEX 0
#define PIPE_WRITER_INDEX 1
  int                 aaiPipes[3][2];
  KLEL_COMMAND       *psCommand = NULL;
#ifdef USE_EMBEDDED_PERL
  SV                 *psScalarValue = NULL;
#endif

  /*-
   *********************************************************************
   *
   * See if there's a hook that needs to be executed.
   *
   *********************************************************************
   */
  psHook = HookMatchHook(psProperties->psFileHookList, psFTFileData);
  if (!psHook)
  {
    return ER_OK;
  }

  /*-
   *********************************************************************
   *
   * Let them know we have a match.
   *
   *********************************************************************
   */
  if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
  {
    snprintf(acMessage, MESSAGE_SIZE, "FileHook=%s RawPath=%s", psHook->pcExpression, psFTFileData->pcRawPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
  }

  /*-
   *********************************************************************
   *
   * Create stdin/stdout/stderr pipes.
   *
   *********************************************************************
   */
  for (i = 0; i < 3; i++)
  {
    iError = pipe(aaiPipes[i]);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pipe(): %s", acRoutine, strerror(errno));
      return ER;
    }
  }

  /*-
   *********************************************************************
   *
   * Fork off a kid to run the specified command.
   *
   *********************************************************************
   */
  iKidPid = fork();
  if (iKidPid == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fork(): %s", acRoutine, strerror(errno));
    return ER;
  }
  else if (iKidPid == 0)
  {
    close(aaiPipes[PIPE_STDIN_INDEX][PIPE_WRITER_INDEX]);
    close(aaiPipes[PIPE_STDOUT_INDEX][PIPE_READER_INDEX]);
    close(aaiPipes[PIPE_STDERR_INDEX][PIPE_READER_INDEX]);
    dup2(aaiPipes[PIPE_STDIN_INDEX][PIPE_READER_INDEX], 0);
    dup2(aaiPipes[PIPE_STDOUT_INDEX][PIPE_WRITER_INDEX], 1);
    dup2(aaiPipes[PIPE_STDERR_INDEX][PIPE_WRITER_INDEX], 2);

/* NOTE: Perhaps this code perhaps should be placed above the fork(), but having it here eliminates the need to have KlelFreeCommand() before each return in the parent. */
    psHook->psContext->pvData = (void *)psFTFileData;
    psCommand = KlelGetCommand(psHook->psContext);
    if (psCommand == NULL)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: KlelGetCommand(): Hook (%s) failed to produce a valid command (%s).", acRoutine, psFTFileData->pcNeuteredPath, psHook->pcName, KlelGetError(psHook->psContext));
      MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
      exit(-2);
    }

    if (strcmp(psCommand->acInterpreter, "exec") == 0)
    {
      execv(psCommand->acProgram, psCommand->ppcArgumentVector);
      snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (%s).", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->acProgram, strerror(errno));
      MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
      KlelFreeCommand(psCommand);
      exit(-1);
    }
#ifdef USE_EMBEDDED_PERL
    else if (strcmp(psCommand->acInterpreter, "perl") == 0)
    {
      dSP;
      ENTER;
//    SAVETMPS; /* This should not be needed since mortal variables are not created/used. */
      call_argv("Embed::Persistent::EvalScript", G_EVAL | G_KEEPERR | G_SCALAR, psCommand->ppcArgumentVector); /* Do not use G_DISCARD here so that Perl stack items are preserved. */
      SPAGAIN;
      psScalarValue = POPs;
      PUTBACK;
      if (SvTRUE(ERRSV))
      {
        iError = -1;
        snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (%s).", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->acProgram, SvPV_nolen(ERRSV));
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
      }
      else
      {
        if (SvOK(psScalarValue) && SvIOK(psScalarValue)) /* Expect the Perl stack to contain exactly one defined integer value. */
        {
          iError = SvIV(psScalarValue);
        }
        else
        {
          iError = -1;
          snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (No $@).", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->acProgram);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
        }
      }
//    FREETMPS; /* This should not be needed since mortal variables are not created/used. */
      LEAVE;
      PL_perl_destruct_level = 1; /* This must be set to 1 since perl_construct() reset it to 0 according to perlembed. */
      perl_destruct(psProperties->psMyPerl);
      perl_free(psProperties->psMyPerl);
      PERL_SYS_TERM();
      KlelFreeCommand(psCommand);
      exit(iError);
    }
#endif
#ifdef USE_EMBEDDED_PYTHON
    else if (strcmp(psCommand->acInterpreter, "python") == 0)
    {
      iError = MapExecutePythonScript(psProperties, psHook, psCommand, psFTFileData, acMessage);
      KlelFreeCommand(psCommand);
      Py_Finalize();
      exit(iError);
    }
#endif
    else if (strcmp(psCommand->acInterpreter, "system") == 0)
    {
      if
      (
           psCommand->ppcArgumentVector[0] != NULL
        && psCommand->ppcArgumentVector[1] != NULL
        && strcmp(psCommand->ppcArgumentVector[0], "") == 0
        && strcmp(psCommand->ppcArgumentVector[1], "") != 0
      )
      {
        iError = system(psCommand->ppcArgumentVector[1]);
        if (iError == -1)
        {
          snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (%s).", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->ppcArgumentVector[1], strerror(errno));
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
        }
      }
      else
      {
        iError = -1;
        snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute (Usage: eval(\"system\", \"\", \"<command>\")).", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
      }
      KlelFreeCommand(psCommand);
      exit(iError);
    }
    else
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) requires an unsupported interpreter.", acRoutine, psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName);
      MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, acMessage);
      KlelFreeCommand(psCommand);
      exit(-1);
    }
  }
  else
  {
    close(aaiPipes[PIPE_STDIN_INDEX][PIPE_READER_INDEX]);
    close(aaiPipes[PIPE_STDOUT_INDEX][PIPE_WRITER_INDEX]);
    close(aaiPipes[PIPE_STDERR_INDEX][PIPE_WRITER_INDEX]);

    close(aaiPipes[PIPE_STDIN_INDEX][PIPE_WRITER_INDEX]); /* There's nothing to send to the kid on stdin. */

    FD_ZERO(&sFdSaveSet);
    FD_SET(aaiPipes[PIPE_STDOUT_INDEX][PIPE_READER_INDEX], &sFdSaveSet);
    iNToWatch++;
    FD_SET(aaiPipes[PIPE_STDERR_INDEX][PIPE_READER_INDEX], &sFdSaveSet);
    iNToWatch++;

    while (iNToWatch > 0)
    {
      struct timeval sTvTimeout = { 1, 0 }; /* The Linux implementation of select() modifies the timeout value, so it must be initialized before each call. */
      sFdReadSet = sFdSaveSet;
      iNReady = select(FD_SETSIZE, &sFdReadSet, NULL, NULL, &sTvTimeout);
      if (iNReady < 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: select(): Hook (%s) failed to select a file descriptor (%s).", acRoutine, psHook->pcName, strerror(errno));
        return ER;
      }
      else if (iNReady == 0)
      {
        /* Select timeout. */
      }
      else
      {
        for(iFd = 0; iFd < FD_SETSIZE; iFd++)
        {
          if (FD_ISSET(iFd, &sFdReadSet))
          {
            iNRead = read(iFd, acData, PIPE_READ_SIZE);
            if (iNRead < 0)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: read(): Hook (%s) failed to read file descriptor %d (%s)", acRoutine, psHook->pcName, iFd, strerror(errno));
              return ER;
            }
            else if (iNRead == 0)
            {
              FD_CLR(iFd, &sFdSaveSet);
              iNToWatch--;
              close(iFd);
            }
            else
            {
              if (iFd == aaiPipes[PIPE_STDOUT_INDEX][PIPE_READER_INDEX])
              {
                iNWritten = fwrite(acData, 1, iNRead, psProperties->pFileOut);
                if (iNWritten != iNRead)
                {
                  snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): Hook (%s) failed to write on file descriptor %d (%s)", acRoutine, psHook->pcName, iFd, strerror(errno));
                  return ER;
                }
                MD5Cycle(&psProperties->sOutFileHashContext, (unsigned char *) acData, iNWritten);
              }
              else if (iFd == aaiPipes[PIPE_STDERR_INDEX][PIPE_READER_INDEX])
              {
                iNWritten = fwrite(acData, 1, iNRead, psProperties->pFileLog);
                if (iNWritten != iNRead)
                {
                  snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): Hook (%s) failed to write on file descriptor %d (%s)", acRoutine, psHook->pcName, iFd, strerror(errno));
                  return ER;
                }
              }
              else
              {
                /* Empty */
              }
            }
          }
        }
      }
    }

    iKidPid = wait(&iKidStatus);
    if (iKidPid == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: wait(): %s", acRoutine, strerror(errno));
      return ER;
    }
    iKidReturn = WEXITSTATUS(iKidStatus);
//  iKidSignal = WTERMSIG(iKidStatus);
//  iKidDumped = WCOREDUMP(iKidStatus);
    if (!KlelIsSuccessReturnCode(psHook->psContext, iKidReturn))
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: wait(): Hook (%s) returned an unexpected exit code (%d).", acRoutine, psFTFileData->pcNeuteredPath, psHook->pcName, iKidReturn);
      ErrorHandler(ER_Failure, acMessage, ERROR_FAILURE);
    }
  }

  return ER_OK;
}
#endif


#ifdef USE_EMBEDDED_PYTHON
/*-
 ***********************************************************************
 *
 * MapExecutePythonScript
 *
 ***********************************************************************
 */
int
MapExecutePythonScript(FTIMES_PROPERTIES *psProperties, HOOK_LIST *psHook, KLEL_COMMAND *psCommand, FTIMES_FILE_DATA *psFTFileData, char *pcMessage)
{
  int                 iError = -1;
  PyObject           *psPyLocals = NULL;
  PyObject           *psPyResult = NULL;
  PyObject           *psPyException = NULL;
  PyObject           *psPyExceptionType = NULL;
  wchar_t           **ppwcArgumentVector = NULL;

  /*-
   *********************************************************************
   *
   * Obtain references to a new dictionary to store local variables.
   *
   *********************************************************************
   */
  psPyLocals = PyDict_New();
  if (psPyLocals == NULL)
  {
    snprintf(pcMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (could not allocate locals).", "MapExecutePythonScript()", psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->acProgram);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, pcMessage);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Convert and marshall the script's arguments.
   *
   *********************************************************************
   */
  ppwcArgumentVector = MapConvertPythonArguments(psCommand->szArgumentCount, psCommand->ppcArgumentVector);
  if (ppwcArgumentVector == NULL)
  {
    snprintf(pcMessage, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Interpreter = [%s]: Hook (%s) failed to execute \"%s\" (could not convert arguments).", "MapExecutePythonScript()", psFTFileData->pcNeuteredPath, psCommand->acInterpreter, psHook->pcName, psCommand->acProgram);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, MESSAGE_HOOK_STRING, pcMessage);
    Py_XDECREF(psPyLocals);
    return iError;
  }
  PySys_SetArgvEx(psCommand->szArgumentCount, ppwcArgumentVector, 0);

  /*-
   *********************************************************************
   *
   * Execute the script. Note that we don't care about the result of
   * the module evaluation since it is generally None. However, we do
   * care if a SystemExit exception was raised, meaning a return code
   * was provided.
   *
   *********************************************************************
   */
  psPyResult = PyEval_EvalCode(psHook->psPyScript, psProperties->psPyGlobals, psPyLocals);
  psPyExceptionType = PyErr_Occurred();
  if (psPyExceptionType != NULL)
  {
    if (PyErr_ExceptionMatches(PyExc_SystemExit))
    {
      PyErr_Fetch(&psPyExceptionType, &psPyException, NULL);
      if (PyLong_Check(psPyException))
      {
        iError = (PyLong_AsLong(psPyException) > INT_MAX) ? -1 : ((int)PyLong_AsLong(psPyException));
      }
    }
    Py_XDECREF(psPyException);
    PyErr_Clear();
  }
  else
  {
    iError = 0;
  }

  /*-
   *********************************************************************
   *
   * Release any remaining local resources.
   *
   *********************************************************************
   */
  Py_XDECREF(psPyResult);
  Py_XDECREF(psPyLocals);
  MapFreePythonArguments(psCommand->szArgumentCount, ppwcArgumentVector);

  return iError;
}
#endif


/*-
 ***********************************************************************
 *
 * MapGetDirectoryCount
 *
 ***********************************************************************
 */
int
MapGetDirectoryCount()
{
  return giDirectories;
}


/*-
 ***********************************************************************
 *
 * MapGetFileCount
 *
 ***********************************************************************
 */
int
MapGetFileCount()
{
  return giFiles;
}


/*-
 ***********************************************************************
 *
 * MapGetSpecialCount
 *
 ***********************************************************************
 */
int
MapGetSpecialCount()
{
  return giSpecial;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapGetStreamCount
 *
 ***********************************************************************
 */
int
MapGetStreamCount()
{
  return giStreams;
}
#endif


/*-
 ***********************************************************************
 *
 * MapGetRecordCount
 *
 ***********************************************************************
 */
int
MapGetRecordCount()
{
  return giRecords;
}


/*-
 ***********************************************************************
 *
 * MapGetIncompleteRecordCount
 *
 ***********************************************************************
 */
int
MapGetIncompleteRecordCount()
{
  return giIncompleteRecords;
}


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * MapTree
 *
 ***********************************************************************
 */
int
MapTree(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTTreeData, char *pcError)
{
  const char          acRoutine[] = "MapTree()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acLinkData[FTIMES_MAX_PATH] = "";
  char                acMessage[MESSAGE_SIZE] = "";
  char               *pcParentPath = NULL;
  DIR                *psDir = NULL;
  FTIMES_FILE_DATA   *psFTFileData = NULL;
  FTIMES_HASH_DATA    sFTHashData;
  int                 iError = 0;
  int                 iNewFSType = 0;
  struct dirent      *psDirEntry = NULL;
  struct stat         sStatPDirectory;
  struct stat        *psStatPDirectory = NULL;
  struct stat        *psStatCDirectory = NULL;

  /*-
   *********************************************************************
   *
   * Let them know where we're at.
   *
   *********************************************************************
   */
  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(acMessage, MESSAGE_SIZE, "FS=%s Directory=%s", gaacFSType[psFTTreeData->iFSType], psFTTreeData->pcRawPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, acMessage);
  }

  /*-
   *********************************************************************
   *
   * Conditionally start directory hashes.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    MapDirHashAlpha(psProperties, &sFTHashData);
  }

  /*-
   *********************************************************************
   *
   * Determine current and parent path attributes. These are used later
   * to check that "." and ".." are really hard links to the current
   * and parent paths, respectively. This is done to ensure that "."
   * or ".." don't point to a hidden file or directory. The attacker
   * would necessarily have to modify the directory structure for this
   * to be the case.
   *
   *********************************************************************
   */
  if (psFTTreeData->psParent == NULL)
  {
    pcParentPath = MapDirname(psFTTreeData->pcRawPath);
    if (lstat((pcParentPath == NULL) ? "" : pcParentPath, &sStatPDirectory) == ER)
    {
      psStatPDirectory = NULL;
    }
    else
    {
      psStatPDirectory = &sStatPDirectory;
    }
  }
  else
  {
    if (psFTTreeData->psParent->ulAttributeMask == 0)
    {
      psStatPDirectory = NULL;
    }
    else
    {
      psStatPDirectory = &psFTTreeData->psParent->sStatEntry;
    }
  }

  if (psFTTreeData->ulAttributeMask == 0)
  {
    psStatCDirectory = NULL;
  }
  else
  {
    psStatCDirectory = &psFTTreeData->sStatEntry;
  }

  /*-
   *********************************************************************
   *
   * Open up the directory to be scanned.
   *
   *********************************************************************
   */
  if ((psDir = opendir(psFTTreeData->pcRawPath)) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTTreeData->pcNeuteredPath, strerror(errno));
    ErrorHandler(ER_opendir, pcError, ERROR_FAILURE);
    return ER_opendir;
  }

  /*-
   *********************************************************************
   *
   * Loop through the list of directory entries. Note that errno must
   * be cleared before each readdir() call so that its value can be
   * checked after the function returns. Read the comment that follows
   * this loop for more details.
   *
   *********************************************************************
   */
  for (errno = 0; (psDirEntry = readdir(psDir)) != NULL; errno = 0, MapFreeFTFileData(psFTFileData))
  {
    /*-
     *******************************************************************
     *
     * Create and initialize a new file data structure.
     *
     *******************************************************************
     */
    psFTFileData = MapNewFTFileData(psFTTreeData, psDirEntry->d_name, acLocalError);
    if (psFTFileData == NULL)
    {
/* FIXME Need to prevent truncation in the case where the new path is larger than MESSAGE_SIZE. */
      char  acTempError[MESSAGE_SIZE] = "";
      char *pcNeuteredName = SupportNeuterString(psDirEntry->d_name, strlen(psDirEntry->d_name), acTempError);
      if (pcNeuteredName)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s%s%s]: %s", acRoutine, psFTTreeData->pcNeuteredPath, FTIMES_SLASH, pcNeuteredName, acLocalError);
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
        free(pcNeuteredName);
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: FallbackPath = [%s%s%s]: %s", acRoutine, psFTTreeData->pcRawPath, FTIMES_SLASH, psDirEntry->d_name, acLocalError);
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      }
      continue;
    }

    /*-
     *******************************************************************
     *
     * Get file attributes. This fills in several structure members.
     *
     *******************************************************************
     */
    MapGetAttributes(psFTFileData);
    if (!psFTFileData->iFileExists)
    {
      continue;
    }

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, skip it.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->psExcludeList, psFTFileData->pcRawPath) != NULL)
    {
      continue;
    }

#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If the new path is matched by an exclude filter, continue with
     * the next entry. If the new path is matched by an include
     * filter, set a flag, but keep going. Include filters do not get
     * applied until the file's type is known. This is because
     * directories must be traversed before they can be filtered.
     *
     *******************************************************************
     */
    if (psProperties->psExcludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, psFTFileData->pcRawPath);
      if (psFilter != NULL)
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
        continue;
      }
    }

    if (psProperties->psIncludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, psFTFileData->pcRawPath);
      if (psFilter == NULL)
      {
        psFTFileData->iFiltered = 1;
      }
      else
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
      }
    }
#endif

    /*-
     *******************************************************************
     *
     * No attributes means no file type, so we have to stop short.
     *
     *******************************************************************
     */
    if (psFTFileData->ulAttributeMask == 0)
    {
#ifdef USE_PCRE
      /*-
       *****************************************************************
       *
       * If this path has been filtered, we're done.
       *
       *****************************************************************
       */
      if (psFTFileData->iFiltered)
      {
        continue;
      }
#endif

      /*-
       *****************************************************************
       *
       * If this is "." or "..", we're done.
       *
       *****************************************************************
       */
      if (strcmp(psDirEntry->d_name, FTIMES_DOT) == 0 || strcmp(psDirEntry->d_name, FTIMES_DOTDOT) == 0)
      {
        continue;
      }

      /*-
       *****************************************************************
       *
       * Conditionally update directory hashes.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        MapDirHashCycle(psProperties, &sFTHashData, psFTFileData);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      continue;
    }

    /*-
     *******************************************************************
     *
     * If this is "." and it has the same inode as the current path,
     * fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * If this is ".." and it has the same inode as the parent path,
     * fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * Otherwise, attempt to process the record.
     *
     *******************************************************************
     */
    if (strcmp(psDirEntry->d_name, FTIMES_DOT) == 0)
    {
      if (psStatCDirectory != NULL)
      {
        if (psFTFileData->sStatEntry.st_ino != psStatCDirectory->st_ino)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Inode mismatch between '.' and current directory.", acRoutine, psFTFileData->pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to compare it to '.'.", acRoutine, psFTFileData->pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else if (strcmp(psDirEntry->d_name, FTIMES_DOTDOT) == 0)
    {
      if (psStatPDirectory != NULL)
      {
        if (psFTFileData->sStatEntry.st_ino != psStatPDirectory->st_ino)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Inode mismatch between '..' and parent directory.", acRoutine, psFTFileData->pcNeuteredPath);
          ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
        }
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on parent directory to compare it to '..'.", acRoutine, psFTFileData->pcNeuteredPath);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
      }
    }
    else
    {
      /*-
       *****************************************************************
       *
       * Map directories, files, and links.
       *
       *****************************************************************
       */
      if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
      {
        giDirectories++;
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          snprintf(psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, "special/directory");
        }
#endif
        /*-
         ***************************************************************
         *
         * Check for a device crossing. Process new file systems if
         * they are supported, and process remote file systems when
         * AnalyzeRemoteFiles is enabled.
         *
         ***************************************************************
         */
        if (psStatCDirectory != NULL)
        {
          if (psFTFileData->sStatEntry.st_dev != psStatCDirectory->st_dev)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Crossing a device boundary.", acRoutine, psFTFileData->pcNeuteredPath);
            ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
            /*-
             *************************************************************
             *
             * WHEN AN ERROR OCCURS OR THE FILE SYSTEM IS UNSUPPORTED,
             * WARN THE USER AND CONTINUE. IF THE FILE SYSTEM IS NFS AND
             * REMOTE SCANNING IS NOT ENABLED, WARN THE USER AND CONTINUE.
             * IF ONE OF THESE CASES SHOULD ARISE, DO NOT WRITE A ENTRY
             * IN THE OUTPUT FILE, AND DO NOT UPDATE THE DIRECTORY HASH.
             *
             *************************************************************
             */
            iNewFSType = GetFileSystemType(psFTFileData->pcRawPath, acLocalError);
            if (iNewFSType == ER || iNewFSType == FSTYPE_UNSUPPORTED)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              continue;
            }
            if (!psProperties->bAnalyzeRemoteFiles && (iNewFSType == FSTYPE_NFS || iNewFSType == FSTYPE_NFS3 || iNewFSType == FSTYPE_SMB))
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, psFTFileData->pcNeuteredPath);
              ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
              continue;
            }
            psFTFileData->iFSType = iNewFSType;
          }
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to determine a device boundary crossing.", acRoutine, psFTFileData->pcNeuteredPath);
          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
          psFTFileData->iFSType = FSTYPE_NA;
        }
        if (psProperties->bEnableRecursion)
        {
          MapTree(psProperties, psFTFileData, acLocalError);
        }
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
      }
      else if (S_ISREG(psFTFileData->sStatEntry.st_mode))
      {
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
        giFiles++;
        if (psProperties->iLastAnalysisStage > 0)
        {
          iError = AnalyzeFile(psProperties, psFTFileData, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
      }
      else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
      {
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
        giSpecial++;
        if (psProperties->bHashSymbolicLinks)
        {
          iError = readlink(psFTFileData->pcRawPath, acLinkData, FTIMES_MAX_PATH - 1);
          if (iError == ER)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Unreadable Symbolic Link: %s", acRoutine, psFTFileData->pcNeuteredPath, strerror(errno));
            ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
          }
          else
          {
            acLinkData[iError] = 0; /* Readlink does not append a NULL. */
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
            {
              MD5HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileMd5);
            }
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
            {
              SHA1HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileSha1);
            }
            if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
            {
              SHA256HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileSha256);
            }
          }
        }
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          iError = XMagicTestSpecial(psFTFileData->pcRawPath, &psFTFileData->sStatEntry, psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }
      else
      {
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
        giSpecial++;
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          iError = XMagicTestSpecial(psFTFileData->pcRawPath, &psFTFileData->sStatEntry, psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
#endif
      }

      /*-
       *****************************************************************
       *
       * Conditionally update directory hashes.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        MapDirHashCycle(psProperties, &sFTHashData, psFTFileData);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

#ifdef USE_FILE_HOOKS
      /*-
       *****************************************************************
       *
       * Conditionally execute hooks for regular files.
       *
       *****************************************************************
       */
      if (psProperties->psFileHookList && S_ISREG(psFTFileData->sStatEntry.st_mode))
      {
        iError = MapExecuteHook(psProperties, psFTFileData, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
          ErrorHandler(iError, pcError, ERROR_CRITICAL);
        }
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * A NULL could mean EOD or error. The question is whether or not
   * errno is set by readdir(). We explicitly set errno to 0 before
   * each call to readdir(). So, in theory if readdir() actually
   * fails, then errno should be something other than 0. Unfortunately,
   * linux and freebsd man pages aren't explicit about their return
   * values for readdir().
   *
   *********************************************************************
   */
  if (psDirEntry == NULL && errno != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTTreeData->pcNeuteredPath, strerror(errno));
    ErrorHandler(ER_readdir, pcError, ERROR_FAILURE);
  }

  /*-
   *********************************************************************
   *
   * Conditionally complete directory hashes.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    MapDirHashOmega(psProperties, &sFTHashData, psFTTreeData);
  }

  closedir(psDir);
  return ER_OK;
}
#endif


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * MapGetFileHandleW
 *
 ***********************************************************************
 */
HANDLE
MapGetFileHandleW(wchar_t *pwcPath)
{
  /*-
   *********************************************************************
   *
   * This is just a wrapper for CreateFile(). The caller must check
   * the return value to ensure that no error has occurred. Directories
   * specifically require the FILE_FLAG_BACKUP_SEMANTICS flag. This
   * does not seem to affect the opening of regular files. In the
   * past, the desired access flag was set to 0, which means device
   * query access. However, the flag has been changed to GENERIC_READ,
   * which includes READ_CONTROL, so that the information in security
   * descriptors (e.g., owner/group SIDs and DACL) can be read as well.
   *
   * Update 1: GENERIC_READ was causing this function to fail on files
   * where it used to work. However, reverting to the previous value
   * (i.e., 0) won't work because that would cause GetSecurityInfo()
   * to fail. The current value of READ_CONTROL seems to produce the
   * best results.
   *
   *********************************************************************
   */
  return CreateFileW(
    pwcPath,
    READ_CONTROL,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
    NULL
    );
}


/*-
 ***********************************************************************
 *
 * MapTree
 *
 ***********************************************************************
 */
int
MapTree(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTTreeData, char *pcError)
{
  const char          acRoutine[] = "MapTree()";
  BOOL                bResult;
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  BY_HANDLE_FILE_INFORMATION sFileInfoCurrent;
//  BY_HANDLE_FILE_INFORMATION sFileInfoParent;
//END (\\?\)
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acMessage[MESSAGE_SIZE] = "";
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  char               *pcParentPath = NULL;
//END (\\?\)
  char               *pcMessage = NULL;
  FTIMES_FILE_DATA   *psFTFileData = NULL;
  FTIMES_HASH_DATA    sFTHashData;
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  HANDLE              hFileCurrent;
//  HANDLE              hFileParent;
//END (\\?\)
  HANDLE              hSearch;
  int                 iError = 0;
  wchar_t             awcSearchPath[FTIMES_MAX_PATH] = L"";
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  wchar_t            *pwcParentPath = NULL;
//END (\\?\)
  WIN32_FIND_DATAW    sFindDataW;

  /*-
   *********************************************************************
   *
   * Let them know where we're at.
   *
   *********************************************************************
   */
  if (psProperties->iLogLevel <= MESSAGE_WAYPOINT)
  {
    snprintf(acMessage, MESSAGE_SIZE, "FS=%s Directory=%s", gaacFSType[psFTTreeData->iFSType], psFTTreeData->pcRawPath);
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_WAYPOINT, MESSAGE_WAYPOINT_STRING, acMessage);
  }

  /*-
   *********************************************************************
   *
   * Conditionally start directory hashes.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    MapDirHashAlpha(psProperties, &sFTHashData);
  }

  /*-
   *********************************************************************
   *
   * Create a search path to match all files (i.e., append "\*").
   *
   *********************************************************************
   */
  if (psFTTreeData->iUtf8RawPathLength > FTIMES_MAX_PATH - 3)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: SearchPath = [%s%s*]: Length (%d) exceeds %d bytes.", acRoutine, psFTTreeData->pcRawPath, FTIMES_SLASH, psFTTreeData->iUtf8RawPathLength, FTIMES_MAX_PATH - 3);
    ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
    return ER_Length;
  }
  _snwprintf(awcSearchPath, FTIMES_MAX_PATH, L"%s%s*", psFTTreeData->pwcRawPath, FTIMES_SLASH_W);

  /*-
   *********************************************************************
   *
   * Begin the search.
   *
   *********************************************************************
   */
  hSearch = FindFirstFileW(awcSearchPath, &sFindDataW);
  if (hSearch == INVALID_HANDLE_VALUE)
  {
    ErrorFormatWinxError(GetLastError(), &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTTreeData->pcNeuteredPath, pcMessage);
    ErrorHandler(ER_FindFirstFile, pcError, ERROR_FAILURE);
    return ER_FindFirstFile;
  }

  /*-
   *********************************************************************
   *
   * Loop through the list of directory entries.
   *
   *********************************************************************
   */
  for (bResult = TRUE; bResult == TRUE; MapFreeFTFileData(psFTFileData), bResult = FindNextFileW(hSearch, &sFindDataW))
  {
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
    /*-
     *******************************************************************
     *
     * If this is "." or "..", we're done.
     *
     *******************************************************************
     */
    if (wcscmp(sFindDataW.cFileName, FTIMES_DOT_W) == 0 || wcscmp(sFindDataW.cFileName, FTIMES_DOTDOT_W) == 0)
    {
      continue;
    }
//END (\\?\)

    /*-
     *******************************************************************
     *
     * Create and initialize a new file data structure.
     *
     *******************************************************************
     */
    psFTFileData = MapNewFTFileDataW(psFTTreeData, sFindDataW.cFileName, acLocalError);
    if (psFTFileData == NULL)
    {
/* FIXME Need to prevent truncation in the case where the new path is larger than MESSAGE_SIZE. */
      char  acTempError[MESSAGE_SIZE] = "";
      char *pcUtf8Name = MapWideToUtf8(sFindDataW.cFileName, -1, acTempError);
      if (pcUtf8Name)
      {
        char *pcNeuteredName = SupportNeuterString(pcUtf8Name, strlen(pcUtf8Name), acTempError);
        if (pcNeuteredName)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s%s%s]: %s", acRoutine, psFTTreeData->pcNeuteredPath, FTIMES_SLASH, pcNeuteredName, acLocalError);
          ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
          free(pcNeuteredName);
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: FallbackPath = [%s%s%s]: %s", acRoutine, psFTTreeData->pcRawPath, FTIMES_SLASH, pcUtf8Name, acLocalError);
          ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
        }
        free(pcUtf8Name);
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: FallbackTree = [%s]: %s", acRoutine, psFTTreeData->pcRawPath, acLocalError);
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      }
      continue;
    }

    /*-
     *******************************************************************
     *
     * Get file attributes. This fills in several structure members.
     *
     *******************************************************************
     */
    MapGetAttributes(psFTFileData);
    if (!psFTFileData->iFileExists)
    {
      continue;
    }

    /*-
     *******************************************************************
     *
     * If the new path is in the exclude list, skip it.
     *
     *******************************************************************
     */
    if (SupportMatchExclude(psProperties->psExcludeList, psFTFileData->pcRawPath) != NULL)
    {
      continue;
    }

#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If the new path is matched by an exclude filter, continue with
     * the next entry. If the new path is matched by an include
     * filter, set a flag, but keep going. Include filters do not get
     * applied until the file's type is known. This is because
     * directories must be traversed before they can be filtered.
     *
     *******************************************************************
     */
    if (psProperties->psExcludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, psFTFileData->pcRawPath);
      if (psFilter != NULL)
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
        continue;
      }
    }

    if (psProperties->psIncludeFilterList)
    {
      FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, psFTFileData->pcRawPath);
      if (psFilter == NULL)
      {
        psFTFileData->iFiltered = 1;
      }
      else
      {
        if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
        {
          snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
          MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
        }
      }
    }
#endif

    /*-
     *******************************************************************
     *
     * No attributes means no file type, so we have to stop short.
     *
     *******************************************************************
     */
    if (psFTFileData->ulAttributeMask == 0)
    {
#ifdef USE_PCRE
      /*-
       *****************************************************************
       *
       * If this path has been filtered, we're done.
       *
       *****************************************************************
       */
      if (psFTFileData->iFiltered)
      {
        continue;
      }
#endif

//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//      /*-
//       *****************************************************************
//       *
//       * If this is "." or "..", we're done.
//       *
//       *****************************************************************
//       */
//      if (wcscmp(sFindDataW.cFileName, FTIMES_DOT_W) == 0 || wcscmp(sFindDataW.cFileName, FTIMES_DOTDOT_W) == 0)
//      {
//        continue;
//      }
//END (\\?\)

      /*-
       *****************************************************************
       *
       * Conditionally update directory hashes.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        MapDirHashCycle(psProperties, &sFTHashData, psFTFileData);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data. In this case we only have a name.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

      continue;
    }

    /*-
     *******************************************************************
     *
     * If this is "." and it has the same volume and file index as the
     * current path, fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * If this is ".." and it has the same volume and file index as the
     * parent path, fall through to the bottom of the loop.
     *
     *******************************************************************
     *
     * Otherwise, attempt to process the record.
     *
     *******************************************************************
     */
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//    if (wcscmp(sFindDataW.cFileName, FTIMES_DOT_W) == 0)
//    {
//      if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, (MAP_VOLUME | MAP_FINDEX)))
//      {
//        hFileCurrent = MapGetFileHandleW(psFTTreeData->pwcRawPath);
//        if (hFileCurrent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileCurrent, &sFileInfoCurrent))
//        {
//          if (sFileInfoCurrent.dwVolumeSerialNumber != psFTFileData->dwVolumeSerialNumber ||
//              sFileInfoCurrent.nFileIndexHigh != psFTFileData->dwFileIndexHigh ||
//              sFileInfoCurrent.nFileIndexLow != psFTFileData->dwFileIndexLow)
//          {
//            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Volume/FileIndex mismatch between '.' and current directory.", acRoutine, psFTFileData->pcNeuteredPath);
//            ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
//          }
//          CloseHandle(hFileCurrent);
//        }
//        else
//        {
//          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on current directory to compare it to '.'.", acRoutine, psFTFileData->pcNeuteredPath);
//          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
//        }
//      }
//      else
//      {
//        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on '.' to compare it to current directory.", acRoutine, psFTFileData->pcNeuteredPath);
//        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
//      }
//    }
//    else if (wcscmp(sFindDataW.cFileName, FTIMES_DOTDOT_W) == 0)
//    {
//      /*-
//       *****************************************************************
//       *
//       * If the file system is remote, skip this check. This is done
//       * because, in testing, the file index for '..' is different than
//       * the parent directory. This was found to be true with NTFS and
//       * Samba shares which, by-the-way, show up as NTFS_REMOTE. For
//       * now, these quirks remain unexplained. NWFS_REMOTE was added
//       * here to follow suit with the other file systems -- i.e., it
//       * has not been tested to see if the '..' issue exists.
//       *
//       *****************************************************************
//       */
//      if (psFTFileData->iFSType != FSTYPE_NTFS_REMOTE && psFTFileData->iFSType != FSTYPE_FAT_REMOTE && psFTFileData->iFSType != FSTYPE_NWFS_REMOTE)
//      {
//        if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, (MAP_VOLUME | MAP_FINDEX)))
//        {
//          pcParentPath = MapDirname(psFTTreeData->pcRawPath);
//          pwcParentPath = MapUtf8ToWide((pcParentPath) ? pcParentPath : "", -1, acLocalError);
//          hFileParent = MapGetFileHandleW((pwcParentPath) ? pwcParentPath : L"");
//          if (hFileParent != INVALID_HANDLE_VALUE && GetFileInformationByHandle(hFileParent, &sFileInfoParent))
//          {
//            if (sFileInfoParent.dwVolumeSerialNumber != psFTFileData->dwVolumeSerialNumber ||
//                sFileInfoParent.nFileIndexHigh != psFTFileData->dwFileIndexHigh ||
//                sFileInfoParent.nFileIndexLow != psFTFileData->dwFileIndexLow)
//            {
//              snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Volume/FileIndex mismatch between '..' and parent directory.", acRoutine, psFTFileData->pcNeuteredPath);
//              ErrorHandler(ER_BadValue, pcError, ERROR_WARNING);
//            }
//            CloseHandle(hFileParent);
//          }
//          else
//          {
//            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on parent directory to compare it to '..'.", acRoutine, psFTFileData->pcNeuteredPath);
//            ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
//          }
//        }
//        else
//        {
//          snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Not enough information on '..' to compare it to parent directory.", acRoutine, psFTFileData->pcNeuteredPath);
//          ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
//        }
//      }
//    }
//    else
//END (\\?\)
    {
      /*-
       *****************************************************************
       *
       * Map directories and files.
       *
       *****************************************************************
       */
      if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
      {
        giDirectories++;
#ifdef USE_XMAGIC
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
        {
          snprintf(psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, "special/directory");
        }
#endif
        if (psProperties->bEnableRecursion)
        {
          MapTree(psProperties, psFTFileData, acLocalError);
        }
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
      }
      else
      {
#ifdef USE_PCRE
        if (psFTFileData->iFiltered) /* We're done. */
        {
          continue;
        }
#endif
        giFiles++;
        if (psProperties->iLastAnalysisStage > 0)
        {
          iError = AnalyzeFile(psProperties, psFTFileData, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
            ErrorHandler(iError, pcError, ERROR_FAILURE);
          }
        }
      }

      /*-
       *****************************************************************
       *
       * Conditionally update directory hashes.
       *
       *****************************************************************
       */
      if (psProperties->bHashDirectories)
      {
        MapDirHashCycle(psProperties, &sFTHashData, psFTFileData);
      }

      /*-
       *****************************************************************
       *
       * Record the collected data.
       *
       *****************************************************************
       */
      iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(iError, pcError, ERROR_CRITICAL);
      }

#ifdef WINNT
      /*-
       *****************************************************************
       *
       * Process alternate streams. This applies only to NTFS.
       *
       *****************************************************************
       */
      if (psFTFileData->iStreamCount > 0)
      {
        MapStream(psProperties, psFTFileData, &sFTHashData, acLocalError);
      }
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * Conditionally complete directory hashes.
   *
   *********************************************************************
   */
  if (psProperties->bHashDirectories)
  {
    MapDirHashOmega(psProperties, &sFTHashData, psFTTreeData);
  }

  FindClose(hSearch);
  return ER_OK;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapStream
 *
 ***********************************************************************
 */
void
MapStream(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, FTIMES_HASH_DATA *psFTHashData, char *pcError)
{
  const char          acRoutine[] = "MapStream()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acRawPath[FTIMES_MAX_PATH] = "";
  char               *pcNeuteredPath = NULL;
  char               *pcStreamName = NULL;
  FTIMES_FILE_DATA    sFTFileData;
  FILE_STREAM_INFORMATION *psFSI = (FILE_STREAM_INFORMATION *) psFTFileData->pucStreamInfo;
  int                 iDone = 0;
  int                 iError = 0;
  int                 iLength = 0;
  int                 iNameLength = 0;
  int                 iNextEntryOffset = 0;
  wchar_t             awcRawPath[FTIMES_MAX_PATH] = L"";
  wchar_t             awcStreamName[FTIMES_MAX_PATH] = L"";

  giStreams += psFTFileData->iStreamCount;

  /*-
   *********************************************************************
   *
   * Make a local copy of the file data. If the stream belongs to
   * a directory, clear the attributes field. If this isn't done,
   * the output routine, will overwrite the hash field with "DIRECTORY"
   * or "D" respectively.
   *
   *********************************************************************
   */
/* FIXME Look into using MapNewFTFileData() here. */
  memcpy(&sFTFileData, psFTFileData, sizeof(FTIMES_FILE_DATA));

  sFTFileData.pcRawPath = acRawPath;

  sFTFileData.iStreamCount = 0;

  if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
  {
    sFTFileData.dwFileAttributes = 0;
  }

  /*-
   *********************************************************************
   *
   * Process each stream, but skip the default stream.
   *
   *********************************************************************
   */
  for (iDone = iNextEntryOffset = 0; !iDone; iNextEntryOffset = psFSI->NextEntryOffset)
  {
    psFSI = (FILE_STREAM_INFORMATION *) ((byte *) psFSI + iNextEntryOffset);
    if (psFSI->NextEntryOffset == 0)
    {
      iDone = 1; /* Instruct the loop to terminate after this pass. */
    }

    /*-
     *******************************************************************
     *
     * Skip unnamed streams. Remove the ":$DATA" suffix since it's not
     * part of the stream name as stored on disk in the MFT and it can
     * result in paths that exceed MAX_PATH. Convert the result to
     * UTF-8.
     *
     *******************************************************************
     */
    iLength = psFSI->StreamNameLength / sizeof(wchar_t);
    if (wcscmp(&psFSI->StreamName[iLength - 6], L":$DATA") == 0)
    {
      if (psFSI->StreamName[iLength - 7] == L':')
      {
        continue;
      }
      iLength -= 6;
    }
    wcsncpy(awcStreamName, psFSI->StreamName, iLength);
    awcStreamName[iLength] = 0;
    pcStreamName = MapWideToUtf8(awcStreamName, iLength + 1, acLocalError);
    if (pcStreamName == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: UTF-8 conversion failed for a stream associated with this file.", acRoutine, psFTFileData->pcRawPath);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      continue;
    }

    /*-
     *******************************************************************
     *
     * Figure out if the new path length will be too long. If yes, warn
     * the user, and continue with the next stream.
     *
     *******************************************************************
     */
    iNameLength = strlen(psFTFileData->pcRawPath) + iLength;
    if (iNameLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s%s]: Length (%d) exceeds %d bytes.",
        acRoutine,
        psFTFileData->pcRawPath,
        pcStreamName,
        iNameLength,
        FTIMES_MAX_PATH - 1
        );
      ErrorHandler(ER_Length, pcError, ERROR_FAILURE);
      MEMORY_FREE(pcStreamName);
      continue;
    }
    snprintf(acRawPath, FTIMES_MAX_PATH, "%s%s", psFTFileData->pcRawPath, pcStreamName);
    sFTFileData.pcRawPath = acRawPath;

    _snwprintf(awcRawPath, FTIMES_MAX_PATH, L"%s%s", psFTFileData->pwcRawPath, awcStreamName);
    sFTFileData.pwcRawPath = awcRawPath;

    /*-
     *******************************************************************
     *
     * Neuter the new given path.
     *
     *******************************************************************
     */
    pcNeuteredPath = SupportNeuterString(acRawPath, iNameLength, acLocalError);
    if (pcNeuteredPath == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: RawPath = [%s]: %s", acRoutine, acRawPath, acLocalError);
      ErrorHandler(ER_NeuterPathname, pcError, ERROR_FAILURE);
      MEMORY_FREE(pcStreamName);
      continue;
    }
    sFTFileData.pcNeuteredPath = pcNeuteredPath;

    /*-
     *******************************************************************
     *
     * Set the file attributes that are unique to this stream.
     *
     *******************************************************************
     */
    sFTFileData.dwFileSizeHigh = (DWORD) (psFSI->StreamSize.QuadPart >> 32);
    sFTFileData.dwFileSizeLow = (DWORD) psFSI->StreamSize.QuadPart;

    /*-
     *******************************************************************
     *
     * Analyze the stream's content.
     *
     *******************************************************************
     */
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, &sFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }

    /*-
     *******************************************************************
     *
     * Conditionally update directory hashes. If psFTHashData is NULL,
     * assume the caller was MapFile() and skip this step -- directory
     * hashes are not computed for includes that are individual files.
     *
     *******************************************************************
     */
    if (psProperties->bHashDirectories && psFTHashData != NULL)
    {
      MapDirHashCycle(psProperties, psFTHashData, &sFTFileData);
    }

    /*-
     *******************************************************************
     *
     * Record the collected data.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, &sFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the neutered path and stream name.
     *
     *******************************************************************
     */
    MEMORY_FREE(pcNeuteredPath);
    MEMORY_FREE(pcStreamName);
  }
}


/*-
 ***********************************************************************
 *
 * MapCountNamedStreams
 *
 ***********************************************************************
 */
int
MapCountNamedStreams(HANDLE hFile, int *piStreamCount, unsigned char **ppucStreamInfo, char *pcError)
{
  const char          acRoutine[] = "MapCountNamedStreams()";
  char               *pcMessage = NULL;
  DWORD               dwStatus = 0;
  FILE_STREAM_INFORMATION *psFSI = NULL;
  FILE_STREAM_INFORMATION *psTempFSI = NULL;
  int                 i = 0;
  int                 iDone = 0;
  int                 iNextEntryOffset = 0;
  IO_STATUS_BLOCK     sIOStatusBlock;
  unsigned long       ulSize = 0;

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x00000000
#endif
#ifndef STATUS_BUFFER_OVERFLOW
#define STATUS_BUFFER_OVERFLOW 0x80000005
#endif

  /*-
   *********************************************************************
   *
   * Make sure the provided file handle is valid.
   *
   *********************************************************************
   */
  if (hFile == INVALID_HANDLE_VALUE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid File Handle", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Request the file's stream information. Loop until enough memory
   * has been allocated to hold the data. However, do not exceed the
   * maximum size limit.
   *
   *********************************************************************
   */
  i = 0; psFSI = psTempFSI = NULL;
  do
  {
    ulSize = FTIMES_STREAM_INFO_SIZE << i;
    if (ulSize > FTIMES_MAX_STREAM_INFO_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Requested buffer size would exceed the maximum size limit (%lu bytes).", acRoutine, FTIMES_MAX_STREAM_INFO_SIZE);
      MEMORY_FREE(psFSI);
      return ER;
    }
    psTempFSI = (FILE_STREAM_INFORMATION *) realloc(psFSI, ulSize);
    if (psTempFSI == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
      MEMORY_FREE(psFSI);
      return ER;
    }
    memset(psTempFSI, 0, ulSize);
    psFSI = psTempFSI;
    dwStatus = NtdllNQIF(hFile, &sIOStatusBlock, psFSI, ulSize, FileStreamInformation);
    if (dwStatus != STATUS_SUCCESS && dwStatus != STATUS_BUFFER_OVERFLOW)
    {
      SetLastError(LsaNtStatusToWinError(dwStatus));
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: NtQueryInformationFile(): %s", acRoutine, pcMessage);
      MEMORY_FREE(psFSI);
      return ER;
    }
    i++;
  } while (dwStatus == STATUS_BUFFER_OVERFLOW);

  /*-
   *********************************************************************
   *
   * Record the final FSI pointer.
   *
   *********************************************************************
   */
  *ppucStreamInfo = (unsigned char *) psFSI;

  /*-
   *********************************************************************
   *
   * Count all but the default stream. This logic is supposed to work
   * even if NtQueryInformationFile() returns no data. This is due to
   * the fact that psFSI should be pointing to a zero-initialized
   * block of memory that is large enough to contain at least one FSI
   * struct. In other words, NextEntryOffset will be set to zero, and
   * the loop will terminate after one pass. Note: At least one pass
   * through the loop is required to catch directories that have only
   * one named stream. This is necessary because directories do not
   * have an unnamed (i.e., default) stream -- see Data attribute in
   * Table 9-1 of Inside Windows NT Second Edition, page 412.
   *
   *********************************************************************
   */
  for (*piStreamCount = iDone = iNextEntryOffset = 0; !iDone; iNextEntryOffset = psFSI->NextEntryOffset)
  {
    psFSI = (FILE_STREAM_INFORMATION *) ((byte *) psFSI + iNextEntryOffset);
    if (psFSI->NextEntryOffset == 0)
    {
      iDone = 1; /* Instruct the loop to terminate after this pass. */
    }
    if (psFSI->StreamNameLength && wcscmp(psFSI->StreamName, DEFAULT_STREAM_NAME_W) != 0)
    {
      (*piStreamCount)++;
    }
  }

  return ER_OK;
}
#endif /* WINNT */
#endif /* WIN32 */


#ifdef UNIX
int
MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError)
{
  const char          acRoutine[] = "MapFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acLinkData[FTIMES_MAX_PATH] = "";
  FTIMES_FILE_DATA   *psFTFileData = NULL;
  int                 iError = 0;
  int                 iFSType = 0;
  int                 iLength = strlen(pcPath);
#ifdef USE_PCRE
  char                acMessage[MESSAGE_SIZE] = "";
#endif

  /*-
   *********************************************************************
   *
   * Create and initialize a new file data structure.
   *
   *********************************************************************
   */
  psFTFileData = MapNewFTFileData(NULL, pcPath, acLocalError);
  if (psFTFileData == NULL)
  {
/* FIXME Need to prevent truncation in the case where the new path is larger than MESSAGE_SIZE. */
    char  acTempError[MESSAGE_SIZE] = "";
    char *pcNeuteredPath = SupportNeuterString(pcPath, iLength, acTempError);
    if (pcNeuteredPath)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      free(pcNeuteredPath);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: FallbackPath = [%s]: %s", acRoutine, pcPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
    }
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Get file attributes. This fills in several structure members.
   *
   *********************************************************************
   */
  MapGetAttributes(psFTFileData);
  if (!psFTFileData->iFileExists)
  {
    return ER_OK;
  }

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * If the path is matched by an exclude filter, just return. If the
   * path is matched by an include filter, set a flag, but keep going.
   * Include filters do not get applied until the file's type is
   * known. This is because directories must be traversed before they
   * can be filtered.
   *
   *********************************************************************
   */
  if (psProperties->psExcludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, psFTFileData->pcRawPath);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      return ER_OK;
    }
  }

  if (psProperties->psIncludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, psFTFileData->pcRawPath);
    if (psFilter == NULL)
    {
      psFTFileData->iFiltered = 1;
    }
    else
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * If the file system is remote and remote scanning is disabled, we're done.
   *
   *********************************************************************
   */
  iFSType = psFTFileData->iFSType;
  if (!psProperties->bAnalyzeRemoteFiles && (iFSType == FSTYPE_NFS || iFSType == FSTYPE_NFS3 || iFSType == FSTYPE_SMB))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, psFTFileData->pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MapFreeFTFileData(psFTFileData);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * No attributes means no file type, so we have to stop short.
   *
   *********************************************************************
   */
  if (psFTFileData->ulAttributeMask == 0)
  {
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If this path has been filtered, we're done.
     *
     *******************************************************************
     */
    if (psFTFileData->iFiltered)
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif

    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the file data structure.
     *
     *******************************************************************
     */
    MapFreeFTFileData(psFTFileData);
    return ER;
  }


  /*-
   *********************************************************************
   *
   * Map directories, files, and links. Everything else is considered
   * a special file. In that case write out any attributes have been
   * collected.
   *
   *********************************************************************
   */
  if (S_ISDIR(psFTFileData->sStatEntry.st_mode))
  {
    giDirectories++;
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      snprintf(psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, "special/directory");
    }
#endif
    MapTree(psProperties, psFTFileData, acLocalError);
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
  }
  else if (S_ISREG(psFTFileData->sStatEntry.st_mode) || ((S_ISBLK(psFTFileData->sStatEntry.st_mode) || S_ISCHR(psFTFileData->sStatEntry.st_mode)) && psProperties->bAnalyzeDeviceFiles))
  {
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
    giFiles++;
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
  }
  else if (S_ISLNK(psFTFileData->sStatEntry.st_mode))
  {
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
    giSpecial++;
    if (psProperties->bHashSymbolicLinks)
    {
      iError = readlink(psFTFileData->pcRawPath, acLinkData, FTIMES_MAX_PATH - 1);
      if (iError == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Unreadable Symbolic Link: %s", acRoutine, psFTFileData->pcNeuteredPath, strerror(errno));
        ErrorHandler(ER_readlink, pcError, ERROR_FAILURE);
      }
      else
      {
        acLinkData[iError] = 0; /* Readlink does not append a NULL. */
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
        {
          MD5HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileMd5);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
        {
          SHA1HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileSha1);
        }
        if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
        {
          SHA256HashString((unsigned char *) acLinkData, strlen(acLinkData), psFTFileData->aucFileSha256);
        }
      }
    }
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      iError = XMagicTestSpecial(psFTFileData->pcRawPath, &psFTFileData->sStatEntry, psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
#endif
  }
  else
  {
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
    giSpecial++;
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      iError = XMagicTestSpecial(psFTFileData->pcRawPath, &psFTFileData->sStatEntry, psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
#endif
  }

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

#ifdef USE_FILE_HOOKS
  /*-
   *********************************************************************
   *
   * Conditionally execute hooks for regular files.
   *
   *********************************************************************
   */
  if (psProperties->psFileHookList && S_ISREG(psFTFileData->sStatEntry.st_mode))
  {
    iError = MapExecuteHook(psProperties, psFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * Free the file data structure.
   *
   *********************************************************************
   */
  MapFreeFTFileData(psFTFileData);

  return ER_OK;
}
#endif


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * MapFile
 *
 ***********************************************************************
 */
int
MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError)
{
  const char          acRoutine[] = "MapFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_FILE_DATA   *psFTFileData = NULL;
  int                 iError = 0;
  int                 iFSType = 0;
  int                 iLength = strlen(pcPath);
#ifdef USE_PCRE
  char                acMessage[MESSAGE_SIZE] = "";
#endif
  wchar_t            *pwcPath = NULL;

  /*-
   *********************************************************************
   *
   * Internally, paths are handled as wide character strings, so the
   * initial conversion is done here.
   *
   *********************************************************************
   */
  pwcPath = MapUtf8ToWide(pcPath, iLength + 1, acLocalError);
  if (pwcPath == NULL)
  {
/* FIXME Need to prevent truncation in the case where the new path is larger than MESSAGE_SIZE. */
    char  acTempError[MESSAGE_SIZE] = "";
    char *pcNeuteredPath = SupportNeuterString(pcPath, iLength, acTempError);
    if (pcNeuteredPath)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      free(pcNeuteredPath);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: FallbackPath = [%s]: %s", acRoutine, pcPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
    }
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Create and initialize a new file data structure.
   *
   *********************************************************************
   */
  psFTFileData = MapNewFTFileDataW(NULL, pwcPath, acLocalError);
  if (psFTFileData == NULL)
  {
/* FIXME Need to prevent truncation in the case where the new path is larger than MESSAGE_SIZE. */
    char  acTempError[MESSAGE_SIZE] = "";
    char *pcNeuteredPath = SupportNeuterString(pcPath, iLength, acTempError);
    if (pcNeuteredPath)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, pcNeuteredPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
      free(pcNeuteredPath);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: FallbackPath = [%s]: %s", acRoutine, pcPath, acLocalError);
      ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
    }
    return ER;
  }
  MEMORY_FREE(pwcPath);

  /*-
   *********************************************************************
   *
   * Get file attributes. This fills in several structure members.
   *
   *********************************************************************
   */
  MapGetAttributes(psFTFileData);
  if (!psFTFileData->iFileExists)
  {
    return ER_OK;
  }

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * If the path is matched by an exclude filter, just return. If the
   * path is matched by an include filter, set a flag, but keep going.
   * Include filters do not get applied until the file's type is
   * known. This is because directories must be traversed before they
   * can be filtered.
   *
   *********************************************************************
   */
  if (psProperties->psExcludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psExcludeFilterList, psFTFileData->pcRawPath);
    if (psFilter != NULL)
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "ExcludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
      return ER_OK;
    }
  }

  if (psProperties->psIncludeFilterList)
  {
    FILTER_LIST *psFilter = SupportMatchFilter(psProperties->psIncludeFilterList, psFTFileData->pcRawPath);
    if (psFilter == NULL)
    {
      psFTFileData->iFiltered = 1;
    }
    else
    {
      if (psProperties->iLogLevel <= MESSAGE_DEBUGGER)
      {
        snprintf(acMessage, MESSAGE_SIZE, "IncludeFilter=%s RawPath=%s", psFilter->pcFilter, psFTFileData->pcRawPath);
        MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);
      }
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * If the file system is remote and remote scanning is disabled, we're done.
   *
   *********************************************************************
   */
  iFSType = psFTFileData->iFSType;
  if (!psProperties->bAnalyzeRemoteFiles && (iFSType == FSTYPE_NTFS_REMOTE || iFSType == FSTYPE_FAT_REMOTE || iFSType == FSTYPE_NWFS_REMOTE))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Excluding remote file system.", acRoutine, psFTFileData->pcNeuteredPath);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    MapFreeFTFileData(psFTFileData);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * No attributes means no file type, so we have to stop short.
   *
   *********************************************************************
   */
  if (psFTFileData->ulAttributeMask == 0)
  {
#ifdef USE_PCRE
    /*-
     *******************************************************************
     *
     * If this path has been filtered, we're done.
     *
     *******************************************************************
     */
    if (psFTFileData->iFiltered)
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif

    /*-
     *******************************************************************
     *
     * Record the collected data. In this case we only have a name.
     *
     *******************************************************************
     */
    iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(iError, pcError, ERROR_CRITICAL);
    }

    /*-
     *******************************************************************
     *
     * Free the file data structure.
     *
     *******************************************************************
     */
    MapFreeFTFileData(psFTFileData);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Map directories and files.
   *
   *********************************************************************
   */
  if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
  {
    giDirectories++;
#ifdef USE_XMAGIC
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
    {
      snprintf(psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, "special/directory");
    }
#endif
    MapTree(psProperties, psFTFileData, acLocalError);
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
  }
  else
  {
#ifdef USE_PCRE
    if (psFTFileData->iFiltered) /* We're done. */
    {
      MapFreeFTFileData(psFTFileData);
      return ER_OK;
    }
#endif
    giFiles++;
    if (psProperties->iLastAnalysisStage > 0)
    {
      iError = AnalyzeFile(psProperties, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
        ErrorHandler(iError, pcError, ERROR_FAILURE);
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = MapWriteRecord(psProperties, psFTFileData, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
    ErrorHandler(iError, pcError, ERROR_CRITICAL);
  }

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Process alternate streams. This applies only to NTFS. The NULL
   * argument causes MapStream to skip directory hashing -- even if
   * the user has enabled HashDirectories.
   *
   *********************************************************************
   */
  if (psFTFileData->iStreamCount > 0)
  {
    MapStream(psProperties, psFTFileData, NULL, acLocalError);
  }
#endif

  /*-
   *********************************************************************
   *
   * Free the file data structure.
   *
   *********************************************************************
   */
  MapFreeFTFileData(psFTFileData);

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * MapWriteRecord
 *
 ***********************************************************************
 */
int
MapWriteRecord(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "MapWriteRecord()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;
  int                 iIndex = 0;
  int                 iWriteCount = 0;

#ifdef UNIX
  /*-
   *********************************************************************
   *
   * prefix        4
   * name          1 (for quote) + (3 * FTIMES_MAX_PATH) + 1 (for quote)
   * dev           FTIMES_MAX_32BIT_SIZE
   * inode         FTIMES_MAX_32BIT_SIZE
   * mode          FTIMES_MAX_32BIT_SIZE
   * nlink         FTIMES_MAX_32BIT_SIZE
   * uid           FTIMES_MAX_32BIT_SIZE
   * gid           FTIMES_MAX_32BIT_SIZE
   * rdev          FTIMES_MAX_32BIT_SIZE
   * atime         FTIMES_TIME_FORMAT_SIZE
   * mtime         FTIMES_TIME_FORMAT_SIZE
   * ctime         FTIMES_TIME_FORMAT_SIZE
   * size          FTIMES_MAX_64BIT_SIZE
   * md5           FTIMES_MAX_MD5_LENGTH
   * sha1          FTIMES_MAX_SHA1_LENGTH
   * sha256        FTIMES_MAX_SHA256_LENGTH
#ifdef USE_XMAGIC
   * magic         XMAGIC_DESCRIPTION_BUFSIZE
#endif
   * |'s           13
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[4 +
                (3 * FTIMES_MAX_PATH) +
                (7 * FTIMES_MAX_32BIT_SIZE) +
                (3 * FTIMES_TIME_FORMAT_SIZE) +
                (1 * FTIMES_MAX_64BIT_SIZE) +
                (1 * FTIMES_MAX_MD5_LENGTH) +
                (1 * FTIMES_MAX_SHA1_LENGTH) +
                (1 * FTIMES_MAX_SHA256_LENGTH) +
#ifdef USE_XMAGIC
                (1 * XMAGIC_DESCRIPTION_BUFSIZE) +
#endif
                17
                ];
#endif

#ifdef WIN32
  /*-
   *********************************************************************
   *
   * prefix        4
   * name          1 (for quote) + (3 * FTIMES_MAX_PATH) + 1 (for quote)
   * volume        FTIMES_MAX_32BIT_SIZE
   * findex        FTIMES_MAX_64BIT_SIZE
   * attributes    FTIMES_MAX_32BIT_SIZE
   * atime|ams     FTIMES_TIME_FORMAT_SIZE
   * mtime|mms     FTIMES_TIME_FORMAT_SIZE
   * ctime|cms     FTIMES_TIME_FORMAT_SIZE
   * chtime|chms   FTIMES_TIME_FORMAT_SIZE
   * size          FTIMES_MAX_64BIT_SIZE
   * altstreams    FTIMES_MAX_32BIT_SIZE
   * md5           FTIMES_MAX_MD5_LENGTH
   * sha1          FTIMES_MAX_SHA1_LENGTH
   * sha256        FTIMES_MAX_SHA256_LENGTH
#ifdef USE_XMAGIC
   * magic         XMAGIC_DESCRIPTION_BUFSIZE
#endif
   * osid          FTIMES_MAX_SID_SIZE
   * gsid          FTIMES_MAX_SID_SIZE
   * dacl          FTIMES_MAX_ACL_SIZE
   * |'s           14 (not counting those embedded in time)
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[4 +
                (3 * FTIMES_MAX_PATH) +
                (3 * FTIMES_MAX_32BIT_SIZE) +
                (4 * FTIMES_TIME_FORMAT_SIZE) +
                (2 * FTIMES_MAX_64BIT_SIZE) +
                (1 * FTIMES_MAX_MD5_LENGTH) +
                (1 * FTIMES_MAX_SHA1_LENGTH) +
                (1 * FTIMES_MAX_SHA256_LENGTH) +
#ifdef USE_XMAGIC
                (1 * XMAGIC_DESCRIPTION_BUFSIZE) +
#endif
                (2 * FTIMES_MAX_SID_SIZE) +
                (1 * FTIMES_MAX_ACL_SIZE) +
                18
                ];
#endif

  /*-
   *********************************************************************
   *
   * Conditionally add a record prefix.
   *
   *********************************************************************
   */
  if (psProperties->acMapRecordPrefix[0])
  {
    iIndex = sprintf(acOutput, "%s", psProperties->acMapRecordPrefix);
  }

  /*-
   *********************************************************************
   *
   * Develop the output. Warn the user if a record has null fields.
   *
   *********************************************************************
   */
  iError = psProperties->piDevelopMapOutput(psProperties, &acOutput[iIndex], &iWriteCount, psFTFileData, acLocalError);
  if (iError == ER_NullFields)
  {
    giIncompleteRecords++;
    snprintf(pcError, MESSAGE_SIZE, "%s: NeuteredPath = [%s], NullFields = [%s]", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
#ifdef USE_PCRE
  else if (iError == ER_Filtered)
  {
    return ER_OK;
  }
#endif
  giRecords++;
  iWriteCount += iIndex;

  /*-
   *********************************************************************
   *
   * Write the output data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, acOutput, iWriteCount, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output file hash.
   *
   *********************************************************************
   */
  MD5Cycle(&psProperties->sOutFileHashContext, (unsigned char *) acOutput, iWriteCount);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapWriteHeader
 *
 ***********************************************************************
 */
int
MapWriteHeader(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "MapWriteHeader()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acHeaderData[FTIMES_MAX_LINE] = "";
  int                 i = 0;
  int                 iError = 0;
  int                 iIndex = 0;
  int                 iMaskTableLength = MaskGetTableLength(MASK_RUNMODE_TYPE_MAP);
  MASK_B2S_TABLE     *psMaskTable = MaskGetTableReference(MASK_RUNMODE_TYPE_MAP);
  unsigned long       ul = 0;

  /*-
   *********************************************************************
   *
   * Build the output's header. Conditionally add a header prefix.
   *
   *********************************************************************
   */
  if (psProperties->bCompress)
  {
    iIndex = sprintf(acHeaderData, "%sz_name", (psProperties->acMapRecordPrefix[0]) ? psProperties->acMapRecordPrefix : "");
    for (i = 0; i < iMaskTableLength; i++)
    {
      ul = (1 << i);
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, ul))
      {
#ifdef WIN32
        switch (ul)
        {
        case MAP_ATIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_atime|z_ams");
          break;
        case MAP_MTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_mtime|z_mms");
          break;
        case MAP_CTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_ctime|z_cms");
          break;
        case MAP_CHTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_chtime|z_chms");
          break;
        default:
          iIndex += sprintf(&acHeaderData[iIndex], "|z_%s", (char *) psMaskTable[i].acName);
          break;
        }
#else
        iIndex += sprintf(&acHeaderData[iIndex], "|z_%s", (char *) psMaskTable[i].acName);
#endif
      }
    }
  }
  else
  {
    iIndex = sprintf(acHeaderData, "%sname", (psProperties->acMapRecordPrefix[0]) ? psProperties->acMapRecordPrefix : "");
    for (i = 0; i < iMaskTableLength; i++)
    {
      ul = (1 << i);
      if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, ul))
      {
#ifdef WIN32
        switch (ul)
        {
        case MAP_ATIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|atime|ams");
          break;
        case MAP_MTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|mtime|mms");
          break;
        case MAP_CTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|ctime|cms");
          break;
        case MAP_CHTIME:
          iIndex += sprintf(&acHeaderData[iIndex], "|chtime|chms");
          break;
        default:
          iIndex += sprintf(&acHeaderData[iIndex], "|%s", (char *) psMaskTable[i].acName);
          break;
        }
#else
        iIndex += sprintf(&acHeaderData[iIndex], "|%s", (char *) psMaskTable[i].acName);
#endif
      }
    }
  }
  iIndex += sprintf(&acHeaderData[iIndex], "%s", psProperties->acNewLine);

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(psProperties->pFileOut, acHeaderData, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Cycle(&psProperties->sOutFileHashContext, (unsigned char *) acHeaderData, iIndex);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapGetAttributes
 *
 ***********************************************************************
 */
unsigned long
MapGetAttributes(FTIMES_FILE_DATA *psFTFileData)
{
  const char          acRoutine[] = "MapGetAttributes()";
  char                acLocalError[MESSAGE_SIZE] = "";
#ifdef WINNT
  BOOL                bResult;
  BY_HANDLE_FILE_INFORMATION sFileInfo;
  char               *pcMessage;
  DWORD               dwLastError;
  DWORD               dwSize;
  DWORD               dwStatus;
  FILE_BASIC_INFORMATION sFileBasicInfo;
  HANDLE              hFile;
  IO_STATUS_BLOCK     sIOStatusBlock;
  int                 iStatus;
  WIN32_FILE_ATTRIBUTE_DATA sFileAttributeData;
#endif

  psFTFileData->ulAttributeMask = 0;
  psFTFileData->iFileExists = 1; /* Be optimistic. */

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Collect attributes. Use GetFileInformationByHandle() if the file
   * handle is valid. Otherwise, fall back to GetFileAttributesEx().
   * If this is NT and the file handle is valid, additionally invoke
   * the native call NTQueryInformationFile() to get ChangeTime.
   *
   *********************************************************************
   */
  hFile = MapGetFileHandleW(psFTFileData->pwcRawPath);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    if (GetFileInformationByHandle(hFile, &sFileInfo))
    {
      psFTFileData->ulAttributeMask |= MAP_VOLUME | MAP_FINDEX | MAP_ATTRIBUTES | MAP_ATIME | MAP_MTIME | MAP_CTIME | MAP_CHTIME | MAP_SIZE;
      psFTFileData->dwVolumeSerialNumber     = sFileInfo.dwVolumeSerialNumber;
      psFTFileData->dwFileIndexHigh          = sFileInfo.nFileIndexHigh;
      psFTFileData->dwFileIndexLow           = sFileInfo.nFileIndexLow;
      psFTFileData->dwFileAttributes         = sFileInfo.dwFileAttributes;
      psFTFileData->sFTATime.dwLowDateTime   = sFileInfo.ftLastAccessTime.dwLowDateTime;
      psFTFileData->sFTATime.dwHighDateTime  = sFileInfo.ftLastAccessTime.dwHighDateTime;
      psFTFileData->sFTMTime.dwLowDateTime   = sFileInfo.ftLastWriteTime.dwLowDateTime;
      psFTFileData->sFTMTime.dwHighDateTime  = sFileInfo.ftLastWriteTime.dwHighDateTime;
      psFTFileData->sFTCTime.dwLowDateTime   = sFileInfo.ftCreationTime.dwLowDateTime;
      psFTFileData->sFTCTime.dwHighDateTime  = sFileInfo.ftCreationTime.dwHighDateTime;
      psFTFileData->sFTChTime.dwLowDateTime  = 0;
      psFTFileData->sFTChTime.dwHighDateTime = 0;
      psFTFileData->dwFileSizeHigh           = sFileInfo.nFileSizeHigh;
      psFTFileData->dwFileSizeLow            = sFileInfo.nFileSizeLow;
    }
    else
    {
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileInformationByHandle(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }

    memset(&sFileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
    dwStatus = NtdllNQIF(hFile, &sIOStatusBlock, &sFileBasicInfo, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
    if (dwStatus == 0)
    {
      psFTFileData->ulAttributeMask |= MAP_ATTRIBUTES | MAP_ATIME | MAP_MTIME | MAP_CTIME | MAP_CHTIME;
      psFTFileData->dwFileAttributes         = sFileBasicInfo.FileAttributes;
      psFTFileData->sFTATime.dwLowDateTime   = sFileBasicInfo.LastAccessTime.LowPart;
      psFTFileData->sFTATime.dwHighDateTime  = sFileBasicInfo.LastAccessTime.HighPart;
      psFTFileData->sFTMTime.dwLowDateTime   = sFileBasicInfo.LastWriteTime.LowPart;
      psFTFileData->sFTMTime.dwHighDateTime  = sFileBasicInfo.LastWriteTime.HighPart;
      psFTFileData->sFTCTime.dwLowDateTime   = sFileBasicInfo.CreationTime.LowPart;
      psFTFileData->sFTCTime.dwHighDateTime  = sFileBasicInfo.CreationTime.HighPart;
      psFTFileData->sFTChTime.dwLowDateTime  = sFileBasicInfo.ChangeTime.LowPart;
      psFTFileData->sFTChTime.dwHighDateTime = sFileBasicInfo.ChangeTime.HighPart;
    }
    else
    {
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: NtdllNQIF(): %08x", acRoutine, psFTFileData->pcNeuteredPath, dwStatus);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }

    /*-
     *********************************************************************
     *
     * Harvest security information (owner/group SIDs and DACL).
     *
     *********************************************************************
     */
    dwStatus = GetSecurityInfo(
      hFile,
      SE_FILE_OBJECT,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
      (PSID) &psFTFileData->psSidOwner,
      (PSID) &psFTFileData->psSidGroup,
      NULL, /* This pointer is not required to obtain DACL information. */
      NULL,
#if (WINVER <= 0x500)
      &psFTFileData->psSd
#else
      (PSECURITY_DESCRIPTOR) &psFTFileData->psSd
#endif
      );
    if (dwStatus != ERROR_SUCCESS)
    {
      dwLastError = dwStatus; /* According to MSDN, GetSecurityInfo() returns a nonzero error code defined in WinError.h when it fails. */
      ErrorFormatWinxError(dwLastError, &pcMessage);
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetSecurityInfo(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }
    if (!IsValidSecurityDescriptor(psFTFileData->psSd))
    {
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: IsValidSecurityDescriptor(): One or more components of the security descriptor are not valid.", acRoutine, psFTFileData->pcNeuteredPath);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }
    else
    {
      psFTFileData->ulAttributeMask |= MAP_OWNER | MAP_GROUP | MAP_DACL;
    }

    /*-
     *********************************************************************
     *
     * Determine the number of alternate streams. This check applies to
     * files and directories, and it is NTFS specific. A valid handle
     * is required to perform the check.
     *
     *********************************************************************
     */
    if (psFTFileData->iFSType == FSTYPE_NTFS)
    {
      iStatus = MapCountNamedStreams(hFile, &psFTFileData->iStreamCount, &psFTFileData->pucStreamInfo, acLocalError);
      if (iStatus == ER_OK)
      {
        psFTFileData->ulAttributeMask |= MAP_ALTSTREAMS;
      }
      else
      {
        snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: Stream Count Failed: %s", acRoutine, psFTFileData->pcNeuteredPath, acLocalError);
        ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
      }
    }
    CloseHandle(hFile);
  }
  else
  {
    dwLastError = GetLastError();
    ErrorFormatWinxError(dwLastError, &pcMessage);
    snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: MapGetFileHandleW(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
    ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    if (dwLastError == ERROR_FILE_NOT_FOUND)
    {
      psFTFileData->iFileExists = 0;
      return psFTFileData->ulAttributeMask;
    }

    bResult = GetFileAttributesExW(psFTFileData->pwcRawPath, GetFileExInfoStandard, &sFileAttributeData);
    if (bResult)
    {
      psFTFileData->ulAttributeMask |= MAP_ATTRIBUTES | MAP_ATIME | MAP_MTIME | MAP_CTIME | MAP_SIZE;
      psFTFileData->dwFileAttributes        = sFileAttributeData.dwFileAttributes;
      psFTFileData->sFTATime.dwLowDateTime  = sFileAttributeData.ftLastAccessTime.dwLowDateTime;
      psFTFileData->sFTATime.dwHighDateTime = sFileAttributeData.ftLastAccessTime.dwHighDateTime;
      psFTFileData->sFTMTime.dwLowDateTime  = sFileAttributeData.ftLastWriteTime.dwLowDateTime;
      psFTFileData->sFTMTime.dwHighDateTime = sFileAttributeData.ftLastWriteTime.dwHighDateTime;
      psFTFileData->sFTCTime.dwLowDateTime  = sFileAttributeData.ftCreationTime.dwLowDateTime;
      psFTFileData->sFTCTime.dwHighDateTime = sFileAttributeData.ftCreationTime.dwHighDateTime;
      psFTFileData->dwFileSizeHigh          = sFileAttributeData.nFileSizeHigh;
      psFTFileData->dwFileSizeLow           = sFileAttributeData.nFileSizeLow;
    }
    else
    {
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileAttributesExW(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }

    /*-
     *********************************************************************
     *
     * Harvest security information (owner/group SIDs and DACL).
     *
     *********************************************************************
     */
    bResult = GetFileSecurityW(
      psFTFileData->pwcRawPath,
      OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
      NULL,
      0,
      &dwSize
      );
    if (!bResult && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
      psFTFileData->psSd = (SECURITY_DESCRIPTOR *) LocalAlloc(LPTR, dwSize);
      if (psFTFileData->psSd)
      {
        bResult = GetFileSecurityW(
          psFTFileData->pwcRawPath,
          OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
          psFTFileData->psSd,
          dwSize,
          &dwSize
          );
        if (bResult)
        {
          if (!IsValidSecurityDescriptor(psFTFileData->psSd))
          {
            snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: IsValidSecurityDescriptor(): One or more components of the security descriptor are not valid.", acRoutine, psFTFileData->pcNeuteredPath);
            ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
          }
          else
          {
            BOOL bDefaulted;
            GetSecurityDescriptorOwner(psFTFileData->psSd, (PSID) &psFTFileData->psSidOwner, &bDefaulted);
            GetSecurityDescriptorGroup(psFTFileData->psSd, (PSID) &psFTFileData->psSidGroup, &bDefaulted);
            psFTFileData->ulAttributeMask |= MAP_OWNER | MAP_GROUP | MAP_DACL;
          }
        }
        else
        {
          ErrorFormatWinxError(GetLastError(), &pcMessage);
          snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileSecurityW(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
          ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
        }
      }
      else
      {
        ErrorFormatWinxError(GetLastError(), &pcMessage);
        snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: LocalAlloc(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
        ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
      }
    }
    else
    {
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: GetFileSecurityW(): %s", acRoutine, psFTFileData->pcNeuteredPath, pcMessage);
      ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    }
  }
#else
  /*-
   *********************************************************************
   *
   * Collect attributes. Use lstat() so links aren't followed.
   *
   *********************************************************************
   */
  if (lstat(psFTFileData->pcRawPath, &psFTFileData->sStatEntry) != ER)
  {
    psFTFileData->ulAttributeMask |= MAP_LSTAT_MASK;
  }
  else
  {
    int iLastErrno = errno;
    snprintf(acLocalError, MESSAGE_SIZE, "%s: NeuteredPath = [%s]: lstat(): %s", acRoutine, psFTFileData->pcNeuteredPath, strerror(errno));
    ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
    if (iLastErrno == ENOENT || iLastErrno == ENOTDIR)
    {
      psFTFileData->iFileExists = 0;
    }
  }
#endif

  return psFTFileData->ulAttributeMask;
}


/*-
 ***********************************************************************
 *
 * MapFreeFTFileData
 *
 ***********************************************************************
 */
void
MapFreeFTFileData(FTIMES_FILE_DATA *psFTFileData)
{
  if (psFTFileData != NULL)
  {
    if (psFTFileData->pcNeuteredPath != NULL)
    {
      free(psFTFileData->pcNeuteredPath);
    }
    if (psFTFileData->pcRawPath != NULL)
    {
      free(psFTFileData->pcRawPath);
    }
#ifdef WINNT
    if (psFTFileData->pwcRawPath != NULL)
    {
      free(psFTFileData->pwcRawPath);
    }
    if (psFTFileData->psSd != NULL)
    {
      LocalFree(psFTFileData->psSd);
    }
    if (psFTFileData->pucStreamInfo != NULL)
    {
      free(psFTFileData->pucStreamInfo);
    }
#endif
    free(psFTFileData);
  }
}


/*-
 ***********************************************************************
 *
 * MapFreePythonArguments
 *
 ***********************************************************************
 */
void
MapFreePythonArguments(size_t szArgumentCount, wchar_t **ppwcArgumentVector)
{
  size_t              szArgument = 0;

  if (ppwcArgumentVector != NULL)
  {
    for (szArgument = 0; szArgument < szArgumentCount; szArgument++)
    {
      if (ppwcArgumentVector[szArgument] != NULL)
      {
        free(ppwcArgumentVector[szArgument]);
      }
    }
    free(ppwcArgumentVector);
  }

  return;
}


#ifndef WINNT
/*-
 ***********************************************************************
 *
 * MapNewFTFileData
 *
 ***********************************************************************
 */
FTIMES_FILE_DATA *
MapNewFTFileData(FTIMES_FILE_DATA *psParentFTFileData, char *pcName, char *pcError)
{
  const char          acRoutine[] = "MapNewFTFileData()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acSeparator[2] = "";
  int                 iFSType = FSTYPE_UNSUPPORTED;
  FTIMES_FILE_DATA   *psFTFileData = NULL;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the file data structure.
   *
   *********************************************************************
   */
  psFTFileData = (FTIMES_FILE_DATA *) calloc(sizeof(FTIMES_FILE_DATA), 1);
  if (psFTFileData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Initialize variables that require a nonzero value. Also note that
   * subsequent logic relies on the assertion that each hash value has
   * been initialized to all zeros.
   *
   *********************************************************************
   */
  psFTFileData->psParent = psParentFTFileData;

  /*-
   *********************************************************************
   *
   * Create the new path. Impose a limit to keep things under control.
   *
   *********************************************************************
   */
  if (psParentFTFileData)
  {
    psFTFileData->iRawPathLength = psParentFTFileData->iRawPathLength + strlen(pcName);
    if (psParentFTFileData->pcRawPath[psParentFTFileData->iRawPathLength - 1] != FTIMES_SLASHCHAR)
    {
      acSeparator[0] = FTIMES_SLASHCHAR;
      acSeparator[1] = 0;
      psFTFileData->iRawPathLength++;
    }
  }
  else
  {
    psFTFileData->iRawPathLength = strlen(pcName);
  }
  if (psFTFileData->iRawPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length (%d) exceeds %d bytes.", acRoutine, psFTFileData->iRawPathLength, FTIMES_MAX_PATH - 1);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->pcRawPath = malloc(psFTFileData->iRawPathLength + 1);
  if (psFTFileData->pcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  if (psParentFTFileData)
  {
    snprintf(psFTFileData->pcRawPath, FTIMES_MAX_PATH, "%s%s%s", psParentFTFileData->pcRawPath, acSeparator, pcName);
  }
  else
  {
    snprintf(psFTFileData->pcRawPath, FTIMES_MAX_PATH, "%s", pcName);
  }

  /*-
   *********************************************************************
   *
   * Neuter the new path.
   *
   *********************************************************************
   */
  psFTFileData->pcNeuteredPath = SupportNeuterString(psFTFileData->pcRawPath, psFTFileData->iRawPathLength, acLocalError);
  if (psFTFileData->pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Conditionally determine file system type. This value is required
   * by MapGetAttributes() under WINX, so it is set here.
   *
   *********************************************************************
   */
  if (psParentFTFileData)
  {
    psFTFileData->iFSType = psParentFTFileData->iFSType; /* Inherit file system type. */
  }
  else
  {
    iFSType = GetFileSystemType(psFTFileData->pcRawPath, acLocalError);
    if (iFSType == ER || iFSType == FSTYPE_UNSUPPORTED)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      MapFreeFTFileData(psFTFileData);
      return NULL;
    }
    psFTFileData->iFSType = iFSType;
  }

  return psFTFileData;
}
#endif


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapNewFTFileData
 *
 ***********************************************************************
 */
FTIMES_FILE_DATA *
MapNewFTFileDataW(FTIMES_FILE_DATA *psParentFTFileData, wchar_t *pwcName, char *pcError)
{
  const char          acRoutine[] = "MapNewFTFileDataW()";
  char                acLocalError[MESSAGE_SIZE] = "";
  wchar_t             awcSeparator[2] = L"";
  int                 iFSType = FSTYPE_UNSUPPORTED;
  FTIMES_FILE_DATA   *psFTFileData = NULL;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the file data structure.
   *
   *********************************************************************
   */
  psFTFileData = (FTIMES_FILE_DATA *) calloc(sizeof(FTIMES_FILE_DATA), 1);
  if (psFTFileData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Initialize variables that require a nonzero value. Also note that
   * subsequent logic relies on the assertion that each hash value has
   * been initialized to all zeros.
   *
   *********************************************************************
   */
  psFTFileData->dwVolumeSerialNumber = -1;
  psFTFileData->dwFileIndexHigh = -1;
  psFTFileData->dwFileIndexLow = -1;
  psFTFileData->iStreamCount = FTIMES_INVALID_STREAM_COUNT; /* The Develop{Compressed,Normal}Output routines check for this value. */
  psFTFileData->psParent = psParentFTFileData;

  /*-
   *********************************************************************
   *
   * Create the new path. Impose a limit to keep things under control.
   *
   *********************************************************************
   */
  if (psParentFTFileData)
  {
    psFTFileData->iWideRawPathLength = psParentFTFileData->iWideRawPathLength + wcslen(pwcName);
    if (psParentFTFileData->pwcRawPath[psParentFTFileData->iWideRawPathLength - 1] != FTIMES_SLASHCHAR_W)
    {
      awcSeparator[0] = FTIMES_SLASHCHAR_W;
      awcSeparator[1] = 0;
      psFTFileData->iWideRawPathLength++;
    }
  }
  else
  {
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//    psFTFileData->iWideRawPathLength = wcslen(pwcName);
    psFTFileData->iWideRawPathLength = FTIMES_EXTENDED_PREFIX_SIZE + wcslen(pwcName);
//END (\\?\)
  }
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  if (psFTFileData->iWideRawPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  if (psFTFileData->iWideRawPathLength - FTIMES_EXTENDED_PREFIX_SIZE > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
//END (\\?\)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length (%d) exceeds %d bytes.", acRoutine, psFTFileData->iWideRawPathLength, FTIMES_MAX_PATH - 1);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->pwcRawPath = malloc((psFTFileData->iWideRawPathLength + 1) * sizeof(wchar_t));
  if (psFTFileData->pwcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  if (psParentFTFileData)
  {
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//    _snwprintf(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, L"%s%s%s", psParentFTFileData->pwcRawPath, awcSeparator, pwcName);
    _snwprintf(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, L"%s%s%s", psParentFTFileData->pwcRawPath, awcSeparator, pwcName); /* The extended path prefix should already be included. */
//END (\\?\)
  }
  else
  {
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//    _snwprintf(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, L"%s", pwcName);
    _snwprintf(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, L"\\\\?\\%s", pwcName); /* Include the extended path prefix since there is no parent. */
//END (\\?\)
  }

  /*-
   *********************************************************************
   *
   * Convert the new path to UTF-8.
   *
   *********************************************************************
   */
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//  psFTFileData->pcRawPath = MapWideToUtf8(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, acLocalError);
  psFTFileData->pcRawPath = MapWideToUtf8(&psFTFileData->pwcRawPath[FTIMES_EXTENDED_PREFIX_SIZE], psFTFileData->iWideRawPathLength - FTIMES_EXTENDED_PREFIX_SIZE + 1, acLocalError);
//END (\\?\)
  if (psFTFileData->pcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->iUtf8RawPathLength = strlen(psFTFileData->pcRawPath);

  /*-
   *********************************************************************
   *
   * Neuter the new path.
   *
   *********************************************************************
   */
  psFTFileData->pcNeuteredPath = SupportNeuterString(psFTFileData->pcRawPath, psFTFileData->iUtf8RawPathLength, acLocalError);
  if (psFTFileData->pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Conditionally determine file system type. This value is required
   * by MapGetAttributes() under WINX, so it is set here.
   *
   *********************************************************************
   */
  if (psParentFTFileData)
  {
    psFTFileData->iFSType = psParentFTFileData->iFSType; /* Inherit file system type. */
  }
  else
  {
    iFSType = GetFileSystemType(psFTFileData->pcRawPath, acLocalError);
    if (iFSType == ER || iFSType == FSTYPE_UNSUPPORTED)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      MapFreeFTFileData(psFTFileData);
      return NULL;
    }
    psFTFileData->iFSType = iFSType;
  }

  return psFTFileData;
}


/*-
 ***********************************************************************
 *
 * MapUtf8ToWide
 *
 ***********************************************************************
 */
wchar_t *
MapUtf8ToWide(char *pcString, int iUtf8Size, char *pcError)
{
  const char          acRoutine[] = "MapUtf8ToWide()";
  char               *pcMessage = NULL;
  int                 iWideSize = 0;
  wchar_t            *pwcString = NULL;

  iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, NULL, 0); /* The byte count returned includes the NULL terminator. */
  if (iWideSize)
  {
    pwcString = malloc(iWideSize * sizeof(wchar_t));
    if (pwcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
    iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, pwcString, iWideSize);
    if (!iWideSize)
    {
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, pcMessage);
      free(pwcString);
      return NULL;
    }
  }
  else
  {
    ErrorFormatWinxError(GetLastError(), &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: MultiByteToWideChar(): %s", acRoutine, pcMessage);
    return NULL;
  }

  return pwcString;
}


/*-
 ***********************************************************************
 *
 * MapWideToUtf8
 *
 ***********************************************************************
 */
char *
MapWideToUtf8(wchar_t *pwcString, int iWideSize, char *pcError)
{
  const char          acRoutine[] = "MapWideToUtf8()";
  char               *pcMessage = NULL;
  char               *pcString = NULL;
  int                 iUtf8Size = 0;

  iUtf8Size = WideCharToMultiByte(CP_UTF8, 0, pwcString, iWideSize, NULL, 0, NULL, NULL); /* The byte count returned includes the NULL terminator. */
  if (iUtf8Size)
  {
    pcString = malloc(iUtf8Size);
    if (pcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
    iUtf8Size = WideCharToMultiByte(CP_UTF8, 0, pwcString, iWideSize, pcString, iUtf8Size, NULL, NULL);
    if (!iUtf8Size)
    {
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: WideCharToMultiByte(): %s", acRoutine, pcMessage);
      free(pcString);
      return NULL;
    }
  }
  else
  {
    ErrorFormatWinxError(GetLastError(), &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: WideCharToMultiByte(): %s", acRoutine, pcMessage);
    return NULL;
  }

  return pcString;
}
#endif
