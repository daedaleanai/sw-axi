cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
enable_language(C CXX ASM)

project(sw-axi)

include(Utils)

include_directories(${CMAKE_SOURCE_DIR}/src/lib)

define_exclusive(
  NAME USE_XSIM
  CONFLICTS USE_MODELSIM
)

define_exclusive(
  NAME USE_MODELSIM
  CONFLICTS USE_XSIM
)

find_package(FlatBuffers)
find_package(Vivado)
find_package(Modelsim)

if (USE_XSIM AND NOT VIVADO_FOUND)
  message(FATAL_ERROR "Vivaddo not installed")
endif()

if (USE_MODELSIM AND NOT MODELSIM_FOUND)
  message(FATAL_ERROR "Modelsim not installed")
endif()

include(SystemVerilog)

add_subdirectory(src)
add_subdirectory(tests)

add_custom_target(
  format
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/format.sh ${CMAKE_SOURCE_DIR}
)

message(STATUS "----------------------------------------" )
message(STATUS "Use Xsim:     " ${USE_XSIM} )
message(STATUS "Use Modelsim: " ${USE_MODELSIM} )
message(STATUS "----------------------------------------" )
