from golang:1.11

# Installing grpc
RUN apt-get update \
        && apt-get dist-upgrade -y \
        && apt-get install -y --no-install-recommends \
            build-essential \
            autoconf \
            ca-certificates \
            git \
            cmake \
            make \
            libtool \
            pkg-config \
            libboost-all-dev \
            libcurl4-openssl-dev \
            automake \
            libtool \
            shtool \
            && rm -rf /var/lib/apt/lists/*

RUN git clone -b v1.17.0 https://github.com/grpc/grpc /grpc
WORKDIR /grpc/
RUN git submodule update --init
RUN make -j 4
RUN make install

#Installing ppconsul
RUN git clone -b catalog-service-register https://github.com/sipian/ppconsul.git /ppconsul
RUN mkdir /ppconsul/workspace
WORKDIR /ppconsul/workspace
RUN cmake ..
RUN make -j 4
RUN make install

#Installing prometheus-cpp client libraries
RUN git clone -b v0.4.2 https://github.com/jupp0r/prometheus-cpp.git /prometheus-cpp
WORKDIR /prometheus-cpp
RUN  git submodule init
RUN git submodule update
RUN mkdir _build
WORKDIR /prometheus-cpp/_build
RUN cmake ..
RUN make -j 4
RUN make install

#Installing curlpp
RUN git clone -b v0.8.1 https://github.com/jpbarrette/curlpp /curlpp
WORKDIR /curlpp
RUN mkdir build
WORKDIR /curlpp/build
RUN cmake ..
RUN make -j 4
RUN make install

#Installing JSON
RUN git clone -b v1.1.0 https://github.com/Tencent/rapidjson /json
WORKDIR /json
RUN mkdir build
WORKDIR /json/build
RUN cmake ..
RUN make -j 4
RUN make install

#Installing protobuf
WORKDIR /
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-cpp-3.6.1.tar.gz
RUN tar -xvzf protobuf-cpp-3.6.1.tar.gz
WORKDIR /protobuf-3.6.1
RUN ./configure
RUN make -j 4
RUN make install

#Refresh shared library cache
RUN ldconfig

RUN mkdir /sba
WORKDIR /sba