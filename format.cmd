@echo off
echo Formatting with config...
for /R ".\" %%f in (*.cpp, *.h, *.c, *.hpp) do (
	echo Formatting "%%f"
	clang-format -i -style=file "%%f"
)
echo Done!