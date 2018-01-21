set(SAP_PREFIX       sap_1_0_latest)
set(SAP_VERSION      v1.0_latest)
set(SAP_BIN_DIR      ${PROJECT_BINARY_DIR}/${SAP_PREFIX}-build)
set(SAP_INSTALL_DIR  ${PROJECT_BINARY_DIR}/${SAP_PREFIX}-install)
set(SAP_INC_DIR      ${SAP_INSTALL_DIR}/include)
set(SAP_LIB_DIR      ${SAP_INSTALL_DIR}/lib)
set(SAP_LIB          ${SAP_INSTALL_DIR}/lib/libsap.a)

ExternalProject_Add(${SAP_PREFIX}
    PREFIX ${SAP_PREFIX}
    GIT_REPOSITORY https://github.com/Cloud-Automation/simple-argument-parser.git
    BINARY_DIR ${SAP_BIN_DIR}
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${SAP_INSTALL_DIR}
      -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
      -DSTATIC=ON 
    LOG_DOWNLOAD 1
    LOG_BUILD 1
    STEP_TARGETS build)


message(STATUS "Source directory of ${SAP_PREFIX} ${SAP_SRC_DIR}")
message(STATUS "Source directory of ${SAP_PREFIX} ${SAP_INC_DIR}")
message(STATUS "Build directory of ${SAP_PREFIX} ${SAP_BIN_DIR}")
