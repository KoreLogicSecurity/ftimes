######################################################################
#
# $Id: common-subdir.mk.in,v 1.2 2007/02/23 00:12:33 mavrik Exp $
#
######################################################################
#
# Purpose: Common Makefile guts for intermediate subdirs.
#
######################################################################

all test install clean:
	@cwd=`pwd` ; for subdir in ${SUBDIRS} "end-of-list" ; do\
		if test $${subdir} != "end-of-list" ; then\
			${SUBDIR_TRACE} && ${SUBDIR_MAKE} ;\
		fi ;\
	done

clean-all:
	@cwd=`pwd` ; for subdir in ${SUBDIRS} "end-of-list" ; do\
		if test $${subdir} != "end-of-list" ; then\
			${SUBDIR_TRACE} && ${SUBDIR_MAKE} ;\
		fi ;\
	done
	@rm -f Makefile

include ${PROJECT_ROOT}/Mk/common.mk

