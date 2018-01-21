set(SOEM_PREFIX       soem-latest)
set(SOEM_VERSION      v1.3.1)
set(SOEM_BIN_DIR      ${PROJECT_BINARY_DIR}/${SOEM_PREFIX}-build)
set(SOEM_INSTALL_DIR  ${PROJECT_BINARY_DIR}/${SOEM_PREFIX}-install)

ExternalProject_Add(${SOEM_PREFIX}
    PREFIX ${SOEM_PREFIX}
    GIT_REPOSITORY https://github.com/OpenEtherCATsociety/SOEM
    BINARY_DIR ${SOEM_BIN_DIR}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${SOEM_INSTALL_DIR} 
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    LOG_DOWNLOAD 1
    LOG_BUILD 1
    STEP_TARGETS build)

ExternalProject_Get_Property(${SOEM_PREFIX} SOURCE_DIR)

set(SOEM_SRC_DIR  ${SOURCE_DIR})
set(SOEM_INC_DIR  ${SOURCE_DIR}/include)

message(STATUS "Source directory of ${SOEM_PREFIX} ${SOEM_SRC_DIR}")
message(STATUS "Include directory of ${SOEM_PREFIX} ${SOEM_INC_DIR}")
message(STATUS "Build directory of ${SOEM_PREFIX} ${SOEM_BIN_DIR}")
