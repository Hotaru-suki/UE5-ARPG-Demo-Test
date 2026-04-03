@echo off
setlocal

set "PROJECT=%~dp0ActionRPG.uproject"
set "MODE=%~1"
set "LABEL=%~2"

if "%MODE%"=="" set "MODE=autoplay"
if "%LABEL%"=="" set "LABEL=%MODE%"

if not exist "%PROJECT%" (
    echo [ERROR] Project file not found: %PROJECT%
    exit /b 1
)

if "%UNREAL_EDITOR_EXE%"=="" (
    set "UNREAL_EDITOR_EXE=D:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
)

if not exist "%UNREAL_EDITOR_EXE%" (
    echo [ERROR] UnrealEditor.exe not found: %UNREAL_EDITOR_EXE%
    echo [HINT] Set UNREAL_EDITOR_EXE before running this script.
    exit /b 1
)

set "COMMON_STATS=t.MaxFPS 120|r.VSync 0|stat fps|stat unit|stat unitgraph|stat game|stat ai|stat navmesh|stat anim|stat memory|stat slate"
set "COMMON_PERF=-RPGPerfInterval=0.5 -RPGPerfScreen"

if /I "%MODE%"=="perf" (
    echo [INFO] Launching standalone performance monitor: %LABEL%
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -game -RPGPerfMonitor -RPGPerfLabel=%LABEL% %COMMON_PERF%
    exit /b 0
)

if /I "%MODE%"=="smoke" (
    echo [INFO] Launching standalone smoke autotest: %LABEL%
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -game -RPGAutoTest -RPGAutoTestLabel=%LABEL% -RPGAutoTestDuration=30 -RPGAutoTestCommands="%COMMON_STATS%" -RPGAutoTestExit %COMMON_PERF%
    exit /b 0
)

if /I "%MODE%"=="perfcheck" (
    echo [INFO] Launching standalone performance check: %LABEL%
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -game -RPGAutoTest -RPGAutoTestAutoplay -RPGAutoTestLabel=%LABEL% -RPGAutoTestDuration=60 -RPGAutoTestDelay=1 -RPGAutoTestAutoplayDelay=1.5 -RPGAutoTestAutoplayRetryInterval=1 -RPGAutoTestMinAvgFPS=45 -RPGAutoTestCommands="%COMMON_STATS%" -RPGAutoTestExit %COMMON_PERF%
    exit /b 0
)

if /I "%MODE%"=="pie" (
    echo [INFO] Launching editor in PIE autotest-ready mode: %LABEL%
    echo [INFO] After the editor opens, manually click Play. C++ autotest will try to trigger BackSpace autoplay after PIE starts.
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -RPGAutoTest -RPGAutoTestAutoplay -RPGAutoTestLabel=%LABEL% -RPGAutoTestDuration=60 -RPGAutoTestDelay=1 -RPGAutoTestAutoplayDelay=1.5 -RPGAutoTestAutoplayRetryInterval=1 -RPGAutoTestCommands="%COMMON_STATS%" %COMMON_PERF%
    exit /b 0
)

if /I "%MODE%"=="pieperf" (
    echo [INFO] Launching editor in PIE performance monitor mode: %LABEL%
    echo [INFO] After the editor opens, manually click Play to create the PIE world.
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -RPGPerfMonitor -RPGPerfLabel=%LABEL% %COMMON_PERF%
    exit /b 0
)

if /I "%MODE%"=="autoplay" (
    echo [INFO] Launching standalone autoplay autotest: %LABEL%
    start "" "%UNREAL_EDITOR_EXE%" "%PROJECT%" -game -RPGAutoTest -RPGAutoTestAutoplay -RPGAutoTestLabel=%LABEL% -RPGAutoTestDuration=60 -RPGAutoTestDelay=1 -RPGAutoTestAutoplayDelay=1.5 -RPGAutoTestAutoplayRetryInterval=1 -RPGAutoTestCommands="%COMMON_STATS%" -RPGAutoTestExit %COMMON_PERF%
    exit /b 0
)

echo [ERROR] Unknown mode: %MODE%
echo [USAGE] run_rpg_automation.bat [autoplay^|perfcheck^|smoke^|perf^|pie^|pieperf] [label]
exit /b 1
