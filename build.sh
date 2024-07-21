mkdir build
cd build
cmake ..
make -j24
cd ..
rm tetris/res/*
cp res/* tetris/res



if [ $# -eq 1 ]; then
   ./tetris/boom_tetris_linux 
fi