@echo off

: $Id$

call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
cl /O2 /MD /nologo %1 /link /NOLOGO

cd /d G:\DDS\maplus_conv\src
copy /y maplus_conv.exe ..\			>nul
copy /y maplus_conv.exe ..\maplus_ggl.exe	>nul
