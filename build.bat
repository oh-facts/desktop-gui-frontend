@echo off

:: Usage
:: ==============
:: ./build app   : Builds exe and puts inside of out/
:: ./build clean : Deletes out/ folder

:: You can pass multiple args to do multiple things
:: ==============

:: todo(mizuho): Get current directory of the script and use that so I can 
:: call the script from anywhere.

for %%a in (%*) do set "%%a=1"

set common_flags="-std=c++17 -msse4.1 -O0 -fno-rtti -fno-exceptions -Wall -Wno-unused-function -Wno-writable-strings -Wno-comment -g"

if "%cloc%" == "1" (
    cloc --exclude-list-file=.clocignore .\code\
)

if "%clean%" == "1" (
    rmdir /s /q "out"
)

if "%compile%" == "1" (
    if not exist "out" (
      mkdir "out"
   )

  clang++ "%common_flags%" -luser32 -lkernel32 -lgdi32 code/main.cpp -o out/yk.exe
)

if %errorlevel% neq 0 (
  echo platform compilation failed
  exit /b
)

if "%game%" == "1" (
  if not exist "out" (
      mkdir "out"
   )
  clang++ "%common_flags%" code/saoirse.cpp -shared -o out/yk.dll
)

if %errorlevel% neq 0 (
  echo game compilation failed
  exit /b
)

if "%run%" == "1" (
  start ./out/yk.exe  
)