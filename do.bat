@echo off

: $Id$

call "%VS90COMNTOOLS%vsvars32.bat"

cl imagehlp.lib /O2 /MD /nologo %1 /link /NOLOGO

: cd /d G:\DDS\maplus_conv\maplus_conv
copy /y maplus_conv.exe ..\			>nul
copy /y maplus_conv.exe ..\maplus_ggl.exe	>nul
