
add_custom_target(
  router-all ALL
  DEPENDS router)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/router
  COMMAND go build -o ${CMAKE_CURRENT_BINARY_DIR}/router
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/main.go
  ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/client.go
  ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/data.go
  ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/router.go
  ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/ipimplementation_string.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building router")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/ipimplementation_string.go
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/iptype_string.go
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sw_axi/wire
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../common/IpcStructs.fbs sw_axi/data.go
  COMMAND go generate router/...
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating source for router"
)
