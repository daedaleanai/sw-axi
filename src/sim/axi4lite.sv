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
 * Slave interface handling the AXI4-Lite IP
 */
class SlaveLite extends Slave;
  virtual function int queueReadTransaction(Transaction txn);
    return 0;
  endfunction

  virtual function int queueWriteTransaction(Transaction txn);
    return 0;
  endfunction

  virtual task driveReads();
  endtask

  virtual task driveWrites();
  endtask
endclass
