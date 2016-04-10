set TOOL_DIR=..\tools\doxygen-1.8.11.windows.bin\
set DOXYGEN=%TOOL_DIR%doxygen

call clean.bat
%DOXYGEN%
