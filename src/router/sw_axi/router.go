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
)

type Router struct {
	uri        string
	numClients int
	clients    []*client
	ipCount    uint64
	ips        map[uint64]*IpInfo
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
	router.ips = make(map[uint64]*IpInfo)
	return &router, nil
}

func (r *Router) allocateIp(ip *IpInfo) (uint64, error) {
	id := r.ipCount
	r.ipCount++
	r.ips[ip.Address] = ip
	return id, nil
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

	var wg sync.WaitGroup
	wg.Add(r.numClients * 2)

	for i := 0; i < r.numClients; i++ {
		conn, err := l.Accept()
		if err != nil {
			return fmt.Errorf("Can't accept a client connection: %s", err)
		}
		ch := newClient(conn, &wg)
		r.clients = append(r.clients, ch)
		defer ch.close()
	}

	clInfo := []SystemInfo{}
	for _, ch := range r.clients {
		if err := ch.shakeHands(); err != nil {
			return fmt.Errorf("Can't shake hands with client %s: %s", ch.SystemInfo.Name, err)
		}

		for {
			ipInfo, err := ch.receiveIpInfo()
			if err != nil {
				return fmt.Errorf("Can't receive IP info from client %s: %s", ch.SystemInfo.Name, err)
			}
			if ipInfo == nil {
				break
			}

			id, err := r.allocateIp(ipInfo)
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

	ips := []*IpInfo{}
	for _, ip := range r.ips {
		ips = append(ips, ip)
	}

	for _, ch := range r.clients {
		if err := ch.commit(clInfo, ips); err != nil {
			return fmt.Errorf("Can't shake hands with client %s: %s", ch.SystemInfo.Name, err)
		}
	}

	for _, ch := range r.clients {
		go ch.writer()
		go ch.reader()
	}

	wg.Wait()

	return nil
}
