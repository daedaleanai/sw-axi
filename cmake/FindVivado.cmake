
find_program(VIVADO_XSIM_EXECUTABLE NAMES xsim)
find_program(VIVADO_XSC_EXECUTABLE NAMES xsc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  VIVADO
  DEFAULT_MSG
  VIVADO_XSIM_EXECUTABLE VIVADO_XSC_EXECUTABLE
)

if (VIVADO_FOUND)
  execute_process(
    COMMAND /bin/bash -c "xsim --version | tr -d '\\n'"
    OUTPUT_VARIABLE XSIM_VERSION
  )

  macro(add_dpi_library_vivado)
    cmake_parse_arguments(ADD_DPI_LIBRARY_VIVADO "" "NAME" "FILES;DEPS" ${ARGN})
    set(OUT ${CMAKE_CURRENT_BINARY_DIR}/${ADD_DPI_LIBRARY_VIVADO_NAME}.so)
    set(COMPILATION_UNITS ${ADD_DPI_LIBRARY_VIVADO_FILES})
    list(FILTER COMPILATION_UNITS EXCLUDE REGEX ".*\.hh$|.*\.h")

    set(CUS "")
    foreach(CU ${COMPILATION_UNITS})
      list(APPEND CUS ${CMAKE_CURRENT_SOURCE_DIR}/${CU})
    endforeach()
    
    add_custom_target(${ADD_DPI_LIBRARY_VIVADO_NAME} DEPENDS ${OUT})
    add_custom_command(
      OUTPUT ${OUT}
      COMMAND ${VIVADO_XSC_EXECUTABLE} ${CUS}  --cppversion 14 --gcc_compile_options '-DSIM_ID=${XSIM_VERSION}'
      COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/xsim.dir/work/xsc/dpi.so ${OUT}
      DEPENDS ${ADD_DPI_LIBRARY_VIVADO_FILES} ${ADD_DPI_LIBRARY_VIVADO_DEPS}
    )
  endmacro()
endif()
