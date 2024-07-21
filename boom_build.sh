mkdir build
cd build
cmake ..
make -j24
cd ..

mkdir boom_linux
mkdir boom_linux/res

cp res/* boom_linux/res

cp build/boom_tetris boom_linux/boom_tetris

zip -r boom_linux.zip boom_linux

if [ "$1" == "run" ]; then
   ./boom_linux/boom_tetris 
fi

rm -r boom_linux