cmake_minimum_required(VERSION 3.10)
project (j3d)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
set(CMAKE_C_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_CXX_FLAGS_DEBUG "/W4 /MP /GF /RTCu /Od /MDd /Zi")
set(CMAKE_C_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /MP /GF /O2 /Ob2 /Oi /Ot /MD /Zi /DNDEBUG")
endif (WIN32)

option(BUILD_SHARED_LIBS OFF)

add_subdirectory(jtk)
add_subdirectory(SDL2)
add_subdirectory(j3d)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_target_properties (jtk PROPERTIES FOLDER jtk)
set_target_properties (jtk.tests PROPERTIES FOLDER jtk)
set_target_properties (jtk.static.tests PROPERTIES FOLDER jtk)

set_target_properties (SDL2-static PROPERTIES FOLDER SDL2)
set_target_properties (SDL2main PROPERTIES FOLDER SDL2)
set_target_properties (uninstall PROPERTIES FOLDER SDL2)

