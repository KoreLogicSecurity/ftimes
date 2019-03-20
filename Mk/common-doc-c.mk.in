######################################################################
#
# $Id: common-doc-c.mk.in,v 1.1 2007/01/02 22:34:13 mavrik Exp $
#
######################################################################
#
# Purpose: Common guts for Makefiles that build c-based docs.
#
######################################################################

.SUFFIXES: .pod .html .1

INSTALL_MODE?=644

all: ${MAN_FILE} ${HTML_FILE}

install-prefix:
	@umask 22 ; if [ ! -d ${INSTALL_PREFIX} ] ; then mkdir -p ${INSTALL_PREFIX} ; fi

install: ${MAN_FILE} install-prefix
	@${INSTALL} -m ${INSTALL_MODE} ${MAN_FILE} ${INSTALL_PREFIX}

clean:
	@rm -f pod2html-dircache pod2html-itemcache pod2htmd.tmp pod2htmi.tmp

clean-all: clean
	@rm -f Makefile ${POD_FILE} ${MAN_FILE} ${HTML_FILE}

test:

${POD_FILE}: ${SRC_FILES}
	@umask 22 ; cat ${SRC_FILES} > ${POD_FILE}

.pod.1:
	@umask 22 ; pod2man --section=1 --center="${MAN_TITLE}" $< > $@

.pod.html:
	@umask 22 ; title=`echo $< | sed 's/\.pod//' | sed 's/_/ /g'` ;\
	pod2html --infile=$< --outfile=$@ --noindex --title="$${title}" ;\
	perl -n -i.bak -e 'next if (/<link/i); s/(<\/?[Hh])(\d)/($$1.($$2+2))/eg; print;' $@ ;\
	rm -f $@.bak *.tmp
