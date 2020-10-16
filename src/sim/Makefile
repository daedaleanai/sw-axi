TOP_DIR=../..
include $(TOP_DIR)/scripts/common.mk
include $(TOP_DIR)/scripts/xilinx.mk

.PHONY: sim format clean
.DEFAULT_GOAL = sim

sim: sw_axi.so

IpcStructs_generated.h: ../common/IpcStructs.fbs
> flatc --cpp $^

sw_axi.so: bridge.cc ../common/Utils.cc ../common/Utils.hh IpcStructs_generated.h
> xsc bridge.cc ../common/Utils.cc -o sw_axi --cppversion 14 --gcc_compile_options '-DSIM_ID="`xsim --version`"'  --gcc_compile_options '-DVERBOSE'

format: bridge.sv bridge.cc ../common/Utils.hh ../common/Utils.cc
> @$(code-format)

clean:
> @$(xsim-clean)
> rm -rf sw_axi.so
> rm -rf IpcStructs_generated.h
