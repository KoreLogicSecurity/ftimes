#include <stdio.h>
#include <direct.h>
#include <windows.h>

#define PROGRAM      "test1"
#define PROGRAM_W   L"test1"

#define MAX_BASE_LENGTH   2
#define MAX_FILL_LENGTH 128
#define MAX_NAME_LENGTH 255

#define DWAS          "dwas" /* Directory With ASCII Streams */
#define FWAS          "fwas" /* File With ASCII Streams */
#define DWUS         L"dwus" /* Directory With Unicode Streams */
#define FWUS         L"fwus" /* File With Unicode Streams */

#define ER_DWAS           1
#define ER_DWUS           2

int
main(int argc, char *argv[])
{
  char   cObject[MAX_NAME_LENGTH + 1];
  char   cName[MAX_NAME_LENGTH + 1];
  char   cFill[MAX_FILL_LENGTH + 1];
  HANDLE h;
  int    i;
  int    iWriteCount;
  WCHAR  wcObject[MAX_NAME_LENGTH + 1];
  WCHAR  wcName[MAX_NAME_LENGTH + 1];
  WCHAR  wcFill[MAX_FILL_LENGTH + 1];
  
  /* ASCII */

  for (i = 0; i < MAX_FILL_LENGTH; i++)
  {
    cFill[i] = 'S';
  }
  cFill[i] = 0;
  
  /* ASCII File */
  
  sprintf(cObject, "%s.%s", PROGRAM, FWAS);

  for (i = 0; 1; i++)
  {
    sprintf(cName, "%s:%05d%s", cObject, i + 1, cFill);
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
    WriteFile(h, cFill, MAX_FILL_LENGTH, &iWriteCount, NULL);
    CloseHandle(h);
  }
  printf("Object=%s StreamCount=%d LastError=%d\n", cObject, i, GetLastError());

  /* ASCII Directory */
  
  sprintf(cObject, "%s.%s", PROGRAM, DWAS);
  if (_mkdir(cObject) != 0)
  {
    return ER_DWAS;
  }
  
  for (i = 0; 1; i++)
  {
    sprintf(cName, "%s:%05d%s", cObject, i + 1, cFill);
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
    WriteFile(h, cFill, MAX_FILL_LENGTH, &iWriteCount, NULL);
    CloseHandle(h);
  }
  printf("Object=%s StreamCount=%d LastError=%d\n", cObject, i, GetLastError());
    
  /* Unicode */
  
  for (i = 0; i < MAX_FILL_LENGTH; i++)
  {
    wcFill[i] = 0x5353;
  }
  wcFill[i] = 0;
  
  /* Unicode File */
  
  swprintf(wcObject, L"%s.%s", PROGRAM_W, FWUS);

  for (i = 0; 1; i++)
  {
    swprintf(wcName, L"%s:%05d%s", wcObject, i + 1, wcFill);
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
    WriteFile(h, wcFill, MAX_FILL_LENGTH, &iWriteCount, NULL);
    CloseHandle(h);
  }
  wprintf(L"Object=%s StreamCount=%d LastError=%d\n", wcObject, i, GetLastError());
  
  /* Unicode Directory */
  
  swprintf(wcObject, L"%s.%s", PROGRAM_W, DWUS);
  if (_wmkdir(wcObject) != 0)
  {
    return ER_DWUS;
  }
  
  for (i = 0; 1; i++)
  {
    swprintf(wcName, L"%s:%05d%s", wcObject, i + 1, wcFill);
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
    WriteFile(h, wcFill, MAX_FILL_LENGTH, &iWriteCount, NULL);
    CloseHandle(h);
  }
  wprintf(L"Object=%s StreamCount=%d LastError=%d\n", wcObject, i, GetLastError());
  
  return 0;
}
