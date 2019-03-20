########################################################################
#
# $Id: Makefile.vs,v 1.1 2011/03/02 06:11:54 mavrik Exp $
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
			/D WINNT\
			/I"."\
			/Fd"$(OBJECT_DIR)\\"\
			/Fo"$(OBJECT_DIR)\\"\
			/Fp"$(OBJECT_DIR)\ftimes-xpa.pch"\
!IF "$(BUILD_TYPE)" == "DEBUG" || "$(BUILD_TYPE)" == "debug"
			/D _DEBUG\
			/c /W3 /MTd /Od /Zi /Gm
!ELSE
			/D NDEBUG\
			/c /W3 /MT /O2
!ENDIF

INCLUDES		=\
			"$(SOURCE_DIR)\all-includes.h"\
			"$(SOURCE_DIR)\md5.h"\
			"$(SOURCE_DIR)\options.h"\
			"$(SOURCE_DIR)\sha1.h"\
			"$(SOURCE_DIR)\ftimes-xpa.h"

OBJECTS			=\
			"$(OBJECT_DIR)\md5.obj"\
			"$(OBJECT_DIR)\options.obj"\
			"$(OBJECT_DIR)\sha1.obj"\
			"$(OBJECT_DIR)\ftimes-xpa.obj"

EXECUTEABLE		= $(OBJECT_DIR)\ftimes-xpa.exe

LINKER			= link.exe

LINKER_FLAGS		=\
			/nologo\
			/subsystem:console\
			/machine:x86\
			/out:"$(EXECUTEABLE)"\
!IF "$(BUILD_TYPE)" == "DEBUG" || "$(BUILD_TYPE)" == "debug"
			/incremental:yes\
			/debug\
			/pdb:"$(OBJECT_DIR)\ftimes-xpa.pdb"\
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
	copy ..\..\doc\ftimes-xpa.html "$(INSTALL_DIR)\doc"

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

"$(OBJECT_DIR)\options.obj": "$(SOURCE_DIR)\options.c" $(INCLUDES) "$(OBJECT_DIR)"

"$(OBJECT_DIR)\sha1.obj": "$(SOURCE_DIR)\sha1.c" $(INCLUDES) "$(OBJECT_DIR)"

"$(OBJECT_DIR)\ftimes-xpa.obj": "$(SOURCE_DIR)\ftimes-xpa.c" $(INCLUDES) "$(OBJECT_DIR)"

