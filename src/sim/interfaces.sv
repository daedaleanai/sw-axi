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
 * A slave interface for concrete implementation of the translations layer between transaction objects
 * and the actual bus signals
 */
virtual class Slave;
  string name;
  Range addressSpace;

  function new(string name_, Range addressSpace_);
    name = name_;
    addressSpace = addressSpace_;
  endfunction

  function string getName();
    return name;
  endfunction

  function Range getAddressSpace();
    return addressSpace;
  endfunction

  pure virtual function IpType getType();
  pure virtual function IpImplementation getImplementation();
  pure virtual function int queueReadTransaction(DataTransaction txn);
  pure virtual function int queueWriteTransaction(DataTransaction txn);
  pure virtual task drive();
endclass
