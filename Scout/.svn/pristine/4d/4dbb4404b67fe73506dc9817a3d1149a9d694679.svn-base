@ECHO off

setlocal

REM initialize defaults

set BOOST_DIR=..\boost_1_41_0
set LLVM_DIR=..\llvm
set LLVM_LIB_DIR=

REM extract command line variables

:Loop
  IF [%1] == [] GOTO FinishLoop

	IF [%1] == [-boost_dir] SET BOOST_DIR=%~f2
	IF [%1] == [-llvm_dir] SET LLVM_DIR=%~f2
	IF [%1] == [-llvm_lib_dir] SET LLVM_LIB_DIR=%~f2

	SHIFT /1
	SHIFT /1
	GOTO Loop

:FinishLoop


REM Test boost configuration
IF EXIST %BOOST_DIR%\NUL GOTO BoostLib
ECHO "Boost root directory (boost_dir) missing." 
GOTO Usage

:BoostLib
cd %BOOST_DIR%
SET BOOST_DIR=%cd%
cd %~p0

REM Test LLVM configuration
:LLVMTest

IF EXIST %LLVM_DIR%\NUL GOTO LLVMLib
ECHO "LLVM root directory (llvm_dir) missing."
GOTO Usage

:LLVMLib
cd %LLVM_DIR%
SET LLVM_DIR=%cd%
cd %~p0

IF [%LLVM_LIB_DIR%] == [] (
  IF EXIST %LLVM_DIR%\lib\NUL GOTO Output 
  ECHO "LLVM library directory (llvm_lib_dir) missing." 
  GOTO Usage
)

IF EXIST %LLVM_LIB_DIR%\NUL GOTO LLExpand
ECHO "LLVM library directory (llvm_lib_dir) missing."
GOTO Usage

:LLExpand
cd %LLVM_LIB_DIR%
SET LLVM_LIB_DIR=%cd%
cd %~p0

:Output
ECHO #Scout configuration for qmake and make > Scout.config
ECHO.>> Scout.config
ECHO BOOST_DIR = %BOOST_DIR% >> Scout.config
ECHO.>> Scout.config
ECHO LLVM_DIR = %LLVM_DIR% >> Scout.config
IF [%LLVM_LIB_DIR%] == [] GOTO EndCo
ECHO LLVM_LIB_DIR = %LLVM_LIB_DIR% >> Scout.config

:Endco
ECHO.>> Scout.config
ECHO #End of configuration >> Scout.config
ECHO.>> Scout.config


ECHO #Scout configuration for cmake > clangAddons\CMakeLists.config
ECHO.>> clangAddons\CMakeLists.config
ECHO set(BOOST_DIR %BOOST_DIR:\=/%) >> clangAddons\CMakeLists.config
ECHO.>> clangAddons\CMakeLists.config
ECHO set(LLVM_DIR %LLVM_DIR:\=/%) >> clangAddons\CMakeLists.config
IF [%LLVM_LIB_DIR%] == [] GOTO CMakeEnd
ECHO set(LLVM_LIB_DIR %LLVM_LIB_DIR:\=/%) >> clangAddons\CMakeLists.config

:CMakeEnd
ECHO.>> clangAddons\CMakeLists.config
ECHO #End of configuration >> clangAddons\CMakeLists.config
ECHO.>> clangAddons\CMakeLists.config

GOTO Finish



:Usage

ECHO Usage:
ECHO configure.bat [-boost_dir=PATH] [-llvm_dir=PATH] 
ECHO               [-llvm_lib_dir=PATH]
ECHO default for boost_dir is ..\boost_1_41_0
ECHO default for llvm_dir is ..\llvm
ECHO default for llvm_lib_dir is llvm_dir\lib\(Debug^|Release) 
ECHO all PATHs may be absolute or relative

:Finish
endlocal

