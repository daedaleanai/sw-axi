TOP_DIR=../..
include $(TOP_DIR)/scripts/common.mk
include $(TOP_DIR)/scripts/xilinx.mk

.PHONY: cpp sim format clean
.DEFAULT_GOAL = cpp

cpp: testbench

testbench: testbench.cc
> $(MAKE) -C $(TOP_DIR)/src/lib
> g++ -L $(TOP_DIR)/src/lib -Wl,-rpath=$(TOP_DIR)/src/lib -I $(TOP_DIR)/src/lib testbench.cc -o testbench -lsw-axi

sim: ../../src/sim/sw-axi.prj testbench.sv
> @$(xsim-run)

format: testbench.sv testbench.cc
> @$(code-format)

clean:
> @$(xsim-clean)
> rm -rf *.prj
> rm -rf testbench
