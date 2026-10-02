@echo off

set thisdir = "%~dp0"
set langfile = %thisdir%\..\app_eclipselauncher\src\main\assets\language_list.txt

del %langfile%
dir %thisdir%\..\app_eclipselauncher\src\main\res\values-* /s /b > %langfile%

