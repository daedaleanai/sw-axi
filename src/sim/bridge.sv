//------------------------------------------------------------------------------
// Copyright (C) 2020 Daedalean AG
//
// This file is part of SW-AXI.
//
// SW-AXI is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SW-AXI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SW-AXI.  If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

import "DPI-C" function chandle sw_axi_connect(string uri);
import "DPI-C" function void sw_axi_disconnect(chandle bridgeHandle);

import "DPI-C" function int sw_axi_ip_info_receive(chandle bridgeHandle, output chandle ip);
import "DPI-C" function string sw_axi_ip_info_get_name(chandle ip);
import "DPI-C" function void sw_axi_ip_info_get_addr(chandle ip, output longint unsigned startAddr, output longint unsigned endAddr);
import "DPI-C" function int sw_axi_ip_info_get_type(chandle ip);
import "DPI-C" function int sw_axi_ip_info_get_implementation(chandle ip);
import "DPI-C" function void sw_axi_ip_info_free(chandle ip);

import "DPI-C" function int sw_axi_acknowledge(chandle bridgeHandle);
import "DPI-C" function int sw_axi_report_error(chandle bridgeHandle, string msg);

import "DPI-C" function int sw_axi_ip_info_send(chandle bridgeHandle, string name, int ipType, int ipImplementation, longint unsigned startAddr, longint unsigned endAddr);

/**
 * A connection point to the software-size of the system
 */
class Bridge;
  chandle bridgeHandle;
  bit started;
  SlaveMap slaves;

  function new();
    bridgeHandle = null;
    started = 'b0;
    slaves = new();
  endfunction

  /**
   * Create a server socket at the rendez-vous point and wait for the client to connect. Then receive the list
   * all the IP blocks implemented on the remote side.
   *
   * @return 0 on success; -1 on failure
   */
  function int connect (string uri = "unix:///tmp/sw-axi");
    bridgeHandle = sw_axi_connect(uri);
    if (bridgeHandle == null) return 'b1;


    while (1) begin
      chandle ipInfo;
      Range addr;
      string name;
      IpType ipType;
      IpImplementation ipImplementation;
      SlaveRemote slave;

      if (sw_axi_ip_info_receive(bridgeHandle, ipInfo) != 0) begin
        return -1;
      end

      // We received a commit that we still have to acknowledge after registering all of our own IP
      if (ipInfo == null) begin
        break;
      end

      sw_axi_ip_info_get_addr(ipInfo, addr.rStart, addr.rEnd);
      name = sw_axi_ip_info_get_name(ipInfo);
      ipType = convertIpType(sw_axi_ip_info_get_type(ipInfo));
      ipImplementation = convertIpImplementation(sw_axi_ip_info_get_implementation(ipInfo));
      slave = new(name, addr, ipType, ipImplementation);
      sw_axi_ip_info_free(ipInfo);

      if (slaves.registerSlave(slave) != 0) begin
        if (sw_axi_report_error(bridgeHandle, "Unable to register IP due to address space conflict") != 0) begin
          return -1;
        end
      end else begin
        if (sw_axi_acknowledge(bridgeHandle) != 0) begin
          return -1;
        end
      end
    end
    return 0;
  endfunction

  /**
   * Wait for the client to disconnect and clean up the server resource associated with the connection
   *
   * TODO(lj): This should probably be a task
   */
  function void waitForDisconnect();
    sw_axi_disconnect(bridgeHandle);
  endfunction

  /**
   * Register a slave with the bridge
   *
   * @return 0 on success; -1 on failure
   */
  function int registerSlave(Slave slave);
    return slaves.registerSlave(slave);
  endfunction

  /**
   * Send the complete list of the available IP to the client and start handling data transactions.
   *
   * @return 0 on success; -1 on failure
   */
  function int start();
    Slave slaveArr[$];
    integer unsigned i;

    // Acknowledget the COMMIT from client
    if (sw_axi_acknowledge(bridgeHandle) != 0) begin
      return -1;
    end

    slaves.getSlaves(slaveArr);
    for (i = 0; i < slaveArr.size(); i++) begin
      Slave s = slaveArr[i];
      bit ret = sw_axi_ip_info_send(bridgeHandle, s.getName(), s.getType(), s.getImplementation(), s.getAddressSpace().rStart, s.getAddressSpace().rEnd);
      if (ret == -1) begin
        return -1;
      end
    end

    if (sw_axi_acknowledge(bridgeHandle) == -1) begin
      return -1;
    end

    return -1;
  endfunction

endclass
