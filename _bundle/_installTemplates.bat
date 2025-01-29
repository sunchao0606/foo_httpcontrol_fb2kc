@ECHO off
REM ------------------------------------------------------------------
REM Install bundled templates v.29/01/2025
REM Usage:
REM 	_installTemplates.bat [Number]
REM Key:
REM 	Number		If Number is provided, will install template without
REM					user input
REM ------------------------------------------------------------------
SETLOCAL
SET sourceFolder=templates
SET destinationFolder=..\..\foo_httpcontrol_data
SET hasErrors=false
ECHO ---------------------------------
ECHO ^|	 Templates installation:	^|
ECHO ---------------------------------
ECHO (1) default
ECHO (2) ajquery
ECHO (3) ajquery-xxx
ECHO (4) foobar2000controller
ECHO (5) ALL
ECHO.
IF [%~1]==[] (
	CHOICE /C 12345 /N /M "CHOOSE TEMPLATE TO INSTALL (1-5): "
) ELSE (
	IF [%1] EQU [0] (
		ECHO 9| CHOICE /C 123456789 /N >NUL
	) ELSE (
		ECHO %1| CHOICE /C 123456789 /N /M "CHOOSE TEMPLATE TO INSTALL (1-5): "
	)
)
IF %ERRORLEVEL% EQU 1 GOTO default
IF %ERRORLEVEL% EQU 2 GOTO ajquery
IF %ERRORLEVEL% EQU 3 GOTO ajquery-xxx
IF %ERRORLEVEL% EQU 4 GOTO foobar2000controller
IF %ERRORLEVEL% EQU 5 GOTO all
IF ERRORLEVEL 6 (
	ECHO Option ^(%1^) not recognized.
	GOTO:EOF
)

REM ------------------------------
REM Templates
REM ------------------------------

:default
ECHO.
CALL :copy_folder default
CALL :finish
GOTO:EOF

:ajquery
ECHO.
CALL :copy_folder ajquery
CALL :finish
GOTO:EOF

:ajquery-xxx
ECHO.
ECHO ajquery-xxx template has integration with components which may need to be installed separately.
ECHO Be sure to read its included readme and requirements. (notepad window should open with it)
ECHO.
START "" %sourceFolder%\ajquery-xxx\_readme.txt
CALL :copy_folder ajquery-xxx
CALL :finish
GOTO:EOF

:foobar2000controller
ECHO.
ECHO foobar2000controller template has integration with foobar2000controller android app.
ECHO The app must be installed on an android device to use it. 
ECHO See this component readme for more info.  (browser window should open with it)
ECHO.
START "" readme.html
CALL :copy_folder foobar2000controller
CALL :finish
GOTO:EOF

:all
ECHO.
ECHO ajquery-xxx template has integration with components which may need to be installed separately.
ECHO Be sure to read its included readme and requirements. (notepad window should open with it)
ECHO.
START "" %sourceFolder%\ajquery-xxx\_readme.txt
ECHO foobar2000controller template has integration with foobar2000controller android app.
ECHO The app must be installed on an android device to use it. 
ECHO See this component readme for more info.  (browser window should open with it)
ECHO.
START "" readme.html
CALL :copy_folder *.*
CALL :finish
GOTO:EOF

REM ------------------------------
REM Internals
REM ------------------------------

:check_folder
SET template=%1
IF NOT EXIST %destinationFolder%\%template% MD %destinationFolder%\%template%
GOTO:EOF

:copy_folder
SET template=%1
CALL :check_folder %template%
IF [%1]==[*.*] (
	XCOPY /E /H /Q /R /V /Y %sourceFolder%\%template% %destinationFolder%\
) ELSE (
	XCOPY /E /H /Q /R /V /Y %sourceFolder%\%template% %destinationFolder%\%template%
)
IF ERRORLEVEL 1 (CALL :report_error %template%)
GOTO:EOF

:copy_folders
SET folder=%1
SET folders=%~2
CALL :check_folder %folder%
FOR %%f in (%folders%) do (
	CALL :copy_folder %folder%\%%f
)
GOTO:EOF

:report_error
ECHO.
ECHO ERROR
ECHO Not found: %1
SET hasErrors=true
EXIT /B 1
GOTO:EOF

:report
SET NORMPATH=%~f1
ECHO.
ECHO Templates may be found at %NORMPATH%
EXIT /B
GOTO:EOF

:finish
IF NOT %hasErrors%==true (
	CALL :report %destinationFolder%
)
PAUSE>NUL
IF %hasErrors%==true (EXIT /B 1)
GOTO:EOF