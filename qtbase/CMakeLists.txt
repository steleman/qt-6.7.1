# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Need an explicit call at the top level. This is the absolute minimum version
# needed to configure the project with any combination of enabled features.
# The call to qt_build_repo_begin() will upgrade policies further.
#
# The absolute minimum version for building Qt is 3.16 because for metatype.json generation, we
# depend on being able to find the location of json files created by AUTOMOC/moc.
cmake_minimum_required(VERSION 3.16)

# Get the repo version and CMake policy details
include(.cmake.conf)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtBaseHelpers.cmake)

qt_internal_check_if_path_has_symlinks("${CMAKE_BINARY_DIR}")

# This needs to be called before the first project() call.
qt_internal_qtbase_run_autodetect()

# This call will load any provided cmake toolchain file.
project(QtBase
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Base Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C ASM
)

set(QT_BUILD_EXTRA_IDE_FILE_PATTERNS bin/* libexec/*)

execute_process(COMMAND uname -m OUTPUT_VARIABLE OS_UNAME_MACHINE)
string(STRIP ${OS_UNAME_MACHINE} OS_UNAME_MACHINE)
execute_process(COMMAND uname -s OUTPUT_VARIABLE OS_UNAME_SYSTEM)
string(STRIP ${OS_UNAME_SYSTEM} OS_UNAME_SYSTEM)

if ("${OS_UNAME_SYSTEM}" STREQUAL "Darwin")
  set(CC "/usr/bin/clang")
  set(CXX "/usr/bin/clang++")
endif()

if ("${OS_UNAME_SYSTEM}" STREQUAL "Linux")
  set(CC "/usr/bin/gcc")
  set(CXX "/usr/bin/g++")
endif()

set(CPPFLAGS "-D_GNU_SOURCE -D_XOPEN_SOURCE=700 -D_REENTRANT")
set(CPPFLAGS "${CPPFLAGS} -DHAVE_PTHREAD -DHAVE_SCHED -DHAVE_SCHED_YIELD")
set(CFLAGS "-g -O2 -std=c11 ${CPPFLAGS} -pthread -Wall -Wextra -Wpedantic")
set(CFLAGS "${CFLAGS} -fPIC -ffunction-sections -fdata-sections")
set(CFLAGS "${CFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer")
set(CXXFLAGS "-g -O2 -std=c++17 ${CPPFLAGS} -pthread -Wall -Wextra -Wpedantic")
set(CXXFLAGS "${CXXFLAGS} -fPIC -ffunction-sections -fdata-sections")
set(CXXFLAGS "${CXXFLAGS} -fno-strict-aliasing -fno-omit-frame-pointer")

set(CFLAGS_DEBUG "-g3 -O0 -std=c11 ${CPPFLAGS} -pthread -Wall -Wextra -Wpedantic")
set(CFLAGS_DEBUG "${CFLAGS_DEBUG} -fPIC -ffunction-sections -fdata-sections")
set(CFLAGS_DEBUG "${CFLAGS_DEBUG} -fno-strict-aliasing -fno-omit-frame-pointer")
set(CXXFLAGS_DEBUG "-g3 -O0 -std=c++17 ${CPPFLAGS} -pthread -Wall -Wextra -Wpedantic")
set(CXXFLAGS_DEBUG "${CXXFLAGS_DEBUG} -fPIC -ffunction-sections -fdata-sections")
set(CXXFLAGS_DEBUG "${CXXFLAGS_DEBUG} -fno-strict-aliasing -fno-omit-frame-pointer")

string(STRIP ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  if ("${OS_UNAME_SYSTEM}" STREQUAL "Darwin")
    set(LDFLAGS "-Wl,-O2 -fuse-ld=lld")
  elseif ("${OS_UNAME_SYSTEM}" STREQUAL "Linux")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(LDFLAGS "-Wl,-O2 -Wl,-fuse-ld=lld")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      set(LDFLAGS "-Wl,-O2 -Wl,-fuse-ld=gold")
    endif()
  endif()
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  if ("${OS_UNAME_SYSTEM}" STREQUAL "Darwin")
    set(LDFLAGS "-Wl,-O0 -Wl,-fuse-ld=lld")
  elseif ("${OS_UNAME_SYSTEM}" STREQUAL "Linux")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(LDFLAGS "-Wl,-O0 -Wl,-fuse-ld=lld")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      set(LDFLAGS "-Wl,-O0 -Wl,-fuse-ld=gold")
    endif()
  endif()
endif()

set(CMAKE_INSTALL_PREFIX "/usr/local/qt-6.7.1")
set(CMAKE_C_COMPILER "${CC}")
set(CMAKE_CXX_COMPILER "${CXX}")


if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(CMAKE_C_FLAGS "${CFLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CFLAGS}")
  set(CMAKE_C_FLAGS_DEBUG "${CFLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS "${CXXFLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CXXFLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CXXFLAGS_DEBUG}")
  set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS} -fPIE")
  set(CMAKE_SHARED_LINKER_FLAGS "${LDFLAGS} -fPIC")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(CMAKE_C_FLAGS "${CFLAGS_DEBUG}")
  set(CMAKE_C_FLAGS_RELEASE "${CFLAGS_DEBUG}")
  set(CMAKE_C_FLAGS_DEBUG "${CFLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS "${CXXFLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CXXFLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CXXFLAGS_DEBUG}")
  set(CMAKE_EXE_LINKER_FLAGS "${LDFLAGS} -fPIE")
  set(CMAKE_SHARED_LINKER_FLAGS "${LDFLAGS} -fPIC")
endif()

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

add_compile_definitions(-D_REENTRANT -DHAVE_PTHREAD -DHAVE_SCHED)
add_compile_definitions(-DHAVE_SCHED_YIELD)

qt_internal_qtbase_build_repo()

