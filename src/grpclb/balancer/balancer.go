package balancer

import (
	"math"
	"math/rand"
	"strconv"
	"strings"

	balancerpb "github.com/sipian/grpc-5G-SBA/grpclb/grpclb_balancer_v1"
)

// Balancer algorithm interface
type Balancer interface {
	Balance([]*balancerpb.Server) *balancerpb.Server
}

// BalancerFunc is a simple balancer function which implements the Balancer interface
type BalancerFunc func([]*balancerpb.Server) *balancerpb.Server

// Balance implements Balance
func (f BalancerFunc) Balance(s []*balancerpb.Server) *balancerpb.Server { return f(s) }

// --------------------------------------------------------------------

// NewRandomBalancer returns a balancer which returns a random known server
func NewRandomBalancer() Balancer {
	return BalancerFunc(func(s []*balancerpb.Server) *balancerpb.Server {
		if len(s) == 0 {
			return nil
		}
		return s[rand.Intn(len(s))]
	})
}

// type busyServers []*balancerpb.Server

// func (p busyServers) Len() int           { return len(p) }
// func (p busyServers) Less(i, j int) bool { return p[i].Tags[0] < p[j].Score }
// func (p busyServers) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }
// func (p busyServers) Shuffle() {
// 	for i := len(p) - 1; i > 0; i-- {
// 		j := rand.Intn(i + 1)
// 		p[i], p[j] = p[j], p[i]
// 	}
// }
// func (p busyServers) Sort() {
// 	sort.Sort(busyServers(p))
// }

// NewLeastBusyBalancer returns a balancer which returns the least busy server
// Tags Format : ["cpu:80.066","responseTime:453.04545"]
func NewLeastCPUBalancer() Balancer {
	return BalancerFunc(func(s []*balancerpb.Server) *balancerpb.Server {
		var minServer *balancerpb.Server
		var minCPU float64

		for _, server := range s {
			if len(server.Tags) == 0 {
				// servers with no tags have lesser priority
				if minServer == nil {
					minServer = server
					minCPU = math.MaxFloat64
				}
			} else {
				for _, tags := range server.Tags {
					if strings.Contains(tags, "cpu:") {
						if minServer == nil {
							if i, err := strconv.ParseFloat(tags[4:], 64); err == nil {
								minServer = server
								minCPU = i
							}
						} else {
							if i, err := strconv.ParseFloat(tags[4:], 64); err == nil && minCPU > i {
								minServer = server
								minCPU = i
							}
						}
						break
					}
				}
			}
		}
		return minServer
	})
}

func NewLeastResponseTimeBalancer() Balancer {
	return BalancerFunc(func(s []*balancerpb.Server) *balancerpb.Server {
		var minServer *balancerpb.Server
		var minResponseTime float64

		for _, server := range s {
			if len(server.Tags) == 0 {
				if minServer == nil {
					// servers with no tags have lesser priority
					minServer = server
					minResponseTime = math.MaxFloat64
				}
			} else {
				for _, tags := range server.Tags {
					if strings.Contains(tags, "responseTime:") {
						if minServer == nil {
							if i, err := strconv.ParseFloat(tags[13:], 64); err == nil {
								minServer = server
								minResponseTime = i
							}
						} else {
							if i, err := strconv.ParseFloat(tags[13:], 64); err == nil && minResponseTime > i {
								minServer = server
								minResponseTime = i
							}
						}
						break
					}
				}
			}

		}
		return minServer
	})
}
