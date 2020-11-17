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

import "DPI-C" function string sw_axi_status_get_msg(chandle status);
import "DPI-C" function int unsigned sw_axi_status_get_code(chandle status);
import "DPI-C" function void sw_axi_status_delete(chandle status);

import "DPI-C" function string sw_axi_system_info_get_name(chandle systemInfo);
import "DPI-C" function string sw_axi_system_info_get_system_name(chandle systemInfo);
import "DPI-C" function longint unsigned sw_axi_system_info_get_pid(chandle systemInfo);
import "DPI-C" function string sw_axi_system_info_get_hostname(chandle systemInfo);
import "DPI-C" function void sw_axi_system_info_delete(chandle systemInfo);

/**
 * Properties of a connected system
 */
typedef struct {
  string name;
  string systemName;
  longint unsigned pid;
  string hostname;
} SystemInfo;

/**
 * Generic status indicator for operations
 */
class Status;
  integer unsigned code;
  string message;

  function integer isOk();
    return code == 0;
  endfunction

  function integer isError();
    return code != 0;
  endfunction
endclass


function SystemInfo convertSystemInfo(chandle si);
  SystemInfo systemInfo;
  systemInfo.name = sw_axi_system_info_get_name(si);
  systemInfo.systemName = sw_axi_system_info_get_system_name(si);
  systemInfo.pid = sw_axi_system_info_get_pid(si);
  systemInfo.hostname = sw_axi_system_info_get_hostname(si);
  sw_axi_system_info_delete(si);
  return systemInfo;
endfunction

function Status convertStatus(chandle st);
  automatic Status status = new();
  status.message = sw_axi_status_get_msg(st);
  status.code = sw_axi_status_get_code(st);
  sw_axi_status_delete(st);
  return status;
endfunction
