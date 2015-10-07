set SRC_X86_DEBUG=Win32\Debug\
set SRC_X86_RELEASE=Win32\Release\
set SRC_X64_DEBUG=x64\Debug\
set SRC_X64_RELEASE=x64\Release\
set SRC_SAMPLE_DATA=sampledata\
set DIST_DIR=dist\
set DIST_TIMESTAMP=%DIST_DIR%buildtime.txt
set DIST_SAMPLE_DIR=%DIST_DIR%sample\

rem clean dist
rmdir /S /Q %DIST_DIR%

rem build
call "%VS140COMNTOOLS%vsvars32.bat"
devenv Qol.sln /build "Debug|x86"
if ERRORLEVEL 1 goto :END
devenv Qol.sln /build "Debug|x64"
if ERRORLEVEL 1 goto :END
devenv Qol.sln /build "Release|x86"
if ERRORLEVEL 1 goto :END
devenv Qol.sln /build "Release|x64"
if ERRORLEVEL 1 goto :END

rem mkdir
mkdir %DIST_DIR%
mkdir %DIST_SAMPLE_DIR%App

rem date, time
echo %date% %time% > %DIST_TIMESTAMP%

rem copy
xcopy %SRC_X86_RELEASE%App.exe %DIST_SAMPLE_DIR%App
xcopy %SRC_X86_RELEASE%*.cso %DIST_SAMPLE_DIR%App
xcopy /S %SRC_SAMPLE_DATA%* %DIST_SAMPLE_DIR%sampledata\

:END
pause
