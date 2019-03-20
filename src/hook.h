/*-
 ***********************************************************************
 *
 * $Id: hook.h,v 1.2 2012/04/14 18:15:02 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2011-2012 The FTimes Project, All Rights Reserved.
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
  KLEL_NODE          *psExpression;
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
KLEL_EXPR_TYPE       *HookGetFuncDesc(const char *pcName, KLEL_CONTEXT *psContext);
KLEL_EXPR_TYPE        HookGetTypeOfVar(const char *pcName, KLEL_CONTEXT *psContext);
KLEL_VALUE           *HookGetValueOfVar(const char *pcName, KLEL_CONTEXT *psContext);
void                  HookInitialize(void);
//HOOK_LIST            *HookMatchHook(HOOK_LIST *psHookList, FTIMES_FILE_DATA *psFTFileData); /* This is declared in ftimes.h. */
HOOK_LIST            *HookNewHook(char *pcExpression, char *pcError);

#endif /* !_HOOK_H_INCLUDED */
