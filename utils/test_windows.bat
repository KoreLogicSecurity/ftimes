@echo off

set CHECK_DEBUG_LEVEL=2
set CLEAN_DEBUG_LEVEL=0
set SETUP_DEBUG_LEVEL=0

set TEST_HARNESS=..\..\..\..\..\utils\test_harness

set PROGRAM=build\ftimes.exe
if not exist "%PROGRAM%" goto :next_test
set TARGET_PROGRAM=..\..\..\..\..\%PROGRAM%
cd tests\ftimes\common\compare\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_2
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_3
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\decoder\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\dig\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_2
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_3
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\map\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_2
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_3
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_4
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_5
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\test_6
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\..\common_windows_ads\map\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\..\..\..
:next_test
set PROGRAM=tools\tarmap\build\tarmap.exe
if not exist "%PROGRAM%" goto :next_test
set TARGET_PROGRAM=..\..\..\..\..\%PROGRAM%
cd tests\tarmap\common\map\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
cd ..\..\..\..\..
:next_test
