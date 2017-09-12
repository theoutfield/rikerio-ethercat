set(RIO_PREFIX      rio101)
set(RIO_VERSION     v1.0.1-dev)
set(RIO_BIN_DIR     ${PROJECT_BINARY_DIR}/${RIO_PREFIX}-build)
set(RIO_INSTALL_DIR ${PROJECT_BINARY_DIR}/${RIO_PREFIX}-install)

ExternalProject_Add(${RIO_PREFIX}
    PREFIX ${RIO_PREFIX}
    GIT_REPOSITORY git@github.com:stefanpoeter/rikerio-server.git
    GIT_TAG ${RIO_VERSION}
    BINARY_DIR ${RIO_BIN_DIR}
    CMAKE_COMMAND cmake -DCMAKE_INSTALL_PREFIX=${RIO_INSTALL_DIR}
    LOG_DOWNLOAD 1
    LOG_BUILD 1
    STEP_TARGETS build)

ExternalProject_Get_Property(${RIO_PREFIX} SOURCE_DIR)

set(RIO_SRC_DIR  ${SOURCE_DIR})
set(RIO_INC_DIR  ${SOURCE_DIR}/include)

message(STATUS "Source directory of ${RIO_PREFIX} ${RIO_SRC_DIR}")
message(STATUS "Source directory of ${RIO_PREFIX} ${RIO_INC_DIR}")
message(STATUS "Build directory of ${RIO_PREFIX} ${RIO_BIN_DIR}")
