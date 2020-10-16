
define xsim-run
$(TOP_DIR)/scripts/xsim-make-project.sh $@ $^
xvlog -prj $@.prj
SW_AXI_PARAMS=""
for i in $^; do                                                   \
  if [[ $$i == *sw-axi.prj ]]; then                               \
    $(MAKE) -C $(TOP_DIR)/src/sim sim;                            \
    SW_AXI_PARAMS="--sv_root $(TOP_DIR)/src/sim --sv_lib sw_axi"; \
  fi;                                                             \
done
eval xelab --timescale 1ns/1ps -debug typical $$SW_AXI_PARAMS $@.testbench -s testbench_sim -prj $@.prj
xsim testbench_sim -t $(TOP_DIR)/scripts/xsim-run.tcl
endef

define xsim-clean
rm -rf xvlog*
rm -rf xsim*
rm -rf xelab*
rm -rf *.wdb
rm -rf webtalk*
rm -rf dump.vcd
rm -rf xsc*
rm -rf *.log
endef
