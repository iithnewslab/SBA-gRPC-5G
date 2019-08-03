package main

import (
	"flag"
	"fmt"

	log "github.com/sirupsen/logrus"

	"net"
	"os"
	"path/filepath"

	"github.com/hashicorp/consul/api"
	"github.com/sipian/grpc-5G-SBA/grpclb/balancer"
	"github.com/sipian/grpc-5G-SBA/grpclb/discovery/consul"
	balancerpb "github.com/sipian/grpc-5G-SBA/grpclb/grpclb_balancer_v1"
	"google.golang.org/grpc"
)

var flags struct {
	addr    string
	consul  string
	version bool
}

var version string

func init() {
	flag.StringVar(&flags.addr, "addr", "0.0.0.0:8383", "Bind address. Default: :8383")
	flag.StringVar(&flags.consul, "consul", "127.0.0.1:8500", "Consul API address. Default: 127.0.0.1:8500")
	flag.BoolVar(&flags.version, "version", false, "Print version and exit")

	log.SetOutput(os.Stdout)
	log.SetLevel(log.InfoLevel)
}

func main() {
	flag.Parse()

	if flags.version {
		fmt.Println(filepath.Base(os.Args[0]), version)
		return
	}

	if err := listenAndServe(); err != nil {
		log.Fatal("FATAL", err.Error())
	}
}

func listenAndServe() error {
	config := api.DefaultConfig()
	config.Address = flags.consul

	discovery, err := consul.New(config)
	if err != nil {
		return err
	}

	lb := balancer.New(discovery, nil)
	defer lb.Reset()

	srv := grpc.NewServer()
	balancerpb.RegisterLoadBalancerServer(srv, lb)

	lis, err := net.Listen("tcp", flags.addr)
	if err != nil {
		return err
	}

	log.Info("Starting Server")

	return srv.Serve(lis)
}
