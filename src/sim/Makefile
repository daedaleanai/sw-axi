TOP_DIR=../..
include $(TOP_DIR)/scripts/common.mk
include $(TOP_DIR)/scripts/xilinx.mk

.PHONY: sim format clean
.DEFAULT_GOAL = sim

sim: sw_axi.so

../common/IpcStructs_generated.h: ../common/IpcStructs.fbs
> (cd ../common && flatc --cpp `basename $^`)

sw_axi.so: bridge.cc ../common/Utils.hh ../common/Utils.cc ../common/IpcStructs_generated.h ../common/Data.hh ../common/RouterClient.cc ../common/RouterClient.hh
> xsc bridge.cc ../common/Utils.cc ../common/RouterClient.cc -o sw_axi --cppversion 14 --gcc_compile_options '-DSIM_ID="`xsim --version`"'  --gcc_compile_options '-DVERBOSE'

format: bridge.sv bridge.cc ../common/Utils.hh ../common/Utils.cc axi4lite.sv axi4lite.sv interfaces.sv package.sv utils.sv
> @$(code-format)

clean:
> @$(xsim-clean)
> rm -rf sw_axi.so
> rm -rf ../common/IpcStructs_generated.h
