package balancer

import (
	"sync"

	log "github.com/sirupsen/logrus"

	balancerpb "github.com/sipian/grpc-5G-SBA/grpclb/grpclb_balancer_v1"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
)

// Discovery describes a service discovery interface
type Discovery interface {
	// Resolve accepts a target string and returns a list of addresses
	Resolve(target string) ([]*balancerpb.Server, error)
}

// Server is a gRPC server
type Server struct {
	discovery Discovery
	config    *Config

	cache             map[string]*service
	roundRobinCounter map[string]int

	mu sync.RWMutex
}

// New creates a new Server instance with a given resolver
func New(discovery Discovery, config *Config) *Server {
	if config == nil {
		config = new(Config)
	}
	return &Server{
		discovery:         discovery,
		config:            config.norm(),
		cache:             make(map[string]*service),
		roundRobinCounter: make(map[string]int),
	}
}

// Servers implements RPC server
func (b *Server) Servers(ctx context.Context, req *balancerpb.ServersRequest) (*balancerpb.ServersResponse, error) {
	if req.Target == "" {
		return &balancerpb.ServersResponse{}, nil
	}

	servers, err := b.GetServers(req.Target)
	if err != nil {
		return nil, grpc.Errorf(codes.Internal, err.Error())
	}
	if servers == nil {
		return nil, grpc.Errorf(codes.Unknown, err.Error())
	}
	log.Infof("Sending Response for LB request %s :=> %s : %v", req.Target, servers.Address, servers.Tags)

	return &balancerpb.ServersResponse{Servers: servers}, nil
}

// GetServers retrieves all known servers for a target service
func (b *Server) GetServers(target string) (*balancerpb.Server, error) {
	// Retrieve from cache, if available
	b.mu.RLock()
	svc, cached := b.cache[target]
	b.mu.RUnlock()
	if cached {
		s := svc.Servers()
		if s == nil || len(s) == 0 {
			return nil, nil
		}
		if b.config.IsRoundRobin {
			b.mu.Lock()
			b.roundRobinCounter[target] = (b.roundRobinCounter[target] + 1) % len(s)
			x, _ := b.roundRobinCounter[target]
			b.mu.Unlock()
			return s[x], nil
		} else {
			return b.config.Balancer.Balance(s), nil
		}
	}

	// Create a new service connection
	newSvc, err := newService(target, b.discovery, b.config.Discovery.Interval)
	if err != nil {
		return nil, err
	}

	// Apply write lock
	b.mu.Lock()
	svc, cached = b.cache[target]
	if !cached {
		svc = newSvc
		b.cache[target] = svc
		b.roundRobinCounter[target] = 0
	}
	b.mu.Unlock()

	// Close new connection again, if svc was cached
	if cached {
		newSvc.Close()
	}

	s := svc.Servers()
	if s == nil || len(s) == 0 {
		return nil, nil
	}

	// Retrieve and filter/sort servers
	if b.config.IsRoundRobin {
		b.mu.Lock()
		b.roundRobinCounter[target] = (b.roundRobinCounter[target] + 1) % len(s)
		x, _ := b.roundRobinCounter[target]
		b.mu.Unlock()
		return s[x], nil
	} else {
		return b.config.Balancer.Balance(s), nil
	}
}

// Reset resets services cache
func (b *Server) Reset() {
	b.mu.Lock()
	services := make([]*service, 0, len(b.cache))
	for target, svc := range b.cache {
		services = append(services, svc)
		delete(b.cache, target)
	}
	b.mu.Unlock()

	for _, svc := range services {
		svc.Close()
	}
}
