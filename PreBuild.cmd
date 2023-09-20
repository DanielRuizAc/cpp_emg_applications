@echo off
rem ----------------------------------------------------------------------------
rem BioDAQ (BDQ)
rem
rem (c) 2012 BTS SpA                                          
rem ----------------------------------------------------------------------------
rem $Id: PreBuild.cmd 2480 2016-11-17 16:14:25Z CRODIE $
rem ----------------------------------------------------------------------------
rem 
rem CONTENTS : AppSampleCppA.exe pre-build script
rem
rem Usage: PreBuild [Debug | Release] [Project directory] [x64]
rem
rem the first parameter indicates the configuration
rem the second parameter indicates the directory of project
rem the third parameter indicates the platform
rem
rem ----------------------------------------------------------------------------

rem ---------------------------------------
rem ---- ARGUMENTS CHECKING
rem ---------------------------------------

rem ---- Test Configuration Name
if %1 == Debug         goto CONFIGNAME_OK
if %1 == Release       goto CONFIGNAME_OK
echo Error during PreBuild step: Invalid Configuration - %1
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

set IMPORT_NAME1=bts.biodaq.drivers
set IMPORT_NAME2=bts.biodaq.core

rem Special case for debug configuration
rem if %1 == Debug set TARGET_NAME=%TARGET_NAME%D
rem if %1 == Debug set IMPORT_NAME1=%IMPORT_NAME1%D
rem if %1 == Debug set IMPORT_NAME2=%IMPORT_NAME2%D

rem ---------------------------------------
rem ---- FOLDERS SETUP
rem ---------------------------------------

if %1 == Debug         set CONFIG_FOLDER=Debug
if %1 == Release       set CONFIG_FOLDER=Release

set SOURCE_FOLDER=%PROJECT_ROOT%\Source
set IMPORT_FOLDER=%IMPORT_ROOT%\%CONFIG_FOLDER%\%PLATFORM%
set REGASM_FOLDER="C:\Windows\Microsoft.NET\Framework64\v4.0.30319\"

if not exist "%BIN_FOLDER%" mkdir "%BIN_FOLDER%"

rem ---------------------------------------
rem ---- ALL CONFIGURATIONS
rem ---------------------------------------

echo Registering BioDaq libraries %IMPORT_NAME1%.dll and %IMPORT_NAME2%.dll
%REGASM_FOLDER%RegAsm "%IMPORT_FOLDER%\%IMPORT_NAME1%.dll" /tlb: "%IMPORT_FOLDER%\%IMPORT_NAME1%.tlb"
%REGASM_FOLDER%RegAsm "%IMPORT_FOLDER%\%IMPORT_NAME2%.dll" /tlb: "%IMPORT_FOLDER%\%IMPORT_NAME2%.tlb"

echo Copying BioDaq type libraries %IMPORT_NAME1%.tlb and %IMPORT_NAME2%.tlb
call :FCOPY "%IMPORT_FOLDER%\mscorlib.tlb"            "%SOURCE_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME1%.tlb"      "%SOURCE_FOLDER%"
call :FCOPY "%IMPORT_FOLDER%\%IMPORT_NAME2%.tlb"      "%SOURCE_FOLDER%"

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

echo PreBuild done.
