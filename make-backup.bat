@echo on
@call :GET_CURRENT_DATE

:: -----------------------------------------------
@SET PRJNAME=bbi2-esz7-term
@SET BACKUPDIR=..\%PRJNAME%-backups
@SET BACKUPNAME=%PRJNAME%-%DT%

@for /f "tokens=3" %%A in ('findstr /C:"#define VER_NUM " "src\ver.h"') do @(
    @SET VERSION=%%~A
)
@echo VER:  %VERSION%

:: -----------------------------------------------

@SET tmpdir=..\%PRJNAME%-%VERSION%-%DT%
@rd /s /q %tmpdir% 2>NUL
@mkdir %tmpdir% >NUL

xcopy /S /Y /V /H /Q * %tmpdir%\*
@if "%ERRORLEVEL%" NEQ "0" (
    @echo ERROR: can't copy to %target%
    @pause
    @goto :EOF
)
@rd /s /q %tmpdir%\dist 2>NUL
@rd /s /q %tmpdir%\build-Release 2>NUL
@rd /s /q %tmpdir%\build-Debug 2>NUL
@rd /s /q %tmpdir%\dist-Debug 2>NUL
@rd /s /q %tmpdir%\dist-Release 2>NUL

:: -----------------------------------------------

@mkdir %BACKUPDIR% 2>NUL
7z a -x!*.o %BACKUPDIR%\%BACKUPNAME%.7z %tmpdir% 
@if "%ERRORLEVEL%" NEQ "0" (
    @echo ERROR: Can't pack %tmpdir% to %BACKUPDIR%\%BACKUPNAME%.7z
)
@rd /s /q %tmpdir% 2>NUL

:: -----------------------------------------------
@rem pause
@goto :EOF

:: -----------------------------------------------
:GET_CURRENT_DATE
@set dt=%date: =0%
@set dt=%dt:~6,4%%dt:~3,2%%dt:~0,2%
@set tm=%time: =0%
@set tm=%tm:~0,2%%tm:~3,2%%tm:~6,2%
@set DT=%dt%-%tm%
@echo Date/Time stamp: %DT%
@exit /b 0
