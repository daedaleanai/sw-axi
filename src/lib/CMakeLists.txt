add_library(
  sw-axi SHARED
  SwAxi.cc                   SwAxi.hh
  ../common/RouterClient.cc  ../common/RouterClient.hh
  ../common/Utils.cc         ../common/Utils.hh
  ../common/Data.hh
)

add_dependencies(sw-axi flatbuffer-cc)
