
module testbench;
  sw_axi::Bridge    bridge = new();
  sw_axi::SlaveLite slaveBridge = new("SRAM", '{'h0000_0000_1000_0000, 'h0000_0000_1000_1000});

  initial begin
    if (bridge.connect() != 0) begin
      $error("Failed to connect to the software driver!");
    end else begin
      $display("Connected to the software driver!");
    end

    if (bridge.registerSlave(slaveBridge) != 0) begin
      $error("Failed to register the slave bridge!");
    end

    if (bridge.start() != 0) begin
      $error("Failed to start the bridge!");
    end

    bridge.waitForDisconnect();
    $finish();
  end
endmodule
