
add_sv_library(
  NAME sw-axi-sv
  FILES
  axi4lite.sv
  bridge.sv
  interfaces.sv
  package.sv
  utils.sv
  DPI_FILES
  bridge.cc
  ../common/RouterClient.cc  ../common/RouterClient.hh
  ../common/Utils.cc         ../common/Utils.hh
  ../common/Data.hh
  DEPS
  flatbuffer-cc
)
