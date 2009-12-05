@echo off

: $Id$

call "%VS90COMNTOOLS%vsvars32.bat"

cl imagehlp.lib /O2 /nologo %1 /link /NOLOGO

copy /y maplus_conv.exe ..\			>nul
copy /y maplus_conv.exe ..\maplus_ggl.exe	>nul

cl imagehlp.lib /DMAPLUS3 /O2 /nologo %1 /link /NOLOGO

copy /y maplus_conv.exe ..\maplus_conv3.exe	>nul
copy /y maplus_conv.exe ..\maplus_ggl3.exe	>nul
