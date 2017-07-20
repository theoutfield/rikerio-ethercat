cmake_minimum_required (VERSION 3.1.0)
project (RikerIO-EtherCAT C CXX)

#configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")

OPTION(WITH_SOEM "Build Open Automation Server with SOEM Master" OFF)
OPTION(WITH_IGH  "Build RikerIO EtherCAT with IGH Master"        OFF)

set (RikerIO_EtherCAT_VERSION_MAJOR 0)
set (RikerIO_EtherCAT_VERSION_MINOR 1)

set(ROOT ${PROJECT_SOURCE_DIR})
set(INC ${ROOT}/include)
set(SRC ${ROOT}/src)

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/lib/include)
include_directories(${ROOT}/include)

find_library(SOEM 
    NAMES soem
    PATHS "${CMAKE_LIB_SOEM_PATH}"
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH)

find_library(YAML
    NAMES yaml
    PATHS "${CMAKE_LIB_YAML_PATH}"
    NOT_DEFAULT_PATH)

find_library(RIKERIO 
    NAMES rikerio
    PATHS "${CMAKE_LIB_RIO_PATH}")

find_package(Threads REQUIRED)

add_executable(ethercat
    ${SRC}/main.c
    ${SRC}/sap.c
    ${SRC}/scan.c
    ${SRC}/map.c
    ${SRC}/help.c
    ${SRC}/rikerio.c
    ${SRC}/ec-slaves.c
    ${SRC}/hashmap.c
    ${SRC}/ecyaml.c
    ${SRC}/run.c
    ${INC}/sap.h
    ${INC}/ecmap.h
    ${INC}/ec-slaves.h
    ${INC}/hashmap.h
    ${INC}/slaves-cli.h
    ${INC}/ecyaml.h)

target_link_libraries(ethercat ${SOEM} pthread ${YAML} ${RIKERIO} rt)

target_compile_features(ethercat PRIVATE cxx_range_for)

install (TARGETS ethercat DESTINATION /usr/bin)


