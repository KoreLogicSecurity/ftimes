######################################################################
#
# $Id: common-tests.mk.in,v 1.6 2007/04/14 19:42:08 mavrik Exp $
#
######################################################################
#
# Purpose: Common guts for all test Makefiles.
#
######################################################################

WORKDIR=work

CHECK_DEBUG_LEVEL?=2

CLEAN_DEBUG_LEVEL?=0

SETUP_DEBUG_LEVEL?=0

all:

install:

_clean_done:
	@if test -n "${PROJECT_LIBDIR}" ; then options="-l ${PROJECT_LIBDIR}" ; fi ;\
	perl ${TEST_HARNESS} -m clean -w ${WORKDIR} -p ${TARGET_PROGRAM} -d ${CLEAN_DEBUG_LEVEL} -s ${PROJECT_SRCDIR} $${options}

clean:
	@rm -f _config _check_done _clean_done _setup_done _stdout _stderr
	@rm -rf ${WORKDIR}

clean-all: clean
	@rm -f _clean_done Makefile

_setup_done: clean
	@if test -n "${PROJECT_LIBDIR}" ; then options="-l ${PROJECT_LIBDIR}" ; fi ;\
	perl ${TEST_HARNESS} -m setup -w ${WORKDIR} -p ${TARGET_PROGRAM} -d ${SETUP_DEBUG_LEVEL} -s ${PROJECT_SRCDIR} $${options}

setup: _setup_done

_check_done: _setup_done
	@if test -n "${PROJECT_LIBDIR}" ; then options="-l ${PROJECT_LIBDIR}" ; fi ;\
	perl ${TEST_HARNESS} -m check -w ${WORKDIR} -p ${TARGET_PROGRAM} -d ${CHECK_DEBUG_LEVEL} -s ${PROJECT_SRCDIR} $${options}

check: _check_done

test: _check_done

include ${INCLUDES_PREFIX}/common.mk

