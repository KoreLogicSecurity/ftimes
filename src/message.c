/*-
 ***********************************************************************
 *
 * $Id: message.c,v 1.13 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static char           gacNewLine[NEWLINE_LENGTH] = CRLF;
#endif
#ifdef UNIX
static char           gacNewLine[NEWLINE_LENGTH] = LF;
#endif
static FILE          *gpFile;
static int            giAutoFlush = MESSAGE_AUTO_FLUSH_OFF;
static int            giLevel = MESSAGE_DEBUGGER;

/*-
 ***********************************************************************
 *
 * MessageSetAutoFlush
 *
 ***********************************************************************
 */
void
MessageSetAutoFlush(int iOnOff)
{
  giAutoFlush = (iOnOff == MESSAGE_AUTO_FLUSH_OFF) ? MESSAGE_AUTO_FLUSH_OFF : MESSAGE_AUTO_FLUSH_ON;
}


/*-
 ***********************************************************************
 *
 * MessageSetLogLevel
 *
 ***********************************************************************
 */
void
MessageSetLogLevel(int iLevel)
{
  giLevel = iLevel;
}


/*-
 ***********************************************************************
 *
 * MessageSetNewLine
 *
 ***********************************************************************
 */
void
MessageSetNewLine(char *pcNewLine)
{
  strcpy(gacNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
}


/*-
 ***********************************************************************
 *
 * MessageSetOutputStream
 *
 ***********************************************************************
 */
void
MessageSetOutputStream(FILE *pFile)
{
  gpFile = pFile;
}


/*-
 ***********************************************************************
 *
 * MessageHandler
 *
 ***********************************************************************
 */
void
MessageHandler(int iAction, int iLevel, char *pcCode, char *pcMessage)
{
  static char         aacMessageQueue[MESSAGE_QUEUE_LENGTH][MESSAGE_SIZE];
  static int          n = 0;
  int                 i = 0;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * The MessageHandler is strictly for use with Log file output. In
   * particular, it can't manage independent queues for multiple file
   * descriptors.
   *
   *********************************************************************
   */

  /*-
   *********************************************************************
   *
   * If the specified level doesn't match or exceed the current global
   * level, ignore the request.
   *
   *********************************************************************
   */
  if (iLevel < giLevel)
  {
    return;
  }

  /*-
   *********************************************************************
   *
   * The MessageHandler has the right to enforce a flush. This knob is
   * typically activated when the output file (not stderr) is open and
   * ready for writing. Once that happens, there's no longer a need to
   * queue messages. Enabling auto flush helps to ensure that messages
   * are written to disk as soon as they are generated.
   *
   *********************************************************************
   */
  if (giAutoFlush == MESSAGE_AUTO_FLUSH_ON)
  {
    iAction = MESSAGE_FLUSH_IT;
  }

  /*-
   *********************************************************************
   *
   * Queue the message and copy it to stderr. If the queue is full,
   * flush it. If a flush is required and gpFile is not defined,
   * silently discard all queued messages.
   *
   *********************************************************************
   */
  if (iAction == MESSAGE_QUEUE_IT)
  {
    if (pcCode && pcMessage)
    {
      if (n < MESSAGE_QUEUE_LENGTH)
      {
        snprintf(aacMessageQueue[n], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
        aacMessageQueue[n][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      }
      else
      {
        for (i = 0; i < n; i++)
        {
          if (gpFile != NULL && gpFile != stderr)
          {
            iLength = strlen(aacMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&aacMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gacNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gacNewLine);
            }
            else
            {
              iLength += snprintf(&aacMessageQueue[i][iLength], 3, "%s", gacNewLine);
            }
            fwrite(aacMessageQueue[i], iLength, 1, gpFile);
          }
          aacMessageQueue[i][0] = 0;
        }
        n = 0;
        if (gpFile != NULL && gpFile != stderr)
        {
          fflush(gpFile);
        }
        snprintf(aacMessageQueue[n], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
        aacMessageQueue[n][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      }
      fprintf(stderr, "%*.*s|%s%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage, gacNewLine);
      fflush(stderr);
      n++;
    }
    return;
  }

  /*-
   *********************************************************************
   *
   * Flush the queue, if it's not empty. If gpFile is not defined,
   * silently discard all queued messages. If a new message was
   * passed, write it to gpFile, if defined, as well as stderr.
   *
   *********************************************************************
   */
  if (iAction == MESSAGE_FLUSH_IT)
  {
    if (pcCode && pcMessage)
    {
      if (n > 0)
      {
        for (i = 0; i < n; i++)
        {
          if (gpFile != NULL && gpFile != stderr)
          {
            iLength = strlen(aacMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&aacMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gacNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gacNewLine);
            }
            else
            {
              iLength += snprintf(&aacMessageQueue[i][iLength], 3, "%s", gacNewLine);
            }
            fwrite(aacMessageQueue[i], iLength, 1, gpFile);
          }
          aacMessageQueue[i][0] = 0;
        }
        n = 0;
      }

      /*-
       *****************************************************************
       *
       * Handle the outstanding message passed in by the caller.
       *
       *****************************************************************
       */
      snprintf(aacMessageQueue[0], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
      aacMessageQueue[0][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      if (gpFile != NULL && gpFile != stderr)
      {
        iLength = strlen(aacMessageQueue[0]);
        if (iLength >= MESSAGE_SIZE - 3)
        {
          snprintf(&aacMessageQueue[0][MESSAGE_SIZE - 3], 3, "%s", gacNewLine);
          iLength = MESSAGE_SIZE - 3 + strlen(gacNewLine);
        }
        else
        {
          iLength += snprintf(&aacMessageQueue[0][iLength], 3, "%s", gacNewLine);
        }
        fwrite(aacMessageQueue[0], iLength, 1, gpFile);
      }
      aacMessageQueue[0][0] = 0;
      fprintf(stderr, "%*.*s|%s%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage, gacNewLine);
      fflush(stderr);
    }
    else
    {
      if (n > 0)
      {
        for (i = 0; i < n; i++)
        {
          if (gpFile != NULL && gpFile != stderr)
          {
            iLength = strlen(aacMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&aacMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gacNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gacNewLine);
            }
            else
            {
              iLength += snprintf(&aacMessageQueue[i][iLength], 3, "%s", gacNewLine);
            }
            fwrite(aacMessageQueue[i], iLength, 1, gpFile);
          }
          aacMessageQueue[i][0] = 0;
        }
        n = 0;
      }
    }
    if (gpFile != NULL && gpFile != stderr)
    {
      fflush(gpFile);
    }
    return;
  }
}
