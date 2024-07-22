cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_COMPILER clang++)
project ("boom_tetris")

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Set compiler flags for different build types
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")  # Add debug flags for Clang

set(PROJECT_SOURCES
    rayui.hpp
    score.hpp
    score.cpp
    tetris.hpp
    tetris.cpp
    main.cpp
)


# Add source to this project's executable using the globbed files
add_executable (boom_tetris ${PROJECT_SOURCES})
file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET boom_tetris PROPERTY CXX_STANDARD 20)
endif()

find_package(raylib QUIET) 
if (NOT raylib_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) 
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

target_compile_features(boom_tetris PRIVATE cxx_std_23)

target_link_libraries(boom_tetris PRIVATE raylib)
target_compile_options(boom_tetris PRIVATE -Os) 
target_link_options(boom_tetris PRIVATE -s)
target_link_options(boom_tetris PRIVATE -flto)
