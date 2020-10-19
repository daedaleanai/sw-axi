set objs [get_objects -r *]

if { [llength ${objs}] != 0 } {
    open_vcd
    log_vcd [get_objects -r *]
}

run all

if { [llength ${objs}] != 0 } {
    close_vcd
}

exit
