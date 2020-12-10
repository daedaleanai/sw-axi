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

package sw_axi

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"runtime"
	"strings"
	"sync"

	flatbuffers "github.com/google/flatbuffers/go"
	log "github.com/sirupsen/logrus"
	"router/sw_axi/wire"
)

type client struct {
	Id         uint64
	SystemInfo SystemInfo
	wg         *sync.WaitGroup
	conn       net.Conn
	outgoing   chan []byte
	incoming   chan []byte
}

func (c *client) readMsg() ([]byte, error) {
	sizeArr := make([]byte, 8)
	if _, err := io.ReadFull(c.conn, sizeArr); err != nil {
		return nil, fmt.Errorf("Unable to read message size: %s", err)
	}
	size := binary.LittleEndian.Uint64(sizeArr)

	msg := make([]byte, size)
	if _, err := io.ReadFull(c.conn, msg); err != nil {
		return nil, fmt.Errorf("Unable to read message: %s", err)
	}
	return msg, nil
}

func (c *client) writeMsg(msg []byte) error {
	sizeArr := make([]byte, 8)
	binary.LittleEndian.PutUint64(sizeArr, uint64(len(msg)))
	if _, err := io.Copy(c.conn, bytes.NewReader(sizeArr)); err != nil {
		return fmt.Errorf("Unable to write message size: %s", err)
	}

	if _, err := io.Copy(c.conn, bytes.NewReader(msg)); err != nil {
		return fmt.Errorf("Unable to write message: %s", err)
	}
	return nil
}

func (c *client) shakeHands() error {
	msgArr, err := c.readMsg()
	if err != nil {
		return err
	}

	msg := wire.GetRootAsMessage(msgArr, 0)
	if msg.Type() != wire.TypeSYSTEM_INFO {
		return fmt.Errorf("Expected a SYSTEM_INFO message but got: %s", wire.EnumNamesType[msg.Type()])
	}

	si := msg.SystemInfo(nil)
	c.SystemInfo = SystemInfo{string(si.Name()), string(si.SystemName()), string(si.Hostname()), si.Pid()}

	hn, err := os.Hostname()
	if err != nil {
		log.Fatalf("Can't get hostname: %s", err)
	}

	builder := flatbuffers.NewBuilder(0)
	router := builder.CreateString("router")
	sysName := builder.CreateString(strings.ToUpper(runtime.GOOS[:1]) + runtime.GOOS[1:] + " Golang")
	hName := builder.CreateString(hn)

	wire.SystemInfoStart(builder)
	wire.SystemInfoAddName(builder, router)
	wire.SystemInfoAddSystemName(builder, sysName)
	wire.SystemInfoAddHostname(builder, hName)
	wire.SystemInfoAddPid(builder, uint64(os.Getpid()))
	mySi := wire.SystemInfoEnd(builder)

	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeSYSTEM_INFO)
	wire.MessageAddSystemInfo(builder, mySi)
	builder.Finish(wire.MessageEnd(builder))

	rsp := builder.FinishedBytes()
	return c.writeMsg(rsp)
}

func (c *client) receiveIpInfo() (*IpInfo, error) {
	msgArr, err := c.readMsg()
	if err != nil {
		return nil, err
	}

	msg := wire.GetRootAsMessage(msgArr, 0)
	if msg.Type() == wire.TypeCOMMIT {
		return nil, nil
	}

	if msg.Type() != wire.TypeIP_INFO {
		return nil, fmt.Errorf("Expected IP_INFO or COMMIT, got: %s", wire.EnumNamesType[msg.Type()])
	}

	ii := msg.IpInfo(nil)
	var typ IpType
	switch ii.Type() {
	case wire.IpTypeSLAVE:
		typ = SLAVE
	case wire.IpTypeSLAVE_LITE:
		typ = SLAVE_LITE
	case wire.IpTypeSLAVE_STREAM:
		typ = SLAVE_STREAM
	case wire.IpTypeMASTER:
		typ = MASTER
	case wire.IpTypeMASTER_LITE:
		typ = MASTER_LITE
	case wire.IpTypeMASTER_STREAM:
		typ = MASTER_STREAM
	}

	var impl IpImplementation
	switch ii.Implementation() {
	case wire.ImplementationTypeSOFTWARE:
		impl = SOFTWARE
	case wire.ImplementationTypeHARDWARE:
		impl = HARDWARE
	}
	return &IpInfo{
			string(ii.Name()),
			ii.Address(),
			ii.Size(),
			ii.FirstInterrupt(),
			ii.NumInterrupts(),
			typ,
			impl,
			0,
			c.Id,
		},
		nil
}

func (c *client) ackIpInfo(id uint64) error {
	builder := flatbuffers.NewBuilder(0)
	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeIP_ACK)
	wire.MessageAddIpId(builder, id)
	builder.Finish(wire.MessageEnd(builder))
	return c.writeMsg(builder.FinishedBytes())
}

func (c *client) ack() error {
	builder := flatbuffers.NewBuilder(0)
	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeACK)
	builder.Finish(wire.MessageEnd(builder))
	return c.writeMsg(builder.FinishedBytes())
}

func (c *client) sendError(err error) error {
	return c.writeMsg(createErrorMessage(err))
}

func (c *client) commit(clients []SystemInfo, ips []*IpInfo) error {
	// Acknowledge the commit message that ended the handshake
	if err := c.ack(); err != nil {
		return err
	}

	// Send the info about all clients
	for _, cl := range clients {
		builder := flatbuffers.NewBuilder(0)
		name := builder.CreateString(cl.Name)
		sysName := builder.CreateString(cl.SystemName)
		hName := builder.CreateString(cl.Hostname)

		wire.SystemInfoStart(builder)
		wire.SystemInfoAddName(builder, name)
		wire.SystemInfoAddSystemName(builder, sysName)
		wire.SystemInfoAddHostname(builder, hName)
		wire.SystemInfoAddPid(builder, cl.Pid)
		si := wire.SystemInfoEnd(builder)

		wire.MessageStart(builder)
		wire.MessageAddType(builder, wire.TypeSYSTEM_INFO)
		wire.MessageAddSystemInfo(builder, si)
		builder.Finish(wire.MessageEnd(builder))

		rsp := builder.FinishedBytes()
		if err := c.writeMsg(rsp); err != nil {
			return err
		}
	}

	// Acknowledge the list of clients
	if err := c.ack(); err != nil {
		return err
	}

	// Send the full IP list
	for _, ip := range ips {
		builder := flatbuffers.NewBuilder(0)
		name := builder.CreateString(ip.Name)

		var typ wire.IpType
		switch ip.Type {
		case SLAVE:
			typ = wire.IpTypeSLAVE
		case SLAVE_LITE:
			typ = wire.IpTypeSLAVE_LITE
		case SLAVE_STREAM:
			typ = wire.IpTypeSLAVE_STREAM
		case MASTER:
			typ = wire.IpTypeMASTER
		case MASTER_LITE:
			typ = wire.IpTypeMASTER_LITE
		case MASTER_STREAM:
			typ = wire.IpTypeMASTER_STREAM
		}

		var impl wire.ImplementationType
		switch ip.Implementation {
		case SOFTWARE:
			impl = wire.ImplementationTypeSOFTWARE
		case HARDWARE:
			impl = wire.ImplementationTypeHARDWARE
		}

		wire.IpInfoStart(builder)
		wire.IpInfoAddName(builder, name)
		wire.IpInfoAddAddress(builder, ip.Address)
		wire.IpInfoAddSize(builder, ip.Size)
		wire.IpInfoAddFirstInterrupt(builder, ip.FirstInterrupt)
		wire.IpInfoAddNumInterrupts(builder, ip.NumInterrupts)
		wire.IpInfoAddType(builder, typ)
		wire.IpInfoAddImplementation(builder, impl)
		ipInfo := wire.SystemInfoEnd(builder)

		wire.MessageStart(builder)
		wire.MessageAddType(builder, wire.TypeIP_INFO)
		wire.MessageAddIpInfo(builder, ipInfo)
		builder.Finish(wire.MessageEnd(builder))

		rsp := builder.FinishedBytes()
		if err := c.writeMsg(rsp); err != nil {
			return err
		}
	}

	// Acknowledge the IP list
	return c.ack()
}

func (c *client) writer() {
	for {
		msgArr := <-c.outgoing
		err := c.writeMsg(msgArr)
		if err != nil {
			log.Fatalf("Cannot write message to client %s: %s", c.SystemInfo.Name, err)
		}

		msg := wire.GetRootAsMessage(msgArr, 0)
		log.Debugf("[%20s] Sent a message of type %s", c.SystemInfo.Name, wire.EnumNamesType[msg.Type()])
		if msg.Type() == wire.TypeDONE {
			break
		}
	}
	c.wg.Done()
}

func (c *client) reader() {
	for {
		msgArr, err := c.readMsg()
		if err != nil {
			log.Fatalf("Cannot read a message from client %s: %s", c.SystemInfo.Name, err)
			continue
		}

		msg := wire.GetRootAsMessage(msgArr, 0)
		log.Debugf("[%20s] Received a message of type %s", c.SystemInfo.Name, wire.EnumNamesType[msg.Type()])
		if msg.Type() == wire.TypeDONE {
			break
		}
		c.incoming <- msgArr
	}
	c.wg.Done()
}

func (c *client) close() {
	c.conn.Close()
	log.Infof("[%20s] Logged out", c.SystemInfo.Name)
}

func newClient(id int, conn net.Conn, incoming chan []byte, wg *sync.WaitGroup) *client {
	c := new(client)
	c.conn = conn
	c.wg = wg
	c.SystemInfo.Name = "unknown"
	c.outgoing = make(chan []byte)
	c.incoming = incoming
	return c
}
