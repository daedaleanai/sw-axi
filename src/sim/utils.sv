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

import "DPI-C" function string sw_axi_ip_config_get_name(chandle ipConfig);
import "DPI-C" function void sw_axi_ip_config_get_address_and_size(chandle ipConfig, output longint unsigned addr, output longint unsigned size);
import "DPI-C" function void sw_axi_ip_config_get_interrupts(chandle ipConfig, output shortint unsigned first, output shortint unsigned num);
import "DPI-C" function void sw_axi_ip_config_get_type(chandle ipConfig, output int typ, output int impl);
import "DPI-C" function void sw_axi_ip_config_delete(chandle ipConfig);

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

/**
 * IP Types
 */
typedef enum {
  SLAVE,
  SLAVE_LITE,
  SLAVE_STREAM,
  MASTER,
  MASTER_LITE,
  MASTER_STREAM
} IpType;

/**
 * IpImplementation types
 */
typedef enum {
  SOFTWARE,
  HARDWARE
} IpImplementation;


/**
 * Information about an IP block
 */
typedef struct {
  string name;
  longint unsigned address;
  longint unsigned size;
  shortint unsigned firstInterrupt;
  shortint unsigned numInterrupts;
  IpType typ;
  IpImplementation implementation;
} IpConfig;

function void printIpConfig(IpConfig ip);
  if (ip.implementation == SOFTWARE) begin
    $write("[SW] ");
  end else begin
    $write("[HW] ");
  end
  case (ip.typ)
    SLAVE_LITE: $write("[SLAVE LITE] ");
    default: $write("   [UNKNOWN] ");
  endcase
  $write("address: [0x%016h+0x%016h] ", ip.address, ip.size);
  $write("interrupts: [%05d+%05d] ", ip.firstInterrupt, ip.numInterrupts);
  $write("%s\n", ip.name);
endfunction

function SystemInfo convertSystemInfo(chandle si);
  SystemInfo systemInfo;
  systemInfo.name = sw_axi_system_info_get_name(si);
  systemInfo.systemName = sw_axi_system_info_get_system_name(si);
  systemInfo.pid = sw_axi_system_info_get_pid(si);
  systemInfo.hostname = sw_axi_system_info_get_hostname(si);
  sw_axi_system_info_delete(si);
  return systemInfo;
endfunction

function IpConfig convertIpConfig(chandle ipc);
  IpConfig ipConfig;
  int typ;
  int impl;
  ipConfig.name = sw_axi_ip_config_get_name(ipc);
  sw_axi_ip_config_get_address_and_size(ipc, ipConfig.address, ipConfig.size);
  sw_axi_ip_config_get_interrupts(ipc, ipConfig.firstInterrupt, ipConfig.numInterrupts);
  sw_axi_ip_config_get_type(ipc, typ, impl);
  sw_axi_ip_config_delete(ipc);
  $cast(ipConfig.typ, typ);
  $cast(ipConfig.implementation, impl);
  return ipConfig;
endfunction

function Status convertStatus(chandle st);
  automatic Status status = new();
  status.message = sw_axi_status_get_msg(st);
  status.code = sw_axi_status_get_code(st);
  sw_axi_status_delete(st);
  return status;
endfunction
