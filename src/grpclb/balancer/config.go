package balancer

import (
	"time"

	log "github.com/sirupsen/logrus"
)

// Config options
type Config struct {
	// Balancer chooses can decide whether to provide a complete list,
	// a subset, or a specific list of "picked" servers in a particular order.
	// Default: LeastBusyBalancer
	Balancer     Balancer
	IsRoundRobin bool

	Discovery struct {
		// Interval between service discovery checks
		// Default: 5s
		Interval time.Duration
	}
}

func (c *Config) norm() *Config {

	c.IsRoundRobin = false
	c.Balancer = NewLeastCPUBalancer()
	//c.Balancer = NewRandomBalancer()
	if c.Discovery.Interval == 0 {
		c.Discovery.Interval = 1 * time.Second
	}
	log.Infof("Config :: Balancer : NewLeastCPUBalancer, Discovery-Interval %v.", c.Discovery.Interval)
	//log.Infof("Config :: Balancer : NewRandomBalancer, Discovery-Interval %v.", c.Discovery.Interval)
	return c
}
