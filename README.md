# 5G-Core-gRPC-SBA

The 5G core modules code is in the [src](src) folder.  
The loadbalancer code is in the [grpclb](src/grpclb) folder.

## How to run 5G modules


### Create Docker containers

```
git clone https://github.com/sipian/5G-Core-gRPC-SBA.git ~/ngcode

docker network create vepc

docker run -ti -d --rm --name ran --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name ausf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name amf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name smf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name upf --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash

docker run -ti -d --rm --name sink --cap-add NET_ADMIN --network vepc -v ~/ngcode/:/code btvk/sba-vnf /bin/bash
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
