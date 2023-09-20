@echo off
rem ----------------------------------------------------------------------------
rem BioDAQ (BDQ)
rem
rem (c) 2012 BTS SpA                                          
rem ----------------------------------------------------------------------------
rem $Id: PostBuild.cmd 2484 2016-11-17 16:33:40Z CRODIE $
rem ----------------------------------------------------------------------------
rem 
rem CONTENTS : AppSampleCppA.exe post-build script
rem
rem Usage: Postbuild [Debug | Release] [Solution directory] [x64]
rem
rem the first parameter indicates the configuration
rem the second parameter indicates the directory of solution
rem the third parameter indicates the platform
rem
rem ----------------------------------------------------------------------------

rem ---------------------------------------
rem ---- ARGUMENTS CHECKING
rem ---------------------------------------

rem ---- Test Configuration Name
if %1 == Debug         goto CONFIGNAME_OK
if %1 == Release       goto CONFIGNAME_OK
echo Error during PostBuild step: Invalid Configuration - %1
goto END
:CONFIGNAME_OK

rem ---------------------------------------
rem ---- CONFIGURATIONS
rem ---------------------------------------

set TARGET_NAME=Cpp_manager

set PROJECT_ROOT=%~2
set TARGET_ROOT=%PROJECT_ROOT%..
rem set IMPORT_ROOT=C:\Users\MADEUC5_1\Documents\BTS BioDAQ SDK 1.2\Bin

set PLATFORM=%3

set IMPORT_NAME1=TdfAccess100
set IMPORT_NAME2=msvcr100
set IMPORT_NAME3=msvcp100
set IMPORT_NAME4=TdfExchange100
set IMPORT_NAME5=TRex100

rem Special case for debug configuration
rem if %1 == Debug set TARGET_NAME=%TARGET_NAME%D
if %1 == Debug set IMPORT_NAME1=%IMPORT_NAME1%D
if %1 == Debug set IMPORT_NAME2=%IMPORT_NAME2%D
if %1 == Debug set IMPORT_NAME3=%IMPORT_NAME3%D
if %1 == Debug set IMPORT_NAME4=%IMPORT_NAME4%D
if %1 == Debug set IMPORT_NAME5=%IMPORT_NAME5%D

rem ---------------------------------------
rem ---- FOLDERS SETUP
rem ---------------------------------------

if %1 == Debug         set CONFIG_FOLDER=Debug
if %1 == Release       set CONFIG_FOLDER=Release

set OUT_FOLDER=Bin\%CONFIG_FOLDER%\%PLATFORM%
set BIN_FOLDER=%TARGET_ROOT%\Build\Bin\%CONFIG_FOLDER%\%PLATFORM%
set IMPORT_FOLDER=%IMPORT_ROOT%\%CONFIG_FOLDER%\%PLATFORM%

if not exist "%BIN_FOLDER%" mkdir "%BIN_FOLDER%"

rem ---------------------------------------
rem ---- ALL CONFIGURATIONS
rem ---------------------------------------

call :FCOPY "%PROJECT_ROOT%\%OUT_FOLDER%\%TARGET_NAME%.exe"					  "%BIN_FOLDER%"

call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME1%.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME1%.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME2%.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME2%.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME3%.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME3%.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME4%.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME4%.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME5%.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME5%.dll" 					          "%OUT_FOLDER%"

call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.core.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.core.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.drivers.dll" 					      "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.drivers.dll" 					      "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.utils.dll" 					          "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\bts.biodaq.utils.dll" 					          "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\FTD2XX_NET.dll" 					              "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\FTD2XX_NET.dll" 					              "%OUT_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\log4net.dll" 					                  "%BIN_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\log4net.dll" 					                  "%OUT_FOLDER%"


goto END

rem ---------------------------------------
rem ---- SUBROUTINES
rem ----

:FCOPY

echo Copying %1 to %2
copy %1 %2 > Nul
if ERRORLEVEL 1 echo Error copying %1 to %2 
goto :EOF 

:END
rem ---------------------------------------
rem                END
rem ---------------------------------------

echo Postbuild done.
