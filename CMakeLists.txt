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

# Library (C compiler)
add_library (${PROJECT_NAME}_shared SHARED ${LIB_SOURCES})
add_library (${PROJECT_NAME}_static STATIC ${LIB_SOURCES})

set_target_properties (${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PUBLIC_HEADER ${INCLUDE_DIR}/libsstv.h LINKER_LANGUAGE C)
set_target_properties (${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PUBLIC_HEADER ${INCLUDE_DIR}/libsstv.h LINKER_LANGUAGE C)

target_compile_options (${PROJECT_NAME}_static PRIVATE -nostdlib)

target_include_directories (${PROJECT_NAME}_shared PUBLIC "${SRC_DIR}" PUBLIC "${INCLUDE_DIR}")
target_include_directories (${PROJECT_NAME}_static PUBLIC "${SRC_DIR}" PUBLIC "${INCLUDE_DIR}")

install (TARGETS ${PROJECT_NAME}_shared)
install (TARGETS ${PROJECT_NAME}_static)

# Tools (C++ compiler)
if (BUILD_TOOLS)
    # Dependencies
    add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=16)
    add_definitions(-DMAGICKCORE_HDRI_ENABLE=0)
    find_library(SNDFILE sndfile)
    find_package(ImageMagick COMPONENTS Magick++)

    # Target
    add_executable (${PROJECT_NAME}-encode ${ENCODE_TOOL_SOURCES})
    set_property (TARGET ${PROJECT_NAME}-encode PROPERTY LINKER_LANGUAGE CXX)
    set_property (TARGET ${PROJECT_NAME}-encode PROPERTY CXX_STANDARD 17)
    target_include_directories(${PROJECT_NAME}-encode PUBLIC "${SRC_DIR}/tools" PUBLIC "${INCLUDE_DIR}" PUBLIC "${ImageMagick_INCLUDE_DIRS}")
    target_link_libraries (${PROJECT_NAME}-encode ${PROJECT_NAME}_shared ${SNDFILE} ${ImageMagick_LIBRARIES})
    install (TARGETS ${PROJECT_NAME}-encode)
endif (BUILD_TOOLS)
