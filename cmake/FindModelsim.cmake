
find_program(MODELSIM_VSIM_EXECUTABLE NAMES vsim REQUIRED)
find_program(MODELSIM_VLIB_EXECUTABLE NAMES vlib REQUIRED)
find_program(MODELSIM_VLOG_EXECUTABLE NAMES vlog REQUIRED)

get_filename_component(
  MODELSIM_GCC_PATH
  ${MODELSIM_VSIM_EXECUTABLE}
  DIRECTORY
)

set(MODELSIM_GCC_PATH "${MODELSIM_GCC_PATH}/../gcc-7.4.0-linux/bin")

find_program(
  MODELSIM_GXX_EXECUTABLE
  NAMES g++
  PATHS ${MODELSIM_GCC_PATH}
  REQUIRED
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  MODELSIM DEFAULT_MSG
  MODELSIM_VSIM_EXECUTABLE
  MODELSIM_VLIB_EXECUTABLE
  MODELSIM_VLOG_EXECUTABLE
  MODELSIM_GXX_EXECUTABLE
)

if (MODELSIM_FOUND)
  execute_process(
    COMMAND /bin/bash -c "vsim -version | tr -d '\\n' | awk '{match($$0,\"(ModelSim) (.+) Simulator\",a)}END{printf \"%s %s\", a[1], a[2]}'"
    OUTPUT_VARIABLE MODELSIM_VERSION
  )

  macro(add_dpi_library_modelsim)
    cmake_parse_arguments(ADD_DPI_LIBRARY_MODELSIM "" "NAME" "FILES;DEPS" ${ARGN})
    set(OUT ${CMAKE_CURRENT_BINARY_DIR}/${ADD_DPI_LIBRARY_MODELSIM_NAME}.so)
    set(COMPILATION_UNITS ${ADD_DPI_LIBRARY_MODELSIM_FILES})
    list(FILTER COMPILATION_UNITS EXCLUDE REGEX ".*\.hh$|.*\.h")
    add_custom_target(${ADD_DPI_LIBRARY_MODELSIM_NAME} DEPENDS ${OUT})
    add_custom_command(
      OUTPUT ${OUT}
      COMMAND ${MODELSIM_GXX_EXECUTABLE} -shared ${COMPILATION_UNITS} -o ${OUT} -DSIM_ID=${MODELSIM_VERSION}
      DEPENDS ${ADD_DPI_LIBRARY_MODELSIM_FILES} ${ADD_DPI_LIBRARY_MODELSIM_DEPS}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
  endmacro()
endif()
