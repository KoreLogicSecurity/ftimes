/*-
 ***********************************************************************
 *
 * $Id: pool2string.c,v 1.3 2005/04/02 18:10:26 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2002-2005 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include <stdio.h>
#include "pool2string.h"

/*-
 ***********************************************************************
 *
 * Main
 *
 ***********************************************************************
 */
int
main()
{
  unsigned char aucPool[POOL2STRING_POOL_SIZE];
  unsigned char aucTaps[POOL2STRING_POOL_TAPS];
  
  POOL2STRING_NEW_POOL(aucPool, POOL2STRING_POOL_SIZE, POOL2STRING_POOL_SEED);
  POOL2STRING_TAP_POOL(aucTaps, aucPool);
  printf("String='%s'\n", aucTaps);
  return 0;
}
