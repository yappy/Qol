set SRC_X86_DEBUG=Win32\Debug\
set SRC_X86_RELEASE=Win32\Release\
set SRC_X64_DEBUG=x64\Debug\
set SRC_X64_RELEASE=x64\Release\
set SRC_SAMPLE_DATA=sampledata\
set DOC_DIR=doc\
set DIST_DIR=dist\
set DIST_TIMESTAMP=%DIST_DIR%buildtime.txt
set DIST_SAMPLE_DIR=%DIST_DIR%sample\
set DIST_DOC_DIR=%DIST_DIR%doc\

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

rem doc
pushd %DOC_DIR%
call doc.bat
if ERRORLEVEL 1 goto :END
popd

rem mkdir
mkdir %DIST_DIR%
mkdir %DIST_SAMPLE_DIR%App
mkdir %DIST_DOC_DIR%html

rem date, time
echo %date% %time% > %DIST_TIMESTAMP%

rem copy
xcopy %SRC_X86_RELEASE%App.exe %DIST_SAMPLE_DIR%App
xcopy %SRC_X86_RELEASE%App.pdb %DIST_SAMPLE_DIR%App
xcopy %SRC_X86_RELEASE%App.map %DIST_SAMPLE_DIR%App
xcopy %SRC_X86_RELEASE%*.cso %DIST_SAMPLE_DIR%App
xcopy /S %SRC_SAMPLE_DATA%* %DIST_SAMPLE_DIR%sampledata\
xcopy /S %DOC_DIR%html %DIST_DOC_DIR%html

:END
pause
