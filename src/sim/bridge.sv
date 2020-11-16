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

import "DPI-C" function chandle sw_axi_client_new();
import "DPI-C" function void sw_axi_client_delete(chandle client);

import "DPI-C" function void sw_axi_client_connect(chandle client, output chandle status, output chandle systemInfo, input string uri, input string name);
import "DPI-C" function void sw_axi_client_disconnect(chandle client);

/**
  * Bridge is the software entry point of the infrastructure.
 *
 * The bridge connects to a router, registers its IP components and receives the list of the components already
 * registered with the router. It can then send and receive requests directed at particular slaves as well as send and
 * receive interrupt messages.
 */
class Bridge;
  chandle client;
  string name;
  SystemInfo routerInfo;

  function new(string name_ = "unnamed");
    client = null;
    name = name_;
  endfunction

  /**
   * Connect to the router
   *
   * @param uri an URI pointing to a rendez-vous point with the router; currently only UNIX domain sockets are
   *            supported
   */
  function Status connect (string uri = "unix:///tmp/sw-axi");
    chandle st;
    chandle si;
    client = sw_axi_client_new();
    sw_axi_client_connect(client, st, si, uri, name);

    if (si != null) begin
      routerInfo = convertSystemInfo(si);
    end
    return convertStatus(st);
  endfunction

  /**
   * Disconnect from the router
   */
  function void disconnect();
    sw_axi_client_disconnect(client);
    sw_axi_client_delete(client);
    client = null;
  endfunction
endclass
