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

//go:generate flatc --go ../common/IpcStructs.fbs

package main

import (
	"flag"
	"os"

	log "github.com/sirupsen/logrus"
	prefixed "github.com/x-cray/logrus-prefixed-formatter"
	"router/sw_axi"
)

func main() {
	logFile := flag.String("log-file", "", "output file for diagnostics")
	logLevel := flag.String("log-level", "Info", "verbosity of the diagnostic information")
	numClients := flag.Int("n", 2, "number of clients")
	uri := flag.String("uri", "unix:///tmp/sw-axi", "the rendez-vous point")

	flag.Parse()

	log.SetFormatter(&prefixed.TextFormatter{
		TimestampFormat: "2006-01-02 15:04:05",
		FullTimestamp:   true,
		ForceFormatting: true,
	})

	if *logFile != "" {
		f, err := os.OpenFile(*logFile, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err != nil {
			log.Fatal(err)
		}
		defer f.Close()
		log.SetOutput(f)
	}

	level := log.InfoLevel
	if *logLevel != "" {
		l, err := log.ParseLevel(*logLevel)
		if err != nil {
			log.Fatal(err)
		}
		level = l
	}
	log.SetLevel(level)

	log.Info("Starting the SW-AXI router...")
	log.Infof("Rendez-vous point: %s", *uri)
	log.Infof("Number of clients: %d", *numClients)

	router, err := sw_axi.NewRouter(*uri, *numClients)
	if err != nil {
		log.Fatalf("Can't create a router: %s", err)
	}

	if err := router.Run(); err != nil {
		log.Fatalf("Can't run a listener: %s", err)
	}
}
