cmake_minimum_required (VERSION 3.8)

project (sstv LANGUAGES C CXX)

# Version
set (VERSION_MAJOR 0)
set (VERSION_MINOR 9)
set (VERSION_PATCH 0)

# Limtis
set (DEFAULT_ENCODER_CONTEXT_COUNT 4)

# Compiler setup
set(CMAKE_C_STANDARD 99)
add_compile_options (-Wall -Wextra -pedantic)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition")

# Options
option (BUILD_TOOLS "build sstv-encode and sstv-decode tools" ON)

# Directory setup
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Configuration file
configure_file (
  "${SRC_DIR}/libsstv.template.h"
  "${INCLUDE_DIR}/libsstv.h"
)

# Source files
set (LIB_SOURCES
  "${SRC_DIR}/sstv.c"
  "${SRC_DIR}/encoder.c"
  "${SRC_DIR}/luts.c"
)

set (ENCODE_TOOL_SOURCES
  "${SRC_DIR}/tools/sstv-encode.cpp"
)

# Dependencies
find_library(GLOG_LIBRARY glog)
find_package(gflags REQUIRED)
find_package(ImageMagick COMPONENTS Magick++)

# Library (C compiler)
add_library (${PROJECT_NAME} SHARED ${LIB_SOURCES})
set_property (TARGET ${PROJECT_NAME} PROPERTY LINKER_LANGUAGE C)
target_include_directories (${PROJECT_NAME} PUBLIC "${SRC_DIR}" PUBLIC "${INCLUDE_DIR}")

# Tools (C++ compiler)
if (BUILD_TOOLS)
  add_executable (${PROJECT_NAME}-encode ${ENCODE_TOOL_SOURCES})
  set_property (TARGET ${PROJECT_NAME}-encode PROPERTY LINKER_LANGUAGE CXX)
  set_property (TARGET ${PROJECT_NAME}-encode PROPERTY CXX_STANDARD 14)
  target_include_directories(${PROJECT_NAME}-encode PUBLIC "${SRC_DIR}/tools" PUBLIC "${INCLUDE_DIR}" PUBLIC "${ImageMagick_INCLUDE_DIRS}")
  target_link_libraries (${PROJECT_NAME}-encode ${PROJECT_NAME} glog gflags sndfile ${ImageMagick_LIBRARIES})
endif (BUILD_TOOLS)
