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

//go:generate go run golang.org/x/tools/cmd/stringer -type IpType data.go
//go:generate go run golang.org/x/tools/cmd/stringer -type IpImplementation data.go

package sw_axi

type IpType int

const (
	SLAVE IpType = iota
	SLAVE_LITE
	SLAVE_STREAM
	MASTER
	MASTER_LITE
	MASTER_STREAM
)

type IpImplementation int

const (
	SOFTWARE IpImplementation = iota
	HARDWARE
)

type SystemInfo struct {
	Name       string
	SystemName string
	Hostname   string
	Pid        uint64
}

type IpInfo struct {
	Name           string
	Address        uint64
	Size           uint64
	FirstInterrupt uint16
	NumInterrupts  uint16
	Type           IpType
	Implementation IpImplementation
}
