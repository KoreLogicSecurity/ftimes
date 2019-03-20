/*-
 ***********************************************************************
 *
 * $Id: hook.h,v 1.4 2013/02/14 16:55:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2011-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _HOOK_H_INCLUDED
#define _HOOK_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */

/* Empty */

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _HOOK_LIST
{
  unsigned int        uiId;
  char               *pcName;
  char               *pcExpression;
  char               *pcInterpreter;
  char               *pcProgram;
  KLEL_CONTEXT       *psContext;
  struct _HOOK_LIST  *psNext;
} HOOK_LIST;

typedef struct _HOOK_TYPE_SPEC
{
  const char         *pcName;
  KLEL_EXPR_TYPE      iType;
} HOOK_TYPE_SPEC;

typedef struct _HOOK_FUNC_DESC
{
  const char         *pcName;
  KLEL_EXPR_TYPE      aiArguments[KLEL_MAX_FUNC_ARGS];
} HOOK_FUNC_DESC;

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */

/* Empty */

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                   HookAddHook(char *pcExpression, HOOK_LIST **psHead, char *pcError);
void                  HookFreeHook(HOOK_LIST *psHook);
KLEL_EXPR_TYPE        HookGetTypeOfVar(const char *pcName, void *pvContext);
KLEL_VALUE           *HookGetValueOfVar(const char *pcName, void *pvContext);
//HOOK_LIST            *HookMatchHook(HOOK_LIST *psHookList, FTIMES_FILE_DATA *psFTFileData); /* This is declared in ftimes.h. */
HOOK_LIST            *HookNewHook(char *pcExpression, char *pcError);

#endif /* !_HOOK_H_INCLUDED */
