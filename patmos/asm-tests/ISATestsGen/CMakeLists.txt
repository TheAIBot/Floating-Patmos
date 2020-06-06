# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /fp:strict")
else()
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -msse2 -mfpmath=sse")
endif()

project ("ISATestsGen")

# Include sub-projects.
add_subdirectory ("ISATestsGen")
