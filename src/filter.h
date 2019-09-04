/*-
 ***********************************************************************
 *
 * $Id: filter.h,v 1.1 2019/08/22 21:57:35 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2019-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FILTER_H_INCLUDED
#define _FILTER_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define FILTER_PREFIX_FOR_PCRE_MD5 "f_md5 =~ "
#define FILTER_PREFIX_FOR_PCRE_NAME "f_name =~ "
#define FILTER_PREFIX_FOR_PCRE_SHA1 "f_sha1 =~ "
#define FILTER_PREFIX_FOR_PCRE_SHA256 "f_sha256 =~ "

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _FILTER_LIST_KLEL
{
  unsigned int        uiId;
  char               *pcExpression;
  int                 iType;
  KLEL_CONTEXT       *psContext;
  unsigned long       ulMask;
  struct _FILTER_LIST_KLEL *psNext;
} FILTER_LIST_KLEL;

typedef struct _FILTER_TYPE_SPEC
{
  const char         *pcName;
  KLEL_EXPR_TYPE      iType;
} FILTER_TYPE_SPEC;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 FilterAddFilter(char *pcExpression, FILTER_LIST_KLEL **psHead, char *pcError);
//void                FilterApplyFilters(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, int iFilterWhen); /* This is defined in ftimes.h */
int                 FilterCheckFilterMasks(FILTER_LIST_KLEL *psFilterList, unsigned long ulMask, char *pcError);
int                 FilterClassifyFilter(FILTER_LIST_KLEL *psFilter, char *pcError);
char               *FilterConvertFromPcre(char *pcExpression, char *pcConversionPrefix, char *pcError);
char               *FilterEvaluateFilterNode(KLEL_NODE *psNode, int iLevel, char *pcError);
void                FilterFreeFilter(FILTER_LIST_KLEL *psFilter);
int                 FilterGetFilterCount(FILTER_LIST_KLEL *psFilterList, int iType);
KLEL_EXPR_TYPE      FilterGetTypeOfVar(const char *pcName, void *pvContext);
KLEL_VALUE         *FilterGetValueOfVar(const char *pcName, void *pvContext);
//FILTER_LIST_KLEL   *FilterMatchFilter(FILTER_LIST_KLEL *psFilterList, FTIMES_FILE_DATA *psFTFileData, int iFilterWhen); /* This is defined in ftimes.h */
FILTER_LIST_KLEL   *FilterNewFilter(char *pcExpression, char *pcError);

#endif /* !_FILTER_H_INCLUDED */
