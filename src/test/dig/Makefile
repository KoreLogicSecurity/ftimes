########################################################################
#
# $Id: Makefile,v 1.1.1.1 2002/01/18 03:17:51 mavrik Exp $
#
########################################################################
#
# Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
# All Rights Reserved.
#
########################################################################
#
# Purpose: Makefile for a single test.
#
########################################################################

TEST_SOURCES		= test1.c
TEST_OBJECTS		= ${TEST_SOURCES:.c=.o}
TEST_EXECUTEABLE	= test1
DATA_FILES		=\
			test1.data0\
			test1.data1\
			test1.data2\
			test1.data3\
			test1.data4\
			test1.data5\
			test1.data6\
			test1.data7\
			test1.data8\
			test1.zeros
CONFIG_FILES		=\
			test1.cfg1\
			test1.cfg2\
			test1.cfg3\
			test1.cfg4\
			test1.cfg5


all: ${TEST_EXECUTEABLE}

clean:
	rm -f ${TEST_EXECUTEABLE} ./*.core core

clean-all: clean
	rm -f ${DATA_FILES} ${CONFIG_FILES}

dist-clean: clean-all

.c.o:
	${CC} ${CFLAGS} ${IFLAGS} -c $< 

