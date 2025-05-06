@echo off
setlocal enabledelayedexpansion

echo Searching for clang-format...
where clang-format >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: clang-format not found in PATH. Install LLVM or add it to PATH.
    exit /b 1
)

echo Checking for .clang-format...
if not exist ".clang-format" (
    echo Error: .clang-format config file not found in the current directory.
    exit /b 1
)

echo Formatting files with .clang-format...
set count=0
for /R ".\" %%f in (*.cpp, *.h, *.c, *.hpp) do (
    echo "%%f" | findstr /i "\\build\\ \\.vs\\ \\.git\\ \\.github\\" >nul
    if !errorlevel! equ 1 (
        echo Formatting "%%f"
        clang-format -i -style=file "%%f"
        set /a count+=1
    )
)

echo Done. Processed !count! files.