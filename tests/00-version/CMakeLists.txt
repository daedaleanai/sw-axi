add_executable(00-version-cc testbench.cc)
target_link_libraries(00-version-cc sw-axi)

add_sv_app(
  NAME 00-version-sv
  FILES testbench.sv
  DEPS sw-axi-sv
)
