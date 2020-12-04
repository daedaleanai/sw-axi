
module testbench;
  sw_axi::Bridge bridge = new("00-version-sv");
  initial begin
    sw_axi::Status st;
    sw_axi::SystemInfo rInfo;

    st = bridge.connect();
    if (st.isError()) begin
      $error("Unable to connect to the router: %s", st.message);
      $finish();
    end

    $display("Connected to the router:");
    rInfo = bridge.routerInfo;
    $display("Name:        %s", rInfo.name);
    $display("System Name: %s", rInfo.systemName);
    $display("Pid:         %0d", rInfo.pid);
    $display("Hostname:    %s", rInfo.hostname);
    $display("Disconnecting");

    bridge.disconnect();
    $finish;
  end
endmodule
