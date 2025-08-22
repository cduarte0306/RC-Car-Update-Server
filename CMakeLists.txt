cmake_minimum_required(VERSION 3.10)
project(rc-car-updater)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system filesystem thread)
include_directories(${Boost_INCLUDE_DIRS})

# Ensure you sourced environment-setup-*
if (NOT DEFINED ENV{SDKTARGETSYSROOT})
  message(FATAL_ERROR "SDKTARGETSYSROOT is not set. Source your Yocto SDK env.")
endif()

# Headers live directly under usr/include for IPC API
find_path(SWUPDATE_INCLUDE_DIR
  NAMES network_ipc.h
  PATHS "$ENV{SDKTARGETSYSROOT}/usr/include"
  NO_DEFAULT_PATH
)
if (NOT SWUPDATE_INCLUDE_DIR)
  message(FATAL_ERROR "Could not find network_ipc.h in $ENV{SDKTARGETSYSROOT}/usr/include")
endif()

# Library is libswupdate.so
find_library(SWUPDATE_LIB
  NAMES swupdate
  PATHS "$ENV{SDKTARGETSYSROOT}/usr/lib" "$ENV{SDKTARGETSYSROOT}/lib"
  NO_DEFAULT_PATH
)
if (NOT SWUPDATE_LIB)
  message(FATAL_ERROR "Could not find libswupdate.so in $ENV{SDKTARGETSYSROOT}")
endif()

add_library(SWUpdate::Client SHARED IMPORTED)
set_target_properties(SWUpdate::Client PROPERTIES
  IMPORTED_LOCATION             "${SWUPDATE_LIB}"
  INTERFACE_INCLUDE_DIRECTORIES "${SWUPDATE_INCLUDE_DIR}"
)

# (Optional) expose headers project-wide
include_directories("${SWUPDATE_INCLUDE_DIR}")

add_subdirectory(src)
