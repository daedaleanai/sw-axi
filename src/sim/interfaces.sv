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


typedef struct {
  bit [7:0] data[
      ];  //!< Payload of the transaction for write requests, a buffer to be filled by read requests
  int unsigned size;  //!< Size of the buffer in bytes
  longint unsigned address;  //!< Address of the transaction
} Transaction;


/**
 * A slave interface for concrete implementation of the translations layer between transaction objects
 * and the actual bus signals
 */
virtual class Slave;
  pure virtual function int queueReadTransaction(Transaction txn);
  pure virtual function int queueWriteTransaction(Transaction txn);
  pure virtual task driveReads();
  pure virtual task driveWrites();
endclass
