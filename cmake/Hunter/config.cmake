get_filename_component(PYTHON_BASE_INCLUDE_DIR ${PYTHON_INCLUDE_DIR} DIRECTORY)
get_filename_component(PYTHON_BASE_ROOT_DIR ${PYTHON_BASE_INCLUDE_DIR} DIRECTORY)

hunter_config(Boost VERSION 1.68.0-p1 CMAKE_ARGS CMAKE_POSITION_INDEPENDENT_CODE=ON)
