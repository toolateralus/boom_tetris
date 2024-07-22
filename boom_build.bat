@echo off

mkdir build
cd build
cmake ..
make -j24
cd ..

mkdir boom_windows
mkdir boom_windows\res

xcopy /s /y res\* boom_windows\res

copy build\boom_tetris.exe boom_windows\boom_tetris

powershell Compress-Archive -Path "boom_windows" -DestinationPath "boom_windows.zip"

if "%~1"=="run" (
  boom_windows\boom_tetris
)

rmdir /s /q boom_windows