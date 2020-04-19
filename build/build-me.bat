@setlocal
@set ADDINST=0
@set ADDRELDBG=0
@rem 20200419 - Change to msvc 16
@REM 20161002 - Change to msvc140 build
@set VCVERS=16
@set VCYEAR=2019

@set GENERATOR=Visual Studio %VCVERS% %VCYEAR%
@REM 20160324 - Change to relative, and use choice
@set TMPPRJ=pedump2
@echo Build %TMPPRJ% project, in 64-bits
@set TMPLOG=bldlog-1.txt
@set BLDDIR=%CD%
@set TMPINST=D:\Projects\3rdParty.x64
@set TMPSRC=..
@if NOT EXIST %TMPSRC%\CMakeLists.txt goto NOCM
@set DOPAUSE=1

@echo Doing build output to %TMPLOG%
@echo Doing build output to %TMPLOG% > %TMPLOG%

@REM call setupqt64

@REM ############################################
@REM NOTE: SPECIAL INSTALL LOCATION
@REM Adjust to suit your environment
@REM ##########################################
@REM set TMPINST=F:\Projects\software.x64
@set TMPOPTS=-DCMAKE_INSTALL_PREFIX=%TMPINST%
@set TMPOPTS=%TMPOPTS% -G "%GENERATOR%" -A x64
@REM set TMPOPTS=%TMPOPTS% -DBUILD_SHARED_LIB:BOOL=OFF

:RPT
@if "%~1x" == "x" goto GOTCMD
@if "%~1x" == "NOPAUSEx" (
    @set DOPAUSE=0
) else (
    @set TMPOPTS=%TMPOPTS% %1
)
@shift
@goto RPT
:GOTCMD

@call chkmsvc %TMPPRJ%

@echo Begin %DATE% %TIME%, output to %TMPLOG%
@echo Begin %DATE% %TIME% >> %TMPLOG%

@echo Doing: 'cmake -S %TMPSRC% %TMPOPTS%'
@echo Doing: 'cmake -S %TMPSRC% %TMPOPTS%' >> %TMPLOG%
@cmake -S %TMPSRC% %TMPOPTS% >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR1

@echo Doing: 'cmake --build . --config debug'
@echo Doing: 'cmake --build . --config debug' >> %TMPLOG%
@cmake --build . --config debug >> %TMPLOG%
@if ERRORLEVEL 1 goto ERR2

@if NOT "%ADDRELDBG%x" == "1x" goto DNRELDBG
@echo Doing: 'cmake --build . --config RelWithDebInfo'
@echo Doing: 'cmake --build . --config RelWithDebInfo' >> %TMPLOG%
@cmake --build . --config RelWithDebInfo >> %TMPLOG%
@if ERRORLEVEL 1 goto ERR22
:DNRELDBG

@echo Doing: 'cmake --build . --config release'
@echo Doing: 'cmake --build . --config release' >> %TMPLOG%
@cmake --build . --config release >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR3
:DNREL

@echo Appears a successful build
@echo Note install location %TMPINST%
@echo.
@if "%ADDINST%" == "1" goto CHKINST
@echo No install configured... set ADDINST=1
@echo Use updexe.bat to copy the EXE to C:\MDOS... 
@echo Above bat requires 'fc4' - https://github.com/geoffmcl/FC4
@echo if you want a compare, and only copy if different...
@echo.
@goto END
:CHKINST
@REM ##############################################
@REM Check if should continue with install
@REM ##############################################
@if "%DOPAUSE%x" == "0x" goto DOINST
@choice /? >nul 2>&1
@if ERRORLEVEL 1 goto NOCHOICE
@choice /D N /T 10 /M "Pausing for 10 seconds. Def=N"
@if ERRORLEVEL 2 goto GOTNO
@goto DOINST
:NOCHOICE
@echo Appears OS does not have the 'choice' command!
@ask *** CONTINUE with install? *** Only y continues
@if ERRORLEVEL 2 goto NOASK
@if ERRORLEVEL 1 goto DOINST
@echo Skipping install to %TMPINST% at this time...
@echo.
@goto END
:NOASK
@echo 'ask' utility not found in path...
@echo.
@echo *** CONTINUE with install? *** Only Ctrl+c aborts...
@echo.
@pause

:DOINST
@echo Proceeding with INSTALL...
@echo.
@REM cmake -P cmake_install.cmake
@echo Doing: 'cmake --build . --config debug --target INSTALL'
@echo Doing: 'cmake --build . --config debug --target INSTALL' >> %TMPLOG%
@cmake --build . --config debug --target INSTALL >> %TMPLOG% 2>&1

@echo Doing: 'cmake --build . --config release --target INSTALL'
@echo Doing: 'cmake --build . --config release --target INSTALL' >> %TMPLOG%
@cmake --build . --config release --target INSTALL >> %TMPLOG% 2>&1

@fa4 " -- " %TMPLOG%

@echo Done build and install of %TMPPRJ%...

@goto END

:GOTNO
@echo.
@echo No install at this time, but there may be an updexe.bat to copy the EXE to c:\MDOS...
@echo.
@goto END

@REM :NOROOT
@REM @echo Can NOT locate %TMPROOT%! *** FIX ME ***
@REM @goto ISERR

:NOCM
@echo Can NOT locate %TMPSRC%\CMakeLists.txt! *** FIX ME ***
@goto ISERR

:ERR1
@echo cmake config, generation error
@goto ISERR

:ERR2
@echo debug build error
@goto ISERR

:ERR22
@echo RelWithDebInfo build error
@goto ISERR

:ERR3
@fa4 "mt.exe : general error c101008d:" %TMPLOG% >nul
@if ERRORLEVEL 1 goto ERR32
:ERR33
@echo release build error
@goto ISERR
:ERR32
@echo Stupid error... trying again...
@echo Doing: 'cmake --build . --config release'
@echo Doing: 'cmake --build . --config release' >> %TMPLOG%
@cmake --build . --config release >> %TMPLOG% 2>&1
@if ERRORLEVEL 1 goto ERR33
@goto DNREL

:ISERR
@endlocal
@exit /b 1

:END
@endlocal
@exit /b 0

@REM eof
