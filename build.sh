mkdir build
cd build
cmake ..
make -j24
cd ..
rm tetris/res/*
cp res/* tetris/res
cp build/boom_tetris tetris/boom_tetris_linux


if [ $# -eq 1 ]; then
   ./tetris/boom_tetris_linux 
fi