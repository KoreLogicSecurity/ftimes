/*-
 ***********************************************************************
 *
 * $Id: message.c,v 1.5 2003/02/24 19:36:00 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static char           gcNewLine[NEWLINE_LENGTH] = CRLF;
#endif
#ifdef UNIX
static char           gcNewLine[NEWLINE_LENGTH] = LF;
#endif
static FILE          *gpFile;
static int            giLevel = MESSAGE_DEBUGGER;

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
  strcpy(gcNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
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
  static char         ccMessageQueue[MESSAGE_QUEUE_LENGTH][MESSAGE_SIZE];
  static int          n = 0;
  int                 i,
                      iLength;

  /*-
   *********************************************************************
   *
   * The MessageHandler is strictly for use with Log file output. In
   * particular, it can't manage independent queues for multiple file
   * descriptors.
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
        snprintf(ccMessageQueue[n], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
        ccMessageQueue[n][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      }
      else
      {
        for (i = 0; i < n; i++)
        {
          if (gpFile != NULL && gpFile != stderr)
          {
            iLength = strlen(ccMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&ccMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gcNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gcNewLine);
            }
            else
            {
              iLength += snprintf(&ccMessageQueue[i][iLength], 3, "%s", gcNewLine);
            }
            fwrite(ccMessageQueue[i], iLength, 1, gpFile);
          }
          ccMessageQueue[i][0] = 0;
        }
        n = 0;

        if (gpFile != NULL && gpFile != stderr)
        {
          fflush(gpFile);
        }

        snprintf(ccMessageQueue[n], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
        ccMessageQueue[n][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      }
      fprintf(stderr, "%*.*s|%s%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage, gcNewLine);
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
            iLength = strlen(ccMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&ccMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gcNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gcNewLine);
            }
            else
            {
              iLength += snprintf(&ccMessageQueue[i][iLength], 3, "%s", gcNewLine);
            }
            fwrite(ccMessageQueue[i], iLength, 1, gpFile);
          }
          ccMessageQueue[i][0] = 0;
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
      snprintf(ccMessageQueue[0], MESSAGE_SIZE, "%*.*s|%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage);
      ccMessageQueue[0][MESSAGE_SIZE - 1] = 0; /* Force termination. */
      if (gpFile != NULL && gpFile != stderr)
      {
        iLength = strlen(ccMessageQueue[0]);
        if (iLength >= MESSAGE_SIZE - 3)
        {
          snprintf(&ccMessageQueue[0][MESSAGE_SIZE - 3], 3, "%s", gcNewLine);
          iLength = MESSAGE_SIZE - 3 + strlen(gcNewLine);
        }
        else
        {
          iLength += snprintf(&ccMessageQueue[0][iLength], 3, "%s", gcNewLine);
        }
        fwrite(ccMessageQueue[0], iLength, 1, gpFile);
      }
      ccMessageQueue[0][0] = 0;
      fprintf(stderr, "%*.*s|%s%s", MESSAGE_WIDTH, MESSAGE_WIDTH, pcCode, pcMessage, gcNewLine);
    }
    else
    {
      if (n > 0)
      {
        for (i = 0; i < n; i++)
        {
          if (gpFile != NULL && gpFile != stderr)
          {
            iLength = strlen(ccMessageQueue[i]);
            if (iLength >= MESSAGE_SIZE - 3)
            {
              snprintf(&ccMessageQueue[i][MESSAGE_SIZE - 3], 3, "%s", gcNewLine);
              iLength = MESSAGE_SIZE - 3 + strlen(gcNewLine);
            }
            else
            {
              iLength += snprintf(&ccMessageQueue[i][iLength], 3, "%s", gcNewLine);
            }
            fwrite(ccMessageQueue[i], iLength, 1, gpFile);
          }
          ccMessageQueue[i][0] = 0;
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
