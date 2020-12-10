
add_custom_target(flatbuffer-cc DEPENDS IpcStructs_generated.h)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_SOURCE_DIR}/IpcStructs_generated.h
  COMMAND flatc --cpp IpcStructs.fbs
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/IpcStructs.fbs
  COMMENT "Compiling flatbuffer IpcStructs.fbs"
)
