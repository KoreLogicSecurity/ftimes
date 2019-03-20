######################################################################
#
# $Id: common-src-c.mk.in,v 1.1 2007/01/02 03:47:39 mavrik Exp $
#
######################################################################
#
# Purpose: Common Makefile guts for gcc-compiled tools.
#
######################################################################

INSTALL_MODE?=755

all: ${TARGET}

clean:
	@rm -f core ${TARGET} ${TARGET}.core *.o

clean-all: clean
	@rm -f Makefile

install-prefix:
	@umask 22 ; if [ ! -d ${INSTALL_PREFIX} ] ; then mkdir -p ${INSTALL_PREFIX} ; fi

install: ${TARGET} install-prefix
	@strip ${TARGET}
	@${INSTALL} -m ${INSTALL_MODE} ${TARGET} ${INSTALL_PREFIX}

test: ${TARGET}

${TARGET}: ${INCS} ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${CFLAGS} ${LIBFLAGS}

.c.o:
	${CC} -c $< ${CFLAGS} ${INCFLAGS}

include ${PROJECT_ROOT}/Mk/common.mk

