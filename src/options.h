/*-
 ***********************************************************************
 *
 * $Id: options.h,v 1.5 2013/02/14 16:55:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2006-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _OPTIONS_H_INCLUDED
#define _OPTIONS_H_INCLUDED

#ifdef UNIX
  #ifndef TCHAR
    #define TCHAR char
  #endif
  #ifndef _tcscmp
    #define _tcscmp strcmp
  #endif
  #ifndef _sntprintf
    #define _sntprintf snprintf
  #endif
  #ifndef _T
    #define _T(x) x
  #endif
#endif

/*-
 ***********************************************************************
 *
 * Defines.
 *
 ***********************************************************************
 */
#define OPTIONS_NICKNAME_SIZE (3 * sizeof(TCHAR))
#define OPTIONS_FULLNAME_SIZE (256 * sizeof(TCHAR))

#define OPTIONS_ER   -1
#define OPTIONS_OK    0
#define OPTIONS_USAGE 1

#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE (1024 * sizeof(TCHAR))
#endif

/*-
 ***********************************************************************
 *
 * Typedefs.
 *
 ***********************************************************************
 */
typedef enum _OPTIONS_ARGUMENT_TYPES
{
  OPTIONS_ARGUMENT_TYPE_PROGRAM = 1,
  OPTIONS_ARGUMENT_TYPE_MODE,
  OPTIONS_ARGUMENT_TYPE_OPTION,
  OPTIONS_ARGUMENT_TYPE_OPTION_ARGUMENT,
  OPTIONS_ARGUMENT_TYPE_END_OF_OPTIONS,
  OPTIONS_ARGUMENT_TYPE_OPERAND,
} OPTIONS_ARGUMENT_TYPES;

typedef struct _OPTIONS_TABLE
{
  int                 iId;
  TCHAR               atcNickName[OPTIONS_NICKNAME_SIZE];
  TCHAR               atcFullName[OPTIONS_FULLNAME_SIZE];
  int                 iAllowRepeats;
  int                 iFound;
  int                 iNArguments; /* 0, 1, or possibly more */
  int                 iRequired;
  int               (*piHandler)();
} OPTIONS_TABLE;

typedef struct _OPTIONS_CONTEXT
{
  TCHAR             **pptcArgumentVector;
  int                 iArgumentCount;
  int                 iArgumentIndex;
  int                 iNOptions;
  int                 iOperandIndex;
  int                *piArgumentTypes;
  OPTIONS_TABLE      *psOptions;
} OPTIONS_CONTEXT;

/*-
 ***********************************************************************
 *
 * Function Prototypes.
 *
 ***********************************************************************
 */
void                  OptionsFreeOptionsContext(OPTIONS_CONTEXT *psOptionsContext);
int                   OptionsGetArgumentIndex(OPTIONS_CONTEXT *psOptionsContext);
int                   OptionsGetArgumentsLeft(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetCommand(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetCurrentArgument(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetCurrentOperand(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetFirstArgument(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetFirstOperand(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetNextArgument(OPTIONS_CONTEXT *psOptionsContext);
TCHAR                *OptionsGetNextOperand(OPTIONS_CONTEXT *psOptionsContext);
int                   OptionsGetOperandCount(OPTIONS_CONTEXT *psOptionsContext);
int                   OptionsHaveRequiredOptions(OPTIONS_CONTEXT *psOptionsContext);
int                   OptionsHaveSpecifiedOption(OPTIONS_CONTEXT *psOptionsContext, int iId);
OPTIONS_CONTEXT      *OptionsNewOptionsContext(int iArgumentCount, TCHAR *pptcArgumentVector[], TCHAR *ptcError);
int                   OptionsProcessOptions(OPTIONS_CONTEXT *psOptionsContext, void *pvProperties, TCHAR *ptcError);
void                  OptionsSetArgumentType(OPTIONS_CONTEXT *psOptionsContext, int iType);
void                  OptionsSetOptions(OPTIONS_CONTEXT *psOptionsContext, OPTIONS_TABLE *psOptions, int iNOptions);

#endif /* !_OPTIONS_H_INCLUDED */
