@echo off
rem claude, TOOL_ONLY: builds the NTIA reference driver outside the repo. Layout: <dir>\ntia_drv\{this .bat, driver.cpp}
rem and <dir>\ntia_src\{LFMF-main, itm-master} unpacked unmodified from NTIA GitHub main.zip:
rem   LFMF sha256 be6ed041d4f095c8845e8cce800e1a4ffb568401dec6f5d93c14b7a8ce58fd95 (v1.1, CMake project VERSION)
rem   ITM  sha256 60325b10afa5b2992aea240b9c0295ec02fab6ca5c6e67f7a42c01a16785c226 (v1.4)
rem LFMF and ITM objects go to separate dirs: both ship a ValidateInputs.cpp. NTIA sources are US public domain.
call "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d %~dp0
if not exist obj_lfmf mkdir obj_lfmf
if not exist obj_itm mkdir obj_itm
set CF=/nologo /EHsc /O2 /std:c++17 /c
cl %CF% /I..\ntia_src\LFMF-main\include /DLIBRARY_NAME=\"LFMF\" /DLIBRARY_VERSION=\"1.1\" ..\ntia_src\LFMF-main\src\*.cpp /Foobj_lfmf\ > build.log 2>&1
cl %CF% /I..\ntia_src\itm-master\include ..\ntia_src\itm-master\src\*.cpp /Foobj_itm\ >> build.log 2>&1
if exist driver.obj del driver.obj
if exist ntia_ref.exe del ntia_ref.exe
cl %CF% /I..\ntia_src\LFMF-main\include /I..\ntia_src\itm-master\include driver.cpp /Fodriver.obj >> build.log 2>&1
if errorlevel 1 (echo exit=COMPILE_FAIL & exit /b 1)
link /nologo driver.obj obj_lfmf\*.obj obj_itm\*.obj /OUT:ntia_ref.exe >> build.log 2>&1
echo exit=%errorlevel%
