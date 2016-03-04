@echo off

cd %~dp0

if /i "%1"=="help" goto help
if /i "%1"=="--help" goto help
if /i "%1"=="-help" goto help
if /i "%1"=="/help" goto help
if /i "%1"=="?" goto help
if /i "%1"=="-?" goto help
if /i "%1"=="--?" goto help
if /i "%1"=="/?" goto help

@rem Process arguments.
set config=Release
set msiplatform=x86
set target=Build
@rem ia32 for 32 bit, x64 for 64 bit
set target_arch=
set debug_arg=
set nosnapshot_arg=
set noprojgen=
set nobuild=
set nosign=
set nosnapshot=
set test=
set test_args=
set msi=
set licensertf=
set upload=
set jslint=
set buildnodeweak=
set noetw=0
set noetw_arg=
set noetw_msi_arg=
set noperfctr=
set noperfctr_arg=
set noperfctr_msi_arg=
set engine_=
set static_library=
set compress_internals=
set c_platform=
set wincore=
set no_asm=
set leveldown=

:next-arg
if "%1"=="" goto args-done
if /i "%1"=="debug"         set config=Debug&goto arg-ok
if /i "%1"=="release"       set config=Release&goto arg-ok
if /i "%1"=="clean"         set target=Clean&goto arg-ok
if /i "%1"=="ia32"          set c_platform="/p:Platform=Win32"&set target_arch=ia32&goto arg-ok
if /i "%1"=="x86"           set c_platform="/p:Platform=Win32"&set target_arch=ia32&goto arg-ok
if /i "%1"=="x64"           set target_arch=x64&goto arg-ok
if /i "%1"=="arm"           set c_platform="/p:Platform=ARM"&set target_arch=arm&set no_asm=--openssl-no-asm&goto arg-ok
if /i "%1"=="noprojgen"     set noprojgen=1&goto arg-ok
if /i "%1"=="nobuild"       set nobuild=1&goto arg-ok
if /i "%1"=="nosign"        set nosign=1&goto arg-ok
if /i "%1"=="nosnapshot"    set nosnapshot=1&goto arg-ok
if /i "%1"=="noetw"         set noetw=1&goto arg-ok
if /i "%1"=="noperfctr"     set noperfctr=1&goto arg-ok
if /i "%1"=="licensertf"    set licensertf=1&goto arg-ok
if /i "%1"=="test-uv"       set test=test-uv&goto arg-ok
if /i "%1"=="test-internet" set test=test-internet&goto arg-ok
if /i "%1"=="test-pummel"   set test=test-pummel&goto arg-ok
if /i "%1"=="test-simple"   set test=test-simple&goto arg-ok
if /i "%1"=="test-message"  set test=test-message&goto arg-ok
if /i "%1"=="test-gc"       set test=test-gc&set buildnodeweak=1&goto arg-ok
if /i "%1"=="test-all"      set test=test-all&set buildnodeweak=1&goto arg-ok
if /i "%1"=="test"          set test=test&goto arg-ok
if /i "%1"=="msi"           set msi=1&set licensertf=1&goto arg-ok
if /i "%1"=="upload"        set upload=1&goto arg-ok
if /i "%1"=="jslint"        set jslint=1&goto arg-ok
if /i "%1"=="--win-onecore" set wincore=--win-onecore&set no_asm=--openssl-no-asm&goto arg-ok
if /i "%1"=="--static-library" set static_library=--static-library&goto arg-ok
if /i "%1"=="--shared-library" set static_library=--shared-library&goto arg-ok
if /i "%1"=="--engine-mozilla" set engine_=--engine-mozilla&goto arg-ok
if /i "%1"=="--engine-chakra" set engine_=--engine-chakra&set WindowsTargetPlatformVersion=10.0.10586.0&goto arg-ok
if /i "%1"=="--compress-internals" set compress_internals=--compress-internals&goto arg-ok

echo Warning: ignoring invalid command line option `%1`.

:arg-ok
:arg-ok
shift
goto next-arg

:args-done
if defined upload goto upload
if defined jslint goto jslint

if "%config%"=="Debug" set debug_arg=--debug
if "%target_arch%"=="x64" set msiplatform=x64
if defined nosnapshot set nosnapshot_arg=--without-snapshot
if defined noetw set noetw_arg=--without-etw& set noetw_msi_arg=/p:NoETW=1
if defined noperfctr set noperfctr_arg=--without-perfctr& set noperfctr_msi_arg=/p:NoPerfCtr=1

if defined NIGHTLY set TAG=nightly-%NIGHTLY%

@rem Set environment for msbuild

@rem Look for Visual Studio 2015
if not defined VS140COMNTOOLS goto vc-set-2013
if not exist "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2013
if "%VCVARS_VER%" NEQ "140" (
  call "%VS140COMNTOOLS%\..\..\vc\vcvarsall.bat"
  SET VCVARS_VER=140
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2015
goto msbuild-found

:vc-set-2013
@rem Look for Visual Studio 2013
if not defined VS120COMNTOOLS goto vc-set-2012
if not exist "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2012
if "%VCVARS_VER%" NEQ "120" (
  call "%VS120COMNTOOLS%\..\..\vc\vcvarsall.bat"
  SET VCVARS_VER=120
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2013
goto msbuild-found

:vc-set-2012
@rem Look for Visual Studio 2012
if not defined VS110COMNTOOLS goto vc-set-2010
if not exist "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat" goto vc-set-2010
if "%VCVARS_VER%" NEQ "110" (
  call "%VS110COMNTOOLS%\..\..\vc\vcvarsall.bat"
  SET VCVARS_VER=110
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2012
goto msbuild-found

:vc-set-2010
if not defined VS100COMNTOOLS goto msbuild-not-found
if not exist "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat" goto msbuild-not-found
if "%VCVARS_VER%" NEQ "100" (
  call "%VS100COMNTOOLS%\..\..\vc\vcvarsall.bat"
  SET VCVARS_VER=100
)
if not defined VCINSTALLDIR goto msbuild-not-found
set GYP_MSVS_VERSION=2010
goto msbuild-found

:msbuild-not-found
echo Failed to find Visual Studio installation.
goto exit

:msbuild-found

:project-gen
@rem Skip project generation if requested.
if defined noprojgen goto msbuild

@rem Generate the VS project.
SETLOCAL
  call :getpythonversion
  if errorlevel 1 goto exit

  python configure %debug_arg% %nosnapshot_arg% %noetw_arg% %noperfctr_arg% %no_asm% --dest-cpu=%target_arch% --tag=%TAG% %leveldown% %wincore% %static_library% %engine_% %compress_internals%
  if errorlevel 1 goto create-msvs-files-failed
  if not exist jx.sln goto create-msvs-files-failed
  echo Project files generated.
ENDLOCAL

:msbuild
@rem Build the sln with msbuild.
msbuild jx.sln /m /t:%target% /p:Configuration="%config%" %c_platform% /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
goto exit

:sign
@rem Skip signing if the `nosign` option was specified.
if defined nosign goto licensertf

signtool sign /a Release\jx.exe

:licensertf
@rem Skip license.rtf generation if not requested.
if not defined licensertf goto msi

%config%\jx tools\license2rtf.js < LICENSE > %config%\license.rtf
if errorlevel 1 echo Failed to generate license.rtf&goto exit

:msi
@rem Skip msi generation if not requested
if not defined msi goto run
call :getnodeversion

if not defined NIGHTLY goto msibuild
set NODE_VERSION=%NODE_VERSION%.%NIGHTLY%

:msibuild
echo Building node-%NODE_VERSION%
msbuild "%~dp0tools\msvs\msi\nodemsi.sln" /m /t:Clean,Build /p:Configuration=%config% /p:Platform=%msiplatform% /p:NodeVersion=%NODE_VERSION% %noetw_msi_arg% %noperfctr_msi_arg% /clp:NoSummary;NoItemAndPropertyList;Verbosity=minimal /nologo
if errorlevel 1 goto exit

if defined nosign goto run
signtool sign /a Release\node-v%NODE_VERSION%-%msiplatform%.msi

:run
@rem Run tests if requested.
if "%test%"=="" goto exit

if "%config%"=="Debug" set test_args=--mode=debug
if "%config%"=="Release" set test_args=--mode=release

if "%test%"=="test" set test_args=%test_args% simple message
if "%test%"=="test-internet" set test_args=%test_args% internet
if "%test%"=="test-pummel" set test_args=%test_args% pummel
if "%test%"=="test-simple" set test_args=%test_args% simple
if "%test%"=="test-message" set test_args=%test_args% message
if "%test%"=="test-gc" set test_args=%test_args% gc
if "%test%"=="test-all" set test_args=%test_args%

:build-node-weak
@rem Build node-weak if required
if "%buildnodeweak%"=="" goto run-tests
"%config%\jx" deps\npm\node_modules\node-gyp\bin\node-gyp rebuild --directory="%~dp0test\gc\node_modules\weak" --nodedir="%~dp0."
if errorlevel 1 goto build-node-weak-failed
goto run-tests

:build-node-weak-failed
echo Failed to build node-weak.
goto exit

:run-tests
echo running 'python tools/test.py %test_args%'
python tools/test.py %test_args%
if "%test%"=="test" goto jslint
goto exit

:create-msvs-files-failed
echo Failed to create vc project files. 
goto exit

:upload
echo not implemented

:jslint
echo running jslint
set PYTHONPATH=tools/closure_linter/
python tools/closure_linter/closure_linter/gjslint.py --unix_mode --strict --nojsdoc -r lib/ -r src/ --exclude_files lib/punycode.js
goto exit

:help
echo vcbuild.bat [debug/release] [msi] [test-all/test-uv/test-internet/test-pummel/test-simple/test-message] [clean] [noprojgen] [nobuild] [nosign] [x86/x64]
echo Examples:
echo   vcbuild.bat                : builds release build
echo   vcbuild.bat debug          : builds debug build
echo   vcbuild.bat release msi    : builds release build and MSI installer package
echo   vcbuild.bat test           : builds debug build and runs tests
goto exit

:exit
goto :EOF

rem ***************
rem   Subroutines
rem ***************

:getnodeversion
set NODE_VERSION=
for /F "usebackq tokens=*" %%i in (`python "%~dp0tools\getnodeversion.py"`) do set NODE_VERSION=%%i
if not defined NODE_VERSION echo Cannot determine current version of node.js & exit /b 1
goto :EOF

:getpythonversion
set PYTHON_VERSION=
for /F "usebackq tokens=*" %%i in (`python "%~dp0tools\getpythonversion.py"`) do set PYTHON_VERSION=%%i
if not defined PYTHON_VERSION echo Cannot determine current version of Python & exit /b 1
set version_ok=0
if "%PYTHON_VERSION:~0,3%"=="2.6" set version_ok=1
if "%PYTHON_VERSION:~0,3%"=="2.7" set version_ok=1
if "%version_ok%"=="0" echo You need Python 2.6 or 2.7 for the script to run. Currently installed version: %python_version% & exit /b 1
goto :EOF

