@echo off

set CHECK_DEBUG_LEVEL=2
set CLEAN_DEBUG_LEVEL=0
set SETUP_DEBUG_LEVEL=0

set TARGET_PROGRAM=..\..\..\..\build\ftimes.exe

set TEST_HARNESS=..\..\..\..\utils\test_harness

cd tests\common\compare\test_1
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

cd ..\..\..\common_windows_ads\map\test_1
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m setup -d %SETUP_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m check -d %CHECK_DEBUG_LEVEL%
perl %TEST_HARNESS% -p %TARGET_PROGRAM% -m clean -d %CLEAN_DEBUG_LEVEL%

cd ..\..\..\..
