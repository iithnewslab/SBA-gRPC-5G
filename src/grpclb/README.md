# grpclb

Our loadbalancer is a modified version of [github.com/bsm/grpclb](https://github.com/bsm/grpclb).

External Load Balancing Service solution for gRPC written in Go. The approach follows the
[proposal](https://github.com/grpc/grpc/blob/master/doc/load-balancing.md) outlined by the
core gRPC team.

An example service discovery implementation is provided for [Consul](discovery/consul/).
