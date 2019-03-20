/*-
 ***********************************************************************
 *
 * $Id: options.c,v 1.5 2012/05/04 20:16:47 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2006-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * OptionsFreeOptionsContext
 *
 ***********************************************************************
 */
void
OptionsFreeOptionsContext(OPTIONS_CONTEXT *psOptionsContext)
{
  if (psOptionsContext != NULL)
  {
    if (psOptionsContext->piArgumentTypes != NULL)
    {
      free(psOptionsContext->piArgumentTypes);
    }
    free(psOptionsContext);
  }
}


/*-
 ***********************************************************************
 *
 * OptionsGetArgumentIndex
 *
 ***********************************************************************
 */
int
OptionsGetArgumentIndex(OPTIONS_CONTEXT *psOptionsContext)
{
  return psOptionsContext->iArgumentIndex;
}


/*-
 ***********************************************************************
 *
 * OptionsGetArgumentsLeft
 *
 ***********************************************************************
 */
int
OptionsGetArgumentsLeft(OPTIONS_CONTEXT *psOptionsContext)
{
  return (psOptionsContext->iArgumentCount - (psOptionsContext->iArgumentIndex + 1));
}


/*-
 ***********************************************************************
 *
 * OptionsGetCommand
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetCommand(OPTIONS_CONTEXT *psOptionsContext)
{
  return psOptionsContext->pptcArgumentVector[0];
}


/*-
 ***********************************************************************
 *
 * OptionsGetCurrentArgument
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetCurrentArgument(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcArgument = NULL;

  if (psOptionsContext->iArgumentIndex < psOptionsContext->iArgumentCount)
  {
    ptcArgument = psOptionsContext->pptcArgumentVector[psOptionsContext->iArgumentIndex];
  }

  return ptcArgument;
}


/*-
 ***********************************************************************
 *
 * OptionsGetCurrentOperand
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetCurrentOperand(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcOperand = NULL;

  if (psOptionsContext->iOperandIndex < psOptionsContext->iArgumentCount)
  {
    ptcOperand = psOptionsContext->pptcArgumentVector[psOptionsContext->iOperandIndex];
  }

  return ptcOperand;
}


/*-
 ***********************************************************************
 *
 * OptionsGetFirstArgument
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetFirstArgument(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcArgument = NULL;

  psOptionsContext->iArgumentIndex = 1;
  if (psOptionsContext->iArgumentIndex < psOptionsContext->iArgumentCount)
  {
    ptcArgument = psOptionsContext->pptcArgumentVector[psOptionsContext->iArgumentIndex];
  }

  return ptcArgument;
}


/*-
 ***********************************************************************
 *
 * OptionsGetFirstOperand
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetFirstOperand(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcOperand = NULL;

  for (psOptionsContext->iOperandIndex = 0; psOptionsContext->iOperandIndex < psOptionsContext->iArgumentCount; psOptionsContext->iOperandIndex++)
  {
    if (psOptionsContext->piArgumentTypes[psOptionsContext->iOperandIndex] == OPTIONS_ARGUMENT_TYPE_OPERAND)
    {
      ptcOperand = psOptionsContext->pptcArgumentVector[psOptionsContext->iOperandIndex];
      break;
    }
  }

  return ptcOperand;
}


/*-
 ***********************************************************************
 *
 * OptionsGetNextArgument
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetNextArgument(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcArgument = NULL;

  if ((psOptionsContext->iArgumentIndex + 1) < psOptionsContext->iArgumentCount)
  {
    ptcArgument = psOptionsContext->pptcArgumentVector[++psOptionsContext->iArgumentIndex];
  }

  return ptcArgument;
}


/*-
 ***********************************************************************
 *
 * OptionsGetNextOperand
 *
 ***********************************************************************
 */
TCHAR *
OptionsGetNextOperand(OPTIONS_CONTEXT *psOptionsContext)
{
  TCHAR              *ptcOperand = NULL;

  while ((psOptionsContext->iOperandIndex + 1) < psOptionsContext->iArgumentCount)
  {
    if (psOptionsContext->piArgumentTypes[++psOptionsContext->iOperandIndex] == OPTIONS_ARGUMENT_TYPE_OPERAND)
    {
      ptcOperand = psOptionsContext->pptcArgumentVector[psOptionsContext->iOperandIndex];
      break;
    }
  }

  return ptcOperand;
}


/*-
 ***********************************************************************
 *
 * OptionsGetOperandCount
 *
 ***********************************************************************
 */
int
OptionsGetOperandCount(OPTIONS_CONTEXT *psOptionsContext)
{
  int                 iOperandCount = 0;
  int                 iOperandIndex = 0;

  for (iOperandIndex = 0; iOperandIndex < psOptionsContext->iArgumentCount; iOperandIndex++)
  {
    if (psOptionsContext->piArgumentTypes[iOperandIndex] == OPTIONS_ARGUMENT_TYPE_OPERAND)
    {
      iOperandCount++;
    }
  }

  return iOperandCount;
}


/*-
 ***********************************************************************
 *
 * OptionsHaveRequiredOptions
 *
 ***********************************************************************
 */
int
OptionsHaveRequiredOptions(OPTIONS_CONTEXT *psOptionsContext)
{
  int                 iIndex = 0;

  for (iIndex = 0; iIndex < psOptionsContext->iNOptions; iIndex++)
  {
    if (psOptionsContext->psOptions[iIndex].iRequired == 1 && psOptionsContext->psOptions[iIndex].iFound == 0)
    {
      return 0;
    }
  }

  return 1;
}


/*-
 ***********************************************************************
 *
 * OptionsHaveSpecifiedOption
 *
 ***********************************************************************
 */
int
OptionsHaveSpecifiedOption(OPTIONS_CONTEXT *psOptionsContext, int iId)
{
  int                 iIndex = 0;

  for (iIndex = 0; iIndex < psOptionsContext->iNOptions; iIndex++)
  {
    if (iId == psOptionsContext->psOptions[iIndex].iId && psOptionsContext->psOptions[iIndex].iFound)
    {
      return 1;
    }
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * OptionsNewOptionsContext
 *
 ***********************************************************************
 */
OPTIONS_CONTEXT *
OptionsNewOptionsContext(int iArgumentCount, TCHAR *pptcArgumentVector[], TCHAR *ptcError)
{
  OPTIONS_CONTEXT    *psOptionsContext = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and clear memory for the structure.
   *
   *********************************************************************
   */
  psOptionsContext = (OPTIONS_CONTEXT *) calloc(sizeof(OPTIONS_CONTEXT), 1);
  if (psOptionsContext == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("OptionsNewOptionsContext(): calloc(): %s"), strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate and clear memory for the argument types.
   *
   *********************************************************************
   */
  psOptionsContext->piArgumentTypes = (int *) calloc(sizeof(int), iArgumentCount);
  if (psOptionsContext->piArgumentTypes == NULL)
  {
    _sntprintf(ptcError, MESSAGE_SIZE, _T("OptionsNewOptionsContext(): calloc(): %s"), strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize members.
   *
   *********************************************************************
   */
  psOptionsContext->pptcArgumentVector = pptcArgumentVector;
  psOptionsContext->iArgumentCount = iArgumentCount;
  psOptionsContext->piArgumentTypes[0] = OPTIONS_ARGUMENT_TYPE_PROGRAM;

  return psOptionsContext;
}


/*-
 ***********************************************************************
 *
 * OptionsProcessOptions
 *
 ***********************************************************************
 */
int
OptionsProcessOptions(OPTIONS_CONTEXT *psOptionsContext, void *pvProperties, TCHAR *ptcError)
{
  TCHAR               atcLocalError[MESSAGE_SIZE] = { 0 };
  TCHAR              *ptcArgument = NULL;
  int                 iError = 0;
  int                 iIndex = 0;
  int                 iStopIndex = psOptionsContext->iArgumentIndex;
  int                 iStopIndexSaved = 0;
  OPTIONS_TABLE      *psOptions = NULL;

  /*-
   *********************************************************************
   *
   * Define local variables to make the code easier to read.
   *
   *********************************************************************
   */
  psOptions = psOptionsContext->psOptions;

  /*-
   *********************************************************************
   *
   * Walk the argument list.
   *
   *********************************************************************
   */
  while ((ptcArgument = OptionsGetNextArgument(psOptionsContext)) != NULL)
  {
    /*-
     *******************************************************************
     *
     * Check for the end of options token ("--"). If it is found, save
     * the current argument index, classify the remaining arguments as
     * operands, and break.
     *
     *******************************************************************
     */
    if (_tcscmp(ptcArgument, _T("--")) == 0)
    {
      OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_END_OF_OPTIONS);
      if (!iStopIndexSaved)
      {
        iStopIndex = psOptionsContext->iArgumentIndex;
        iStopIndexSaved = 1;
      }
      while ((ptcArgument = OptionsGetNextArgument(psOptionsContext)) != NULL)
      {
        OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_OPERAND);
      }
      break;
    }

    /*-
     *******************************************************************
     *
     * Scan the options table looking for a match on the nick name or
     * the full name. If a match is found, pass the option and its
     * argument, if any, to the designated handler.
     *
     *******************************************************************
     */
    for (iIndex = 0; iIndex < psOptionsContext->iNOptions; iIndex++)
    {
      if (
           (psOptions[iIndex].atcNickName[0] && _tcscmp(ptcArgument, psOptions[iIndex].atcNickName) == 0) ||
           (psOptions[iIndex].atcFullName[0] && _tcscmp(ptcArgument, psOptions[iIndex].atcFullName) == 0)
         )
      {
        if (psOptions[iIndex].iAllowRepeats == 0 && psOptions[iIndex].iFound == 1)
        {
          _sntprintf(ptcError, MESSAGE_SIZE, _T("OptionsProcessOptions(): The %s option may not be specified more than once."), ptcArgument);
          return OPTIONS_ER;
        }
        OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_OPTION);
/* TODO: Modify this section (down to the break) to support multiple arguments per option. */
        if (psOptions[iIndex].iNArguments > 0)
        {
          if (OptionsGetArgumentsLeft(psOptionsContext) < 1)
          {
            return OPTIONS_USAGE;
          }
          ptcArgument = OptionsGetNextArgument(psOptionsContext);
          OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_OPTION_ARGUMENT);
        }
        else
        {
          ptcArgument = NULL;
        }
        psOptions[iIndex].iFound = 1;
        iError = psOptions[iIndex].piHandler(&psOptions[iIndex], ptcArgument, pvProperties, atcLocalError);
        if (iError != OPTIONS_OK)
        {
          _sntprintf(ptcError, MESSAGE_SIZE, _T("OptionsProcessOptions(): %s"), atcLocalError);
          return OPTIONS_ER;
        }
        break;
      }
    }

    /*-
     *******************************************************************
     *
     * Reject unmatched arguments that look like options. Treat single
     * hyphens ("-") as arguments/operands rather than options. Save
     * the index of the first argument that looks like an operand. It
     * will be needed to adjust the options context before returning
     * to the caller. In older implementations of this routine, option
     * processing stopped once the first operand was identified.
     *
     *******************************************************************
     */
    if (iIndex == psOptionsContext->iNOptions)
    {
      if (ptcArgument[0] == '-' && ptcArgument[1] != 0)
      {
        _sntprintf(ptcError, MESSAGE_SIZE, _T("OptionsProcessOptions(): option=[%s]: Unknown option."), ptcArgument);
        return OPTIONS_ER;
      }
      else
      {
        OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_OPERAND);
        if (!iStopIndexSaved)
        {
          iStopIndex = psOptionsContext->iArgumentIndex - 1; /* Subtract one from the index so the next call to OptionsGetNextArgument() outside this routine will get the correct argument (i.e., the argument that was just checked). */
          iStopIndexSaved = 1;
        }
      }
    }
  }

  /*-
   *********************************************************************
   *
   * Set the argument index back to where it should have been if option
   * processing stopped at the first operand.
   *
   *********************************************************************
   */
  psOptionsContext->iArgumentIndex = iStopIndex;

  return OPTIONS_OK;
}


/*-
 ***********************************************************************
 *
 * OptionsSetArgumentType
 *
 ***********************************************************************
 */
void
OptionsSetArgumentType(OPTIONS_CONTEXT *psOptionsContext, int iType)
{
  psOptionsContext->piArgumentTypes[psOptionsContext->iArgumentIndex] = iType;
}


/*-
 ***********************************************************************
 *
 * OptionsSetOptions
 *
 ***********************************************************************
 */
void
OptionsSetOptions(OPTIONS_CONTEXT *psOptionsContext, OPTIONS_TABLE *psOptions, int iNOptions)
{
  psOptionsContext->psOptions = psOptions;
  psOptionsContext->iNOptions = iNOptions;
}
