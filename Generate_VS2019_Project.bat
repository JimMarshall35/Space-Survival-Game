@echo off
call Clean.bat
for %%I in (.) do set CurrDirName=%%~nxI
cmake -G "Visual Studio 17 2019" -S "vendor\glfw" -B "vendor\glfw\BUILD"
call premake\premake5.exe vs2019 %CurrDirName%
rmdir /s /q x64