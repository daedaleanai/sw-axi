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

package sw_axi;

  import "DPI-C" function chandle sw_axi_connect(string uri);
  import "DPI-C" function void sw_axi_disconnect(chandle data);

  /**
   * A connection point to the software-size of the system
   */
  class Bridge;
    chandle bridgeData;

    function new();
      bridgeData = null;
    endfunction

    /**
     * Create a server socket at the rendez-vous point and wait for the client to connect
     *
     * @return 0 on success; 1 on failure
     */
    function byte connect (string uri = "unix:///tmp/sw-axi");
      bridgeData = sw_axi_connect(uri);
      if (bridgeData == null) return 'b1;
      else return 'b0;
    endfunction

    /**
     * Wait for the client to disconnect and clean up the server resource associated with the connection
     *
     * TODO(lj): This should probably be a task
     */
    function void waitForDisconnect();
      sw_axi_disconnect(bridgeData);
    endfunction

  endclass
endpackage
