########################################################################
#
# $Id: Makefile,v 1.1.1.1 2002/01/18 03:16:26 mavrik Exp $
#
########################################################################
#
# Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
# All Rights Reserved.
#
########################################################################
#
# Purpose: Makefile for a single pod document.
#
########################################################################

.SUFFIXES: .pod .html .1

POD_NAME		= FTimes
POD_FILE		= ftimes.pod
VERSION_FILE		= ../../src/app-includes.h
SRC_FILES		=\
			sections/NAME.pod\
			sections/SYNOPSIS.pod\
			sections/DESCRIPTION.pod\
			sections/MODES-OF-OPERATION.pod\
			sections/OPTIONS.pod\
			sections/CONFIGUARTION-CONTROLS.pod\
			sections/CONTROL-DESCRIPTIONS.head.pod\
			controls/*.pod\
			sections/CONTROL-DESCRIPTIONS.tail.pod\
			sections/RETURN-VALUES.pod\
			sections/FILES.pod\
			sections/NOTES.pod\
			sections/EXAMPLES.pod\
			sections/SEE-ALSO.pod\
			sections/AUTHOR.pod\
			sections/HISTORY.pod
DST_FILES		=\
			ftimes.html\
			ftimes.1

all: ${POD_FILE}

clean:
	rm -f pod2html-dircache pod2html-itemcache

clean-all: clean
	rm -f ${POD_FILE}

dist: ${DST_FILES} clean

dist-clean: clean-all

${POD_FILE}: ${SRC_FILES}
	umask 22; cat ${SRC_FILES} > ${POD_FILE}

.pod.1:
	version=`grep VERSION ${VERSION_FILE} | sed '1d' | awk '{print $$3}' | sed 's/"//g'`;\
	umask 22; pod2man --center="${POD_NAME} Documentation" --release="${POD_NAME} $$version" $< > ../$@

.pod.html:
	title=`echo $< | sed 's/\.pod//' | sed 's/_/ /g'`;\
	umask 22; pod2html --infile=$< --outfile=../$@ --noindex --title="$$title"
