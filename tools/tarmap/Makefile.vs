########################################################################
#
# $Id: Makefile.vs,v 1.5 2010/06/18 18:00:04 mavrik Exp $
#
########################################################################
#
# Purpose: Makefile for Visual Studio.
#
########################################################################

BUILD_TYPE		= RELEASE	# [RELEASE|DEBUG]

INSTALL_DIR		= C:\FTimes
SOURCE_DIR		= .
OBJECT_DIR		= b

COMPILER		= cl.exe

COMPILER_FLAGS		=\
			/nologo\
			/D _MBCS\
			/D _CONSOLE\
			/D _CRT_SECURE_NO_DEPRECATE\
			/D WIN32\
			/I"."\
			/Fo"$(OBJECT_DIR)\\"\
			/Fd"$(OBJECT_DIR)\\"\
			/Fp"$(OBJECT_DIR)\tarmap.pch"\
			/c /W3 /EHsc /FD\
!IF "$(BUILD_TYPE)" == "DEBUG" || "$(BUILD_TYPE)" == "debug"
			/D _DEBUG\
			/MTd /Od /Zi /Gm
!ELSE
			/D NDEBUG\
			/MT /O2
!ENDIF

INCLUDES		=\
			"$(SOURCE_DIR)\all-includes.h"\
			"$(SOURCE_DIR)\md5.h"\
			"$(SOURCE_DIR)\sha1.h"\
			"$(SOURCE_DIR)\tarmap.h"

OBJECTS			=\
			"$(OBJECT_DIR)\md5.obj"\
			"$(OBJECT_DIR)\sha1.obj"\
			"$(OBJECT_DIR)\tarmap.obj"

EXECUTEABLE		= $(OBJECT_DIR)\tarmap.exe

LINKER			= link.exe

LINKER_FLAGS		=\
			/nologo\
			/subsystem:console\
			/machine:x86\
			/out:"$(EXECUTEABLE)"\
!IF "$(BUILD_TYPE)" == "DEBUG" || "$(BUILD_TYPE)" == "debug"
			/incremental:yes\
			/debug\
			/pdb:"$(OBJECT_DIR)\tarmap.pdb"\
!ELSE
			/incremental:no\
			/release\
			/pdb:none\
!ENDIF
			advapi32.lib wsock32.lib

all: "$(EXECUTEABLE)"

install: "$(EXECUTEABLE)"
	if not exist "$(INSTALL_DIR)" mkdir "$(INSTALL_DIR)"
	if not exist "$(INSTALL_DIR)\bin" mkdir "$(INSTALL_DIR)\bin"
	if not exist "$(INSTALL_DIR)\doc" mkdir "$(INSTALL_DIR)\doc"
	copy "$(EXECUTEABLE)" "$(INSTALL_DIR)\bin"
	copy ..\..\doc\tarmap.html "$(INSTALL_DIR)\doc"

clean:
	if exist "$(OBJECT_DIR)" rd /Q /S "$(OBJECT_DIR)"

clean-all: clean

test: all

"$(EXECUTEABLE)": "$(OBJECT_DIR)" $(OBJECTS)
	$(LINKER) $(LINKER_FLAGS) $(OBJECTS)

{$(SOURCE_DIR)}.c{$(OBJECT_DIR)}.obj::
	$(COMPILER) $(COMPILER_FLAGS) $<

"$(OBJECT_DIR)":
	if not exist "$(OBJECT_DIR)" mkdir "$(OBJECT_DIR)"

"$(OBJECT_DIR)\md5.obj": "$(SOURCE_DIR)\md5.c" $(INCLUDES) "$(OBJECT_DIR)"

"$(OBJECT_DIR)\sha1.obj": "$(SOURCE_DIR)\sha1.c" $(INCLUDES) "$(OBJECT_DIR)"

"$(OBJECT_DIR)\tarmap.obj": "$(SOURCE_DIR)\tarmap.c" $(INCLUDES) "$(OBJECT_DIR)"

