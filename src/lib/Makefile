TOP_DIR=../..
include $(TOP_DIR)/scripts/common.mk

.PHONY: cpp sim format clean
.DEFAULT_GOAL = cpp

cpp: libsw-axi.so

../common/IpcStructs_generated.h: ../common/IpcStructs.fbs
> (cd ../common && flatc --cpp `basename $^`)

libsw-axi.so: SwAxi.cc SwAxi.hh ../common/Utils.hh ../common/Utils.cc ../common/IpcStructs_generated.h ../common/Data.hh ../common/RouterClient.cc ../common/RouterClient.hh
> g++ -fPIC SwAxi.cc ../common/Utils.cc ../common/RouterClient.cc -shared -o libsw-axi.so -DVERBOSE

format: SwAxi.cc SwAxi.hh ../common/Utils.hh ../common/Utils.cc ../common/Data.hh ../common/RouterClient.hh ../common/RouterClient.cc
> @$(code-format)

clean:
> rm -rf libsw-axi.so
> rm -rf ../common/IpcStructs_generated.h
