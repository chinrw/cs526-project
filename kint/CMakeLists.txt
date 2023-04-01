cmake_minimum_required(VERSION 3.14)
project(CS526-KINT-kint)

# ===============================================================================
# 1. LOAD LLVM CONFIGURATION
# ===============================================================================
# Set this to a valid LLVM installation dir
set(LT_LLVM_INSTALL_DIR
    ""
    CACHE PATH "LLVM installation directory")

# Add the location of LLVMConfig.cmake to CMake search paths (so that
# find_package can locate it)
list(APPEND CMAKE_PREFIX_PATH "${LT_LLVM_INSTALL_DIR}/lib/cmake/llvm/")

# HelloWorld includes headers from LLVM - update the include paths accordingly
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

# ===============================================================================
# 1. LLVM-TUTOR BUILD CONFIGURATION
# ===============================================================================
# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD
    17
    CACHE STRING "")

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wimplicit-fallthrough -fPIC -g")

# ===============================================================================
# 1. ADD THE TARGET
# ===============================================================================
add_library(KINT MODULE
    # List your source files here.
    CheckInsertion.cpp
    CompilerAttributes.h
    Constraints.cpp
    Constraints.h
    SMTQuery.cpp
    SMTSolver.h
)

# Allow undefined symbols in shared objects on Darwin (this is the default
# behaviour on Linux)
target_link_libraries(HelloWorld
                      "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
