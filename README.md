
sw-axi
======

Building and running instructions
---------------------------------

`sw-axi` uses CMake for building. It requires the following packages on Debian/Ubuntu:

 * `build-essential`
 * `cmake`
 * `libflatbuffers-dev`
 * `golang`

It also requires one of the following simulators:

 * Xilinx Xsim
 * Modelsim

You can type the following to build it:

    ]==> git clone git@github.com:daedaleanai/sw-axi.git
    ]==> cd sw-axi
    ]==> mkdir build
    ]==> cd build
    ]==> cmake .. -DUSE_MODELSIM
    ]==> make

The above will detect modelsim and build the compatible SystemVerilog simulation
archives. Alternatively, you can specify `USE_XSIM` to use the Xilinx simulator
distributed together with Vivado.

You need to use one of the runtime scripts to run the simulation. Like this:

    ]==> cd tests/01-handshake
    ]==> ../../../scripts/run-modelsim.sh testbench 01-handshake-sv.tar

This will set up the simulation environment, including DPI, and run module
`testbench`.
