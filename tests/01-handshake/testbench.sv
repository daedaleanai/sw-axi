


module testbench;
  sw_axi::Bridge    bridge = new("01-handshake-sv");
  sw_axi::SlaveLite ramBridge = new();
  initial begin
    sw_axi::Status st;
    sw_axi::SystemInfo rInfo;
    sw_axi::IpConfig ramConfig;

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

    ramConfig.name = "RAM";
    ramConfig.address = 'h4000;
    ramConfig.size = 'h2000;
    ramConfig.firstInterrupt = 0;
    ramConfig.numInterrupts = 0;
    ramConfig.typ = sw_axi::SLAVE_LITE;
    ramConfig.implementation = sw_axi::HARDWARE;

    st = bridge.registerSlave(ramBridge, ramConfig);
    if (st.isError()) begin
      $error("Unable to register the RAM slave: %s", st.message);
      $finish();
    end

    st = bridge.commitIp();
    if (st.isError()) begin
      $error("Unable to commit the IP: %s", st.message);
      $finish();
    end

    st = bridge.start();
    if (st.isError()) begin
      $error("Unable to start the bridge: %s", st.message);
      $finish();
    end

    foreach (bridge.peers[i]) begin
      sw_axi::SystemInfo si;
      si = bridge.peers[i];
      $display("%20s %25s %s:%0d", si.name, si.systemName, si.hostname, si.pid);
    end

    foreach (bridge.ipBlocks[i]) begin
      sw_axi::IpConfig ip;
      ip = bridge.ipBlocks[i];
      if (ip.implementation == sw_axi::SOFTWARE) begin
        $write("[SW] ");
      end else begin
        $write("[HW] ");
      end
      case (ip.typ)
        sw_axi::SLAVE_LITE: $write("[SLAVE LITE] ");
        default: $write("   [UNKNOWN] ");
      endcase
      $write("address: [0x%016h+0x%016h] ", ip.address, ip.size);
      $write("interrupts: [%05d+%05d] ", ip.firstInterrupt, ip.numInterrupts);
      $write("%s\n", ip.name);
    end

    $display("Disconnecting");
    bridge.disconnect();
    $finish;
  end
endmodule
