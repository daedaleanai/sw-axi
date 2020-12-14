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
	"fmt"
	"net"
	"os"
	"strings"
	"sync"

	flatbuffers "github.com/google/flatbuffers/go"
	log "github.com/sirupsen/logrus"
	"router/sw_axi/wire"
)

func createErrorMessage(err error) []byte {
	builder := flatbuffers.NewBuilder(0)
	msg := builder.CreateString(err.Error())
	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeERROR)
	wire.MessageAddErrorMessage(builder, msg)
	builder.Finish(wire.MessageEnd(builder))
	return builder.FinishedBytes()
}

func createErrorTxn(initiator, id uint64, typ wire.TransactionType, err error) []byte {
	builder := flatbuffers.NewBuilder(0)
	msg := builder.CreateString(err.Error())

	wire.TransactionStart(builder)

	if typ == wire.TransactionTypeREAD_REQ {
		wire.TransactionAddType(builder, wire.TransactionTypeREAD_RESP)
	}
	if typ == wire.TransactionTypeWRITE_REQ {
		wire.TransactionAddType(builder, wire.TransactionTypeWRITE_RESP)
	}

	wire.TransactionAddInitiator(builder, initiator)
	wire.TransactionAddId(builder, id)
	wire.TransactionAddOk(builder, false)
	wire.TransactionAddMessage(builder, msg)
	txn := wire.TransactionEnd(builder)

	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeTRANSACTION)
	wire.MessageAddTxn(builder, txn)
	builder.Finish(wire.MessageEnd(builder))
	return builder.FinishedBytes()
}

type Router struct {
	uri         string
	numClients  int
	clients     []*client
	ipCount     uint64
	masterCount uint64
	ipMMap      map[uint64]*IpInfo
	ips         []*IpInfo
	incoming    chan []byte
	wg          sync.WaitGroup
}

func NewRouter(uri string, numClients int) (*Router, error) {
	if !strings.HasPrefix(uri, "unix://") {
		return nil, fmt.Errorf("Only unix:// protocol is supported")
	}

	if numClients <= 0 {
		return nil, fmt.Errorf("You'd probably really like to have at least some clients")
	}

	router := Router{}
	router.uri = uri
	router.numClients = numClients
	router.ipMMap = make(map[uint64]*IpInfo)
	router.incoming = make(chan []byte)
	return &router, nil
}

func (r *Router) registerIp(ip *IpInfo) (uint64, error) {
	if ip.Type == MASTER || ip.Type == MASTER_LITE || ip.Type == MASTER_STREAM {
		r.masterCount++
	}

	for _, rIp := range r.ips {
		if ip.Address >= rIp.Address && ip.Address < (rIp.Address+rIp.Size) {
			return 0, fmt.Errorf("Address space overlaps with already registered %s IP block", rIp.Name)
		}
		if ip.Address < rIp.Size && (ip.Address+ip.Size) > rIp.Address {
			return 0, fmt.Errorf("Address space overlaps with already registered %s IP block", rIp.Name)
		}
	}
	id := r.ipCount
	ip.Id = id
	r.ipCount++
	r.ipMMap[ip.Address] = ip
	r.ips = append(r.ips, ip)
	log.Infof("[%20s] %s", r.clients[ip.ClientId].SystemInfo.Name, ip.String())
	return id, nil
}

func (r *Router) sendDone() {
	builder := flatbuffers.NewBuilder(0)
	wire.MessageStart(builder)
	wire.MessageAddType(builder, wire.TypeDONE)
	builder.Finish(wire.MessageEnd(builder))

	for _, ch := range r.clients {
		ch.outgoing <- builder.FinishedBytes()
	}
}

// Return the IP ID and the Client ID of the IP block
func (r *Router) findTarget(address, size uint64) (uint64, uint64, error) {
	var ip *IpInfo
	for _, rIp := range r.ipMMap {
		if address >= rIp.Address && address < (rIp.Address+rIp.Size) {
			ip = rIp
			break
		}
	}

	if ip == nil {
		return 0, 0, fmt.Errorf("No IP block found at address: 0x%016x", address)
	}

	if address+size > ip.Address+ip.Size {
		return 0, 0, fmt.Errorf("Transaction spans blocks bondary: %s", ip.Name)
	}

	return ip.Id, ip.ClientId, nil
}

func (r *Router) route() {
	for {
		msgArr := <-r.incoming
		msg := wire.GetRootAsMessage(msgArr, 0)

		if msg.Type() == wire.TypeTERMINATE {
			ip := r.ips[msg.IpId()]
			log.Infof("[%20s] %s terminated", r.clients[ip.ClientId].SystemInfo.Name, ip.Name)
			r.masterCount--
			if r.masterCount == 0 {
				log.Infof("No active master remains")
				r.sendDone()
				break
			}
		}

		if msg.Type() != wire.TypeTRANSACTION {
			log.Fatalf("Received unexpected message: %s", msg.Type())
		}

		txn := msg.Txn(nil)
		op := "Write"
		if txn.Type() == wire.TransactionTypeREAD_RESP || txn.Type() == wire.TransactionTypeREAD_REQ {
			op = "Read "
		}
		status := ""
		if !txn.Ok() {
			status = "error "
		}

		if txn.Type() == wire.TransactionTypeREAD_RESP || txn.Type() == wire.TransactionTypeWRITE_RESP {
			log.Debugf("Routing %sresponse %d->%d %s:[0x%016x+0x%016x]", status, txn.Initiator(), txn.Target(),
				op, txn.Address(), txn.Size())
			r.clients[r.ips[txn.Initiator()].ClientId].outgoing <- msgArr
			continue
		}

		if txn.Type() == wire.TransactionTypeREAD_REQ || txn.Type() == wire.TransactionTypeWRITE_REQ {
			target, client, err := r.findTarget(txn.Address(), txn.Size())
			if err != nil {
				log.Debugf("Unable to find target: %s", err)
				msgArr = createErrorTxn(txn.Initiator(), txn.Id(), txn.Type(), err)
				r.clients[r.ips[txn.Initiator()].ClientId].outgoing <- msgArr
				continue
			}

			msg.Txn(nil).MutateTarget(target)
			log.Debugf("Routing %srequest %d->%d %s:[0x%016x+0x%016x]", status, txn.Initiator(), txn.Target(),
				op, txn.Address(), txn.Size())

			r.clients[client].outgoing <- msgArr
			continue
		}
	}
	r.wg.Done()
}

func (r *Router) Run() error {
	if err := os.RemoveAll(r.uri[7:]); err != nil {
		return fmt.Errorf("Can't remove the socket file: %s", err)
	}

	l, err := net.Listen("unix", r.uri[7:])
	if err != nil {
		return fmt.Errorf("Can't listen at %s: %s", r.uri, err)
	}
	defer l.Close()

	r.wg.Add(r.numClients*2 + 1)

	for i := 0; i < r.numClients; i++ {
		conn, err := l.Accept()
		if err != nil {
			return fmt.Errorf("Can't accept a client connection: %s", err)
		}
		ch := newClient(i, conn, r.incoming, &r.wg)
		r.clients = append(r.clients, ch)
		defer ch.close()
	}

	clInfo := []SystemInfo{}
	for _, ch := range r.clients {
		if err := ch.shakeHands(); err != nil {
			return fmt.Errorf("Can't shake hands with client %s: %s", ch.SystemInfo.Name, err)
		}
		log.Infof("Connected: %s", ch.SystemInfo)
		log.Infof("Ip blocks:")

		for {
			ipInfo, err := ch.receiveIpInfo()
			if err != nil {
				return fmt.Errorf("Can't receive IP info from client %s: %s", ch.SystemInfo.Name, err)
			}
			if ipInfo == nil {
				break
			}

			id, err := r.registerIp(ipInfo)
			if err != nil {
				if err := ch.sendError(err); err != nil {
					return fmt.Errorf("Can't receive IP info from client %s: %s", ch.SystemInfo.Name, err)
				}
			} else {
				if err := ch.ackIpInfo(id); err != nil {
					return fmt.Errorf("Can't receive IP info from client %s: %s", ch.SystemInfo.Name, err)
				}
			}
		}
		clInfo = append(clInfo, ch.SystemInfo)
	}

	for _, ch := range r.clients {
		if err := ch.commit(clInfo, r.ips); err != nil {
			return fmt.Errorf("Can't shake hands with client %s: %s", ch.SystemInfo.Name, err)
		}
	}

	for _, ch := range r.clients {
		go ch.writer()
		go ch.reader()
	}

	go r.route()

	r.wg.Wait()

	return nil
}
