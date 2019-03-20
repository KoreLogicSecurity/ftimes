#include <stdio.h>

int
main()
{
  char cFilename[16];
  FILE *pFile;
  int i;
  int j;
  int test = 1;
  unsigned char cData[8192];
  unsigned char cString[16] = { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e };

  memset(cData, 0, sizeof(cData));

  pFile = fopen("test1.zeros", "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fwrite(cData, 1, 8192, pFile);
  fclose(pFile);
  
  sprintf(cFilename, "test1.cfg%d", test++);
  pFile = fopen(cFilename, "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fprintf(pFile, "DigString=%%00\n");
  fclose(pFile);

  sprintf(cFilename, "test1.cfg%d", test++);
  pFile = fopen(cFilename, "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fprintf(pFile, "DigString=%%00%%00\n");
  fclose(pFile);

  memcpy(&cData[0], cString, 16);
  memcpy(&cData[4096 - 16], cString, 16);
  memcpy(&cData[4096], cString, 16);
  memcpy(&cData[8192 - 16], cString, 16);

  for (i = 0; i < 9; i++)
  {
    sprintf(cFilename, "test1.data%d", i);
    pFile = fopen(cFilename, "wb");
    if (pFile == NULL)
    {
      return -1;
    }
    for (j = 0; j < i; j++)
    {
      fwrite(cData, 1, 8192, pFile);
    }
    fclose(pFile);
  }

// Basic string

  sprintf(cFilename, "test1.cfg%d", test++);
  pFile = fopen(cFilename, "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fprintf(pFile, "DigString=%%d4%%1d%%8c%%d9%%8f%%00%%b2%%04%%e9%%80%%09%%98%%ec%%f8%%42%%7e\n");
  fclose(pFile);

// Little string in big string

  sprintf(cFilename, "test1.cfg%d", test++);
  pFile = fopen(cFilename, "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fprintf(pFile, "DigString=%%d4%%1d%%8c%%d9%%8f%%00%%b2%%04%%e9%%80%%09%%98%%ec%%f8%%42%%7e\n");
  fprintf(pFile, "DigString=%%ec%%f8%%42%%7e\n");
  fclose(pFile);

// String splits read boundary

  sprintf(cFilename, "test1.cfg%d", test++);
  pFile = fopen(cFilename, "wb");
  if (pFile == NULL)
  {
    return -1;
  }
  fprintf(pFile, "DigString=%%8f%%00%%b2%%04%%e9%%80%%09%%98%%ec%%f8%%42%%7e%%d4%%1d%%8c%%d9\n");
  fclose(pFile);
  return 0;
}
