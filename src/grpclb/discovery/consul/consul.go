// Package consul implements grpclb service discovery via Consul (consul.io).
// Service names may be specified as: svcname,tag1,tag2,tag3
package consul

import (
	"fmt"
	"strings"

	"github.com/hashicorp/consul/api"
	"github.com/sipian/grpc-5G-SBA/grpclb/balancer"

	balancerpb "github.com/sipian/grpc-5G-SBA/grpclb/grpclb_balancer_v1"
)

type discovery struct {
	*api.Client
}

// New returns an implementation of balancer.Discovery interface.
func New(config *api.Config) (balancer.Discovery, error) {
	if config == nil {
		config = api.DefaultConfig()
	}

	c, err := api.NewClient(config)
	if err != nil {
		return nil, err
	}
	return &discovery{Client: c}, nil
}

// Resolve implements balancer.Discovery
func (d *discovery) Resolve(target string) ([]*balancerpb.Server, error) {
	service, tags := splitTarget(target)
	return d.query(service, tags)
}

func (d *discovery) query(service string, tags []string) ([]*balancerpb.Server, error) {
	var tag string
	if len(tags) > 0 {
		tag = tags[0]
	}

	// Query watcher
	// only get healthy endpoints
	entries, _, err := d.Health().Service(service, tag, true, nil)
	if err != nil {
		return nil, err
	}

	// If more than one tag is passed, we need to filter
	if len(tags) > 1 {
		entries = filterEntries(entries, tags[1:])
	}

	return parseEntries(entries), nil
}

// --------------------------------------------------------------------

func splitTarget(target string) (service string, tags []string) {
	parts := strings.Split(target, ",")
	service = parts[0]
	if len(parts) > 1 {
		tags = parts[1:]
	}
	return
}

func parseEntries(entries []*api.ServiceEntry) []*balancerpb.Server {
	res := make([]*balancerpb.Server, len(entries))
	for i, entry := range entries {
		addr := entry.Node.Address
		if entry.Service.Address != "" {
			addr = entry.Service.Address
		}
		res[i] = &balancerpb.Server{
			Address: fmt.Sprintf("%s:%d", addr, entry.Service.Port),
			Tags:    entry.Service.Tags,
		}
	}
	return res
}

func filterEntries(entries []*api.ServiceEntry, requiredTags []string) (res []*api.ServiceEntry) {
EntriesLoop:
	for _, entry := range entries {
		for _, required := range requiredTags {
			var found bool
			for _, tag := range entry.Service.Tags {
				if tag == required {
					found = true
					break
				}
			}
			if !found {
				continue EntriesLoop
			}
		}
		res = append(res, entry)
	}
	return
}
