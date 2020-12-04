add_executable(01-handshake-cc testbench.cc)
target_link_libraries(01-handshake-cc sw-axi)

add_sv_app(
  NAME 01-handshake-sv
  FILES testbench.sv
  DEPS sw-axi-sv
)
