# CMakeList.txt : CMake project for ISATestsGen, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (ISATestsGen "ISATestsGen.cpp" "ISATestsGen.h" "register.h" "register.cpp")

# TODO: Add tests and install targets if needed.
