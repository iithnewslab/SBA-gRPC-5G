package balancer

import (
	"sync"
	"time"

	log "github.com/sirupsen/logrus"

	balancerpb "github.com/sipian/grpc-5G-SBA/grpclb/grpclb_balancer_v1"
	"google.golang.org/grpc/grpclog"
)

type service struct {
	target    string
	discovery Discovery
	endpoints []*balancerpb.Server

	mu sync.RWMutex

	closing, closed chan struct{}
}

func newService(target string, discovery Discovery, discoveryInterval time.Duration) (*service, error) {
	s := &service{
		target:    target,
		discovery: discovery,
		endpoints: make([]*balancerpb.Server, 0),

		closing: make(chan struct{}),
		closed:  make(chan struct{}),
	}
	if err := s.updateBackends(); err != nil {
		return nil, err
	}

	go s.loop(discoveryInterval)
	return s, nil
}

func (s *service) Servers() []*balancerpb.Server {
	s.mu.RLock()
	defer s.mu.RUnlock()

	return s.endpoints
}

func (s *service) Close() {
	close(s.closing)
	<-s.closed
}

func (s *service) loop(discoveryInterval time.Duration) {
	t := time.NewTicker(discoveryInterval)
	defer t.Stop()

	for {
		select {
		case <-s.closing:
			close(s.closed)
			return
		case <-t.C:
			err := s.updateBackends()
			if err != nil {
				grpclog.Printf("error on service discovery of %s: %s", s.target, err)
			}
		}
	}
}

func (s *service) updateBackends() error {
	endpoints, err := s.discovery.Resolve(s.target)
	if err != nil {
		return err
	}

	log.Debugf("Updating endpoints for %s : %v", s.target, endpoints)

	s.mu.Lock()
	s.endpoints = endpoints
	s.mu.Unlock()
	return nil
}
