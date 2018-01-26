cmake_minimum_required (VERSION 3.1.0)
project (RikerIO-EtherCAT C CXX)

# Switch this option on if you want to build
# the RikerIO Server Library automatically, otherwise
# specify a RIO_INCLUDE AND RIO_LIB to look for
# rikerio header files and the library
OPTION(WITH_RIO "Build RikerIO Server Libray" OFF)

include(ExternalProject)
include(${PROJECT_SOURCE_DIR}/thirdparty/yaml.cmake)
include(${PROJECT_SOURCE_DIR}/thirdparty/soem.cmake)
include(${PROJECT_SOURCE_DIR}/thirdparty/sap.cmake)

if (WITH_RIO)
  include(${PROJECT_SOURCE_DIR}/thirdparty/rio.cmake)
 else()
     set(RIO_LIB rikerio)
endif()

set(RikerIO_EtherCAT_VERSION_MAJOR  1)
set(RikerIO_EtherCAT_VERSION_MINOR  2)
set(RikerIO_EtherCAT_VERSION_PATCH  4)
set(ROOT                            ${PROJECT_SOURCE_DIR})
set(INC                             ${ROOT}/include)
set(SRC                             ${ROOT}/src)

find_package(Threads REQUIRED)

add_executable(ethercat
    ${SRC}/main.c
    ${SRC}/scan.c
    ${SRC}/map.c
    ${SRC}/help.c
    ${SRC}/rikerio.c
    ${SRC}/ec-slaves.c
    ${SRC}/ec_config.c
    ${SRC}/ec-tools.c
    ${SRC}/hashmap.c
    ${SRC}/ecyaml.c
    ${SRC}/run.c)

target_link_libraries(ethercat 
    pthread rt
    ${SAP_LIB}
    ${YAML_BIN_DIR}/libyaml.a 
    ${SOEM_INSTALL_DIR}/lib/libsoem.a 
    ${RIO_LIB})
target_compile_features(ethercat PRIVATE cxx_range_for)
target_include_directories(ethercat 
    PUBLIC 
      ${PROJECT_BINARY_DIR}
      ${INC}
      ${SAP_INC_DIR}
      ${YAML_SRC_DIR}/include
      ${SOEM_INSTALL_DIR}/include/soem
      ${RIO_INC_DIR})
add_dependencies(ethercat yaml017 soem-latest sap_1_0_latest)
if (WITH_RIO)
    add_dependencies(ethercat rio_2_1)
endif()

add_executable(snake ${SRC}/snake.c)
target_link_libraries(snake ${RIO_LIB} rt)
target_include_directories(snake PUBLIC ${RIO_INC_DIR})
target_compile_features(snake PRIVATE cxx_range_for)
if (WITH_RIO)
add_dependencies(snake rio_2_1)
endif()

install (TARGETS ethercat DESTINATION /usr/bin)


