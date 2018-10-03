@echo off
setlocal enabledelayedexpansion


set my_dir=%cd%


:search_remote_make
IF EXIST %cd%\remote_make (
	set remote_dir=%cd%\remote_make
	goto remote_make_found
) ELSE (
	set tmp_cd=%cd%
	cd ..
	if "%tmp_cd%"=="%cd%" (
		goto remote_make_not_found
	)
	goto search_remote_make
)


:remote_make_found
echo remote make dir: %remote_dir%
echo project location: %my_dir%


						   
if "%1" == "" (
    goto make
)

if "%1" == "clean" (
	goto make_clean
)

rem only to get config
if "%1" == "end" (
	goto eof
)

if "%1" == "flash" (
	goto flash
)

goto unknown

:make
echo make
call %remote_dir%\remote2.bat %my_dir% make
goto eof

:make_clean
echo clean
call %remote_dir%\remote2.bat %my_dir% make clean
goto eof

:flash
echo upload
cd %my_dir%
C:\MinGW\bin\mingw32-make.exe flash
goto eof

:unknown
echo unknown command
goto eof


:remote_make_not_found
echo remote make dir not found!

goto eof

:eof
rem 
pause