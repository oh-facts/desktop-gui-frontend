@echo off
setlocal
cd /D "%~dp0"

:: Usage
:: ==============
:: ./build pf    : Builds platform
:: ./build yk    : Builds game
:: ./build clean : Deletes out/ folder

for %%a in (%*) do set "%%a=1"

set common_flags="-std=c++17 -msse4.1 -O0 -fno-rtti -fno-exceptions -Wall -Wno-unused-function -Wno-writable-strings -Wno-comment -g"

if "%cloc%" == "1" cloc --exclude-list-file=.clocignore .\code\
if "%clean%" == "1" rmdir /s /q "out"

if not exist out mkdir out 

if "%pf%" == "1" clang++ "%common_flags%" -luser32 -lkernel32 -lgdi32 -lopengl32 code/main.cpp -o out/pf.exe

if %errorlevel% neq 0 echo platform compilation failed && exit /b

if "%yk%" == "1" clang++ "%common_flags%" code/saoirse.cpp -shared -o out/yk.dll

if %errorlevel% neq 0 echo game compilation failed && exit /b

if "%run%" == "1" start ./out/pf.exe