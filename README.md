# 5G-Core-gRPC-SBA

This repository is the proof of concept for Service Based Architecture of 5G using gRPC. This work is published with the title **Prototyping and Load Balancing the Service Based Architecture of 5G Core using NFV**  in the 5th proceedings of IEEE Network Softwarization (NetSoft) 2019.  The paper can be accessed [here](AuthorCopySBA5G.pdf) with the presentation at this [link](Presentation_Netsoft19_gRPC_5G.pdf). 



The 5G core modules code is in the [src](src) folder.  
The loadbalancer code is in the [grpclb](src/grpclb) folder.

## How to run 5G modules


### Create Docker containers

```
git clone https://github.com/iithnewslab/SBA-gRPC-5G.git ~/ngcode

docker network create vepc

docker run -ti -d --rm --name ran --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name ausf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name amf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name smf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name upf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name sink --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash
```

### Start Consul Server for NRF

Download Consul pre-compiled binaries from [here](https://www.consul.io/docs/install/index.html). From host machine get the IP address of your interface on which Consul is bound and update the [code](https://github.com/iithnewslab/SBA-gRPC-5G/search?q=192.168.136.88&unscoped_q=192.168.136.88) with this IP address value. Run the following command in the host machine to start consul server.
```
consul agent -server -bootstrap -bind=<Interface-IP-Address> -client=<Interface-IP-Address> -ui -data-dir=/tmp/consul -node=consul-node
```

### Start the 5G Network Function modules

> Check your docker subnet for network vepc.
If the subnet is not 172.18.x.x, update your corresponding modules IP addresses in src files of  
Amf.cpp  
Ran.cpp   
Smf.cpp   
Upf.cpp  
Sink.cpp

Run the follow commands in every docker container shell

```
export LD_LIBRARY_PATH=/usr/local/lib/

cd code/src
```


#### Run in AUSF
```
sudo service mysql restart
make ausf.out
./ausf.out 10
# 10 denotes the num_of_ausf_threads for N_ausf interface
```

#### Run in SMF
```
make smf.out
./smf.out 10
# 10 denotes the num_of_smf_threads for N_smf and N4 interfaces
```

#### Run in AMF
```
make amf.out
./amf.out 10 
# 10 denotes the num_of_amf_threads for N_amf interface
```

#### Run in UPF
```
make upf.out
./upf.out 10 10 10
# 10 denotes the num_of_upf_threads for N3, N4 and N6 interface
```

#### Run in SINK
```
make sink.out
./settings.sh
./sink.out 10 
# 10 denotes the num_of_sink_server threads
```

#### Run in RAN
```
make ransim.out
./settings.sh
./ransim.out 10 10
# first 10 denotes the num_of_ue_threads at RAN
# second 10 denotes the time duration of each UE thread
```

To disable logging, set the DEBUG macro present in [utils.h](https://github.com/iithnewslab/SBA-gRPC-5G/blob/master/src/utils.h) to 0.

