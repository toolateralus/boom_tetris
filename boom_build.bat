@echo off

mkdir build
cd build
cmake ..
make -j24
cd ..

mkdir boom_linux
mkdir boom_linux\res

xcopy /s /y res\* boom_linux\res

copy build\boom_tetris boom_linux\boom_tetris

powershell Compress-Archive -Path "boom_linux" -DestinationPath "boom_linux.zip"

if "%~1"=="run" (
  boom_linux\boom_tetris
)

rmdir /s /q boom_linux