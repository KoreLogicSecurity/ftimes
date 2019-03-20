#include <stdio.h>
#include <windows.h>

#define PROGRAM       "test2"
#define PROGRAM_W    L"test2"

#define MAX_NAME_LENGTH 1000

#define FWGAS         "fwgas" /* File With Growing ASCII Streams */
#define FWGUS        L"fwgus" /* File With Growing Unicode Streams */

int
main(int argc, char *argv[])
{
  char   cObject[MAX_NAME_LENGTH + 1];
  char   cName[MAX_NAME_LENGTH + 1];
  HANDLE h;
  int    i;
  int    j;
  int    n;
  WCHAR  wcObject[MAX_NAME_LENGTH + 1];
  WCHAR  wcName[MAX_NAME_LENGTH + 1];
  
  /* ASCII */

  sprintf(cObject, "%s.%s", PROGRAM, FWGAS);

  n = sprintf(cName, "%s:", cObject);
  
  for (i = 0; i < MAX_NAME_LENGTH; i++)
  {
    for (j = 0; j < i + 1; j++)
    {
      cName[n+j] = 'S';
    }
    cName[n+j] = 0;
    
    h = CreateFile(
      cName,
      GENERIC_WRITE,
      FILE_SHARE_READ,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      0
      );   
    if (h == INVALID_HANDLE_VALUE)
    {
      break;
    }
    CloseHandle(h);
  }
  printf("Object=%s MaxLength=%d LastError=%u\n", cObject, i, GetLastError());
  
  /* Unicode */

  swprintf(wcObject, L"%s.%s", PROGRAM_W, FWGUS);

  n = swprintf(wcName, L"%s:", wcObject);
  
  for (i = 0; i < MAX_NAME_LENGTH; i++)
  {
    for (j = 0; j < i + 1; j++)
    {
      wcName[n+j] = 0x5353;
    }
    wcName[n+j] = 0;
    
    h = CreateFileW(
      wcName,
      GENERIC_WRITE,
      FILE_SHARE_READ,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      0
      );   
    if (h == INVALID_HANDLE_VALUE)
    {
      break;
    }
    CloseHandle(h);
  }
  wprintf(L"Object=%s MaxLength=%d LastError=%u\n", wcObject, i, GetLastError());
  
  return 0;
}
