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

/**
 * A helper for managing ranges; typical semantics is: `[start, end)` meaning that the start of the range is
 * include in the range, but the end is not.
 */
typedef struct {
  longint unsigned rStart;
  longint unsigned rEnd;
} Range;

/**
 * Transaction class
 */
class DataTransaction;
endclass

/**
 * A helper class for mapping request addresses to particular slave instances
 */
class SlaveMap;
  Slave slaveMap[longint unsigned];

  /**
   * Register a slave in the map. The call may fail if adding the slave would introduce a conflict in the memory map.
   *
   * @return 0 on success; 1 on failure
   */
  function int registerSlave(Slave slave);
    bit conflict = 'b0;

    foreach (slaveMap[startAddr]) begin
      if (slave.getAddressSpace().rEnd <= startAddr) begin
        break;
      end

      if (slave.getAddressSpace().rStart >= slaveMap[startAddr].getAddressSpace().rEnd) begin
        continue;
      end

      conflict = 'b1;
    end

    if (conflict == 'b1) begin
      return -1;
    end

    slaveMap[slave.getAddressSpace().rStart] = slave;
    return 0;
  endfunction

  /**
   * Retrieve a slave associated with the particular address range
   *
   * @return the slave handle on success, null on failure
   */
  function Slave getSlave(Range addressSpace);
    foreach (slaveMap[startAddr]) begin
      if (addressSpace.rStart >= startAddr &&
          addressSpace.rEnd <= slaveMap[startAddr].getAddressSpace().rEnd) begin
        return slaveMap[startAddr];
      end
    end
    return null;
  endfunction

  /**
   * Get a queue of all slaves registered with the map
   */
  function void getSlaves(ref Slave slaves[$]);
    foreach (slaveMap[startAddr]) begin
      slaves.push_back(slaveMap[startAddr]);
    end
  endfunction
endclass

/**
 * Helper for converting wire IP types into enums
 */
function IpType convertIpType(int ipType);
  case (ipType)
    1: return SLAVE_LITE;
    default: return SLAVE;
  endcase
endfunction

/**
 * Helper for converting wire IP implementation types into enums
 */
function IpImplementation convertIpImplementation(int ipImplementation);
  case (ipImplementation)
    0: return SOFTWARE;
    default: return HARDWARE;
  endcase
endfunction
