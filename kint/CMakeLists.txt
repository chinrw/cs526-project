cmake_minimum_required(VERSION 3.14)
project(CS526-KINT-kint)

# Set this to a valid LLVM installation dir
set(LT_LLVM_INSTALL_DIR
    ""
    CACHE PATH "LLVM installation directory")

# Add the location of LLVMConfig.cmake to CMake search paths (so that
# find_package can locate it)
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD 17)

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wimplicit-fallthrough -fPIC -g")

ExternalProject_Add(
  z3-repo
  GIT_REPOSITORY "https://github.com/Z3Prover/z3.git"
  GIT_TAG "z3-4.12.1"
  CMAKE_ARGS -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
             -DZ3_ENABLE_EXAMPLE_TARGETS=0
             -GNinja
             -DCMAKE_INSTALL_LIBDIR=lib64
             -DZ3_BUILD_PYTHON_BINDINGS=0
             -DZ3_BUILD_LIBZ3_SHARED=1
             -DZ3_BUILD_TEST_EXECUTABLES=0
             -DZ3_BUILD_EXECUTABLE=0
             -DCMAKE_INSTALL_PREFIX=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/z3
  BUILD_BYPRODUCTS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/z3/lib64/libz3.so")

# ===============================================================================
# 1. ADD THE TARGET
# ===============================================================================

include_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/z3/include)
set(LIBZ3_SHARED_LIB "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/z3/lib64/libz3.so")
add_library(libz3 SHARED IMPORTED)
set_target_properties(libz3 PROPERTIES IMPORTED_LOCATION "${LIBZ3_SHARED_LIB}")

set(CMAKE_INSTALL_RPATH ${LIBZ3_SHARED_LIB})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_llvm_library(
  KINT
  MODULE
  kint.cpp
  kint_function.cpp
  kint_function.h
  kint_module.cpp
  kint_module.h
  smt_solving.cpp
  range.h
  range.cpp
  kint_constant_range.h
  range_helper.cpp
  init_range.cpp
  # List your source files here.
  check_insertion.cpp
  check_insertion.h
  # constraint.cpp
  # constraint.h
  tainted.cpp
  DEPENDS
  z3-repo
  PLUGIN_TOOL
  opt)

# add_dependencies(KINT libz3)
target_link_libraries(KINT PRIVATE libz3)
