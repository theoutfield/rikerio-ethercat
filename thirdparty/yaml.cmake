set(YAML_PREFIX     yaml017)
set(YAML_VERSION    0.1.7)
set(YAML_URL        https://api.github.com/repos/yaml/libyaml/tarball/${YAML_VERSION})
set(YAML_URL_MD5    42c5a3096489e5df98fbc1022869325c)
set(YAML_BIN_DIR    ${PROJECT_BINARY_DIR}/${YAML_PREFIX}-build)

ExternalProject_Add(${YAML_PREFIX}
    PREFIX ${YAML_PREFIX}
    GIT_REPOSITORY https://github.com/yaml/libyaml
    GIT_TAG ${YAML_VERSION}
    BINARY_DIR ${YAML_BIN_DIR}
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    LOG_DOWNLOAD 1
    LOG_BUILD 1
    STEP_TARGETS build)

ExternalProject_Get_Property(${YAML_PREFIX} SOURCE_DIR)

set(YAML_SRC_DIR  ${SOURCE_DIR})
set(YAML_INC_DIR  ${SOURCE_DIR}/include)

message(STATUS "Source directory of ${YAML_PREFIX} ${YAML_SRC_DIR}")
message(STATUS "Source directory of ${YAML_PREFIX} ${YAML_INC_DIR}")
message(STATUS "Build directory of ${YAML_PREFIX} ${YAML_BIN_DIR}")
