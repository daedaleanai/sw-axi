macro(add_dpi_library)
  cmake_parse_arguments(ADD_DPI_LIBRARY "" "NAME" "FILES;DEPS" ${ARGN})
  if (${USE_MODELSIM})
    add_dpi_library_modelsim(
      NAME  ${ADD_DPI_LIBRARY_NAME}
      FILES ${ADD_DPI_LIBRARY_FILES}
      DEPS  ${ADD_DPI_LIBRARY_DEPS}
    )
  elseif(${USE_XSIM})
    add_dpi_library_vivado(
      NAME  ${ADD_DPI_LIBRARY_NAME}
      FILES ${ADD_DPI_LIBRARY_FILES}
      DEPS  ${ADD_DPI_LIBRARY_DEPS}
    )
  else()
    message(FATAL_ERROR "No simulator selected")
  endif()
endmacro()

macro(add_sv_library)
  cmake_parse_arguments(ADD_SV_LIBRARY "" "NAME" "FILES;DPI_FILES;DEPS" ${ARGN})
  set(DPI_TARGET "")

  if (ADD_SV_LIBRARY_DPI_FILES)
    add_dpi_library(
      NAME ${ADD_SV_LIBRARY_NAME}-dpi
      FILES ${ADD_SV_LIBRARY_DPI_FILES}
      DEPS ${ADD_SV_LIBRARY_DEPS}
      )
    set(DPI_TARGET ${CMAKE_CURRENT_BINARY_DIR}/${ADD_SV_LIBRARY_NAME}-dpi.so)
  endif()

  set(OUT ${CMAKE_CURRENT_BINARY_DIR}/${ADD_SV_LIBRARY_NAME}.tar)
  add_custom_target(${ADD_SV_LIBRARY_NAME} ALL DEPENDS ${OUT})
  set_target_properties(${ADD_SV_LIBRARY_NAME} PROPERTIES SV_LIB_FILE ${OUT})
  add_custom_command(
    OUTPUT ${OUT}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build-sv-lib.sh ${ADD_SV_LIBRARY_NAME} ${OUT} ${CMAKE_CURRENT_SOURCE_DIR} ${ADD_SV_LIBRARY_FILES}
    DEPENDS ${ADD_SV_LIBRARY_FILES} ${DPI_TARGET} ${ADD_SV_LIBRARY_DEPS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endmacro()

macro(add_sv_app)
  cmake_parse_arguments(ADD_SV_APP "" "NAME" "FILES;DEPS" ${ARGN} )
  set(OUT ${CMAKE_CURRENT_BINARY_DIR}/${ADD_SV_APP_NAME}.tar)
  add_custom_target(${ADD_SV_APP_NAME} ALL DEPENDS ${OUT})
  set(DEP_LIBS "")

  foreach(DEP ${ADD_SV_APP_DEPS})
    get_target_property(PROP ${DEP} SV_LIB_FILE)
    list(APPEND DEP_LIBS ${PROP})
  endforeach()

  add_custom_command(
    OUTPUT ${OUT}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build-sv-app.sh ${ADD_SV_APP_NAME} ${OUT} ${CMAKE_CURRENT_SOURCE_DIR} ${DEP_LIBS} ${ADD_SV_APP_FILES}
    DEPENDS ${ADD_SV_APP_FILES} ${DEP_LIBS} ${ADD_SV_APP_DEPS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endmacro()
