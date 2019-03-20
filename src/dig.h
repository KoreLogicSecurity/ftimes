/*-
 ***********************************************************************
 *
 * $Id: dig.h,v 1.28 2013/02/14 16:55:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _DIG_H_INCLUDED
#define _DIG_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define DIG_FIRST_CHAIN_INDEX 0
#define DIG_FINAL_CHAIN_INDEX 255
#define DIG_MIN_CHAINS 1
#define DIG_MAX_CHAINS 256
#define DIG_MAX_STRING_SIZE 1024
#define DIG_MAX_TYPE_SIZE 64
#define DIG_MAX_TAG_SIZE 64

#ifdef USE_PCRE
#define PCRE_CAPTURE_INDEX_0L 0 /* This is the low offset of the entire pattern. */
#define PCRE_CAPTURE_INDEX_0H 1 /* This is the high offset of the entire pattern. */
#define PCRE_CAPTURE_INDEX_1L 2 /* This is the low offset of the first capturing subpattern. */
#define PCRE_CAPTURE_INDEX_1H 3 /* This is the high offset of the first capturing subpattern. */
#define PCRE_MAX_CAPTURE_COUNT 9 /* This is the maximum number of capturing '()' subpatterns allowed. */
/*
 * The following quote was taken from pcreapi(3) man page.
 *
 *   The smallest size for ovector that will allow for n captured
 *   substrings, in addition to the  offsets of the substring matched
 *   by the whole pattern, is (n+1)*3.
 *
 */
#define PCRE_OVECTOR_ARRAY_SIZE 30
#endif

enum DigStringTypes
{
  DIG_STRING_TYPE_NORMAL = 0,
  DIG_STRING_TYPE_NOCASE,
#ifdef USE_PCRE
  DIG_STRING_TYPE_REGEXP,
#endif
#ifdef USE_XMAGIC
  DIG_STRING_TYPE_XMAGIC,
#endif
  DIG_STRING_TYPE_NOMORE
};

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _DIG_SEARCH_DATA
{
  char               *pcFile;
  char               *pcTag;
  unsigned char      *pucData;
  int                 iLength;
  int                 iType;
  APP_UI64            ui64Offset;
} DIG_SEARCH_DATA;

typedef struct _DIG_STRING
{
  char               *pcTag; /* User-supplied tag. */

  unsigned char      *pucEncodedString; /* User-supplied dig string (could be URL encoded). */
  unsigned char      *pucDecodedString; /* Decoded version of the user-supplied dig string. */

  int                 iEncodedLength;
  int                 iDecodedLength;

  int                 iHitsPerBuffer; /* Total number of matches for the current buffer. */
  int                 iHitsPerStream; /* Total number of matches for the current stream. */
  int                 iHitsPerJob; /* Total number of matches for the current job. */

  int                 iType; /* The type of dig string (i.e., Normal, NoCase, RegExp). */

#ifdef USE_PCRE
  int                 iOffset; /* The relative location where digging should start. */
  int                 iLastOffset; /* The last relative location that digging took place. */
  int                 iCaptureCount; /* The number of capturing subpatterns in this expression. */
  pcre               *psPcre;
  pcre_extra         *psPcreExtra;
#endif

#ifdef USE_XMAGIC
  XMAGIC             *psXMagic;
#endif

  struct _DIG_STRING *psNext;
} DIG_STRING;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 DigAddDigString(char *pcString, int iType, char *pcError);
void                DigAdjustRegExpOffsets(int iTrimSize);
void                DigClearCounts(void);
//int                 DigDevelopOutput(FTIMES_PROPERTIES *psProperties, DIG_SEARCH_DATA *psSearchData, char *pcError); /* This is declared in ftimes.h. */
void                DigFreeDigString(DIG_STRING *psDigString);
int                 DigGetMaxStringLength(void);
int                 DigGetSaveLength(void);
DIG_STRING         *DigGetSearchList(int iType, int iIndex);
int                 DigGetStringCount(void);
int                 DigGetStringsMatched(void);
char               *DigGetStringType(int iType);
APP_UI64            DigGetTotalMatches(void);
DIG_STRING         *DigNewDigString(char *pcString, int iType, char *pcError);
int                 DigSearchData(unsigned char *pucData, int iDataLength, int iStopShort, int iType, APP_UI64 ui64SearchOffset, char *pcFilename, char *pcError);
void                DigSetMaxStringLength(int iMaxStringLength);
//void                DigSetPropertiesReference(FTIMES_PROPERTIES *psProperties); /* This is declared in ftimes.h. */
void                DigSetSaveLength(int iSaveLength);
int                 DigSetSearchList(DIG_STRING *psDigString, char *pcError);
//int                 DigWriteHeader(FTIMES_PROPERTIES *psProperties, char *pcError); /* This is declared in ftimes.h. */

#endif /* !_DIG_H_INCLUDED */
