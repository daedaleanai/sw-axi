TOP_DIR=../..
include $(TOP_DIR)/scripts/common.mk

.PHONY: cpp sim format clean
.DEFAULT_GOAL = cpp

cpp: libsw-axi.so

IpcStructs_generated.h: ../common/IpcStructs.fbs
> flatc --cpp $^

libsw-axi.so: SwAxi.cc SwAxi.hh ../common/Utils.hh ../common/Utils.cc IpcStructs_generated.h
> g++ -fPIC SwAxi.cc ../common/Utils.cc -shared -o libsw-axi.so

format: SwAxi.cc SwAxi.hh ../common/Utils.hh ../common/Utils.cc
> @$(code-format)

clean:
> rm -rf libsw-axi.so
> rm -rf IpcStructs_generated.h
