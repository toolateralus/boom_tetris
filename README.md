# Begal Tetris: 
  An attempt to make a very NES Tetris (1989) accurate remake with some bagels and cream cheese.

# Playing the game: 
  The easiest way to play the game is to grab one of the releases. Currently pre-built for linux and windows.

# Building  
## Installing dependencies:
### raylib
  raylib is fetched with cmake automatically.


### Cmake and Make
#### On Linux:
To install CMake and Make on Linux, you can use the package manager of your distribution. For example, on Ubuntu, you can run the following command to install both CMake and Make:

```bash
sudo apt install cmake make
```

#### On Windows:

> Note: using `choco` or `winget` makes this a lot easier.

To install CMake and Make on Windows, you can follow these steps:

1. Download the CMake installer from the official CMake website (https://cmake.org/download/).
2. Run the installer and follow the installation instructions.
3. Download the Make for Windows from the official GNU Make website (http://gnuwin32.sourceforge.net/packages/make.htm).
4. Run the installer and follow the installation instructions.

After installing CMake and Make on Windows, make sure to add their installation directories to the system's PATH environment variable.

### Clang compiler
### On Linux: 
  To install the latest version of clang on Linux, you can obtain clang's repository for your package manager. You need at least `clang 18.1`. You can install it by running the following command:
  
  ```bash
  sudo apt install clang-19
  ```
#### On Windows
  To install clang on Windows, you can go to [llvm's git repository](https://github.com/llvm/llvm-project/releases), find the Windows 64 .exe, and download the installer. Run the installer to complete the installation. You may also need the `MinGW` compiler for gcc, but i'm not certain.

### to build & run:
#### On Linux:
```bash
  mkdir build
  cd build
  cmake ..
  make -j24 # ajdust the -j24 to a reasonable amount for your system. 24 means 24 jobs asynchronously, which can be quite heavy.
  ./boom_tetris
```

#### On Windows: 
```bash
  mkdir build
  cd build
  # this may take a while.
  cmake .. -G "MinGW Makefiles"
  make -j24 # ajdust the -j24 to a reasonable amount for your system. 24 means 24 jobs asynchronously, which can be quite heavy.
  ./boom_tetris

```
