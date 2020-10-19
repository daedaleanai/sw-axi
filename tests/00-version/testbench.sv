
module testbench;
  sw_axi::Bridge bridge = new();
  initial begin
    if (bridge.connect() == 1) begin
      $display("connected!");
    end

    bridge.waitForDisconnect();
  end
endmodule
