cmake_minimum_required (VERSION 3.1.0)
project (RikerIO-EtherCAT C CXX)

include(ExternalProject)
include(${PROJECT_SOURCE_DIR}/thirdparty/yaml.cmake)
include(${PROJECT_SOURCE_DIR}/thirdparty/soem.cmake)
include(${PROJECT_SOURCE_DIR}/thirdparty/rio.cmake)

#configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")

OPTION(WITH_SOEM "Build Open Automation Server with SOEM Master" OFF)
OPTION(WITH_IGH  "Build RikerIO EtherCAT with IGH Master"        OFF)

set(RikerIO_EtherCAT_VERSION_MAJOR  1)
set(RikerIO_EtherCAT_VERSION_MINOR  0)
set(RikerIO_EtherCAT_VERSION_PATCH  1)
set(ROOT                            ${PROJECT_SOURCE_DIR})
set(INC                             ${ROOT}/include)
set(SRC                             ${ROOT}/src)

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
    ${SRC}/run.c)

target_link_libraries(ethercat 
    pthread rt
    ${YAML_BIN_DIR}/libyaml.a 
    ${SOEM_INSTALL_DIR}/lib/libsoem.a 
    ${RIO_INSTALL_DIR}/lib/librikerio.so)
target_compile_features(ethercat PRIVATE cxx_range_for)
target_include_directories(ethercat 
    PUBLIC 
      ${PROJECT_BINARY_DIR}
      ${INC}
      ${YAML_SRC_DIR}/include
      ${SOEM_INSTALL_DIR}/include/soem
      ${RIO_INSTALL_DIR}/include)


install (TARGETS ethercat DESTINATION /usr/bin)


