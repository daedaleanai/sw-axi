
module testbench;
  sw_axi::Bridge bridge = new();
  initial begin
    if (bridge.connect() == 0) begin
      $display("connected!");
    end

    bridge.waitForDisconnect();
  end
endmodule
