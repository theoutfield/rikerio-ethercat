set(RIO_PREFIX      rio_2_1)
set(RIO_VERSION     v2.1.latest)
set(RIO_BIN_DIR     ${PROJECT_BINARY_DIR}/${RIO_PREFIX}-build)
set(RIO_INSTALL_DIR ${PROJECT_BINARY_DIR}/${RIO_PREFIX}-install)

ExternalProject_Add(${RIO_PREFIX}
    PREFIX ${RIO_PREFIX}
    GIT_REPOSITORY https://github.com/RikerIO/rikerio-server.git
    GIT_TAG ${RIO_VERSION}
    BINARY_DIR ${RIO_BIN_DIR}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${RIO_INSTALL_DIR}
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
    LOG_BUILD 1
    STEP_TARGETS build)

ExternalProject_Get_Property(${RIO_PREFIX} SOURCE_DIR)

set(RIO_SRC_DIR  ${SOURCE_DIR})
set(RIO_INC_DIR  ${RIO_BIN_DIR}/include)
set(RIO_LIB      ${RIO_BIN_DIR}/librikerio.so)

message(STATUS "Source directory of ${RIO_PREFIX} ${RIO_SRC_DIR}")
message(STATUS "Source directory of ${RIO_PREFIX} ${RIO_INC_DIR}")
message(STATUS "Build directory of ${RIO_PREFIX} ${RIO_BIN_DIR}")
