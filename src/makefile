#---------------Final Target----------------#
#
# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -lgrpc++_reflection\
           -ldl 
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../protos

vpath %.proto $(PROTOS_PATH)



.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h greeter_client greeter_server greeter_async_client greeter_async_client2 greeter_async_server


# The following is to test your system and ensure a smoother experience.
# They are by no means necessary to actually compile a grpc-enabled software.

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif


all:  ransim.out sink.out upf.out ausf.out smf.out amf.out 

#-------------------------------------------#


#-----------------Compiler------------------#

G++ = g++ -std=c++0x -std=c++11 -std=gnu++0x -ggdb -lgrpc++ -lm -lcurl -lcurlpp -ljsoncpp 


#-------------------------------------------#


#---------------Object files----------------#

diameter.o:	diameter.cpp diameter.h utils.h
	$(G++) -c -o diameter.o diameter.cpp

gtp.o:	gtp.cpp gtp.h utils.h
	$(G++) -c -o gtp.o gtp.cpp

amf.o:	diameter.h gtp.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h utils.h amf.cpp amf.h
	$(G++) -c $(LDFLAGS) -o $@ -o amf.o amf.cpp 

amf_server.o:	diameter.h gtp.h amf.h amf_server.cpp amf_server.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h utils.h
	$(G++) -c $(LDFLAGS) -o $@ -o amf_server.o amf_server.cpp 

ausf.o:  diameter.h gtp.h ausf.cpp ausf.h mysql.h network.h packet.h s1ap.h sctp_server.h sync.h utils.h
	$(G++) -c  $(LDFLAGS)  -o $@ -o ausf.o ausf.cpp 

ausf_server.o:   diameter.h gtp.h ausf.h ausf_server.cpp ausf_server.h mysql.h network.h packet.h s1ap.h sctp_server.h sync.h utils.h
	$(G++) -c  $(LDFLAGS) -o $@ -o ausf_server.o ausf_server.cpp       


hss.o:	diameter.h gtp.h hss.cpp hss.h mysql.h network.h packet.h s1ap.h sctp_server.h sync.h utils.h
	$(G++) -c -o hss.o hss.cpp	

hss_server.o:	diameter.h gtp.h hss.h hss_server.cpp hss_server.h mysql.h network.h packet.h s1ap.h sctp_server.h sync.h utils.h
	$(G++) -c -o hss_server.o hss_server.cpp	

mme.o:	diameter.h gtp.h mme.cpp mme.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h utils.h
	$(G++) -c -o mme.o mme.cpp

mme_server.o:	diameter.h gtp.h mme.h mme_server.cpp mme_server.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h utils.h
	$(G++) -c -o mme_server.o mme_server.cpp	


smf.o:	diameter.h gtp.h smf.cpp smf.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h udp_server.h utils.h
	$(G++) -c  $(LDFLAGS)  -o $@ -o smf.o smf.cpp

smf_server.o:	diameter.h gtp.h smf.h smf_server.cpp smf_server.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h udp_server.h utils.h
	$(G++) -c  $(LDFLAGS)  -o $@ -o smf_server.o smf_server.cpp        

amqp.o:	diameter.h gtp.h amqp.cpp amqp.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h udp_server.h utils.h
	$(G++) -c -o amqp.o amqp.cpp

amqp_server.o:	diameter.h gtp.h amqp.h amqp_server.cpp amqp_server.h network.h packet.h s1ap.h sctp_client.h sctp_server.h security.h sync.h telecom.h udp_client.h udp_server.h utils.h
	$(G++) -c -o amqp_server.o amqp_server.cpp        





mysql.o:	mysql.cpp mysql.h utils.h
	$(G++) -c -o mysql.o mysql.cpp

network.o:	diameter.h gtp.h network.cpp network.h packet.h s1ap.h utils.h 
	$(G++) -c -o network.o network.cpp

packet.o:	diameter.h gtp.h packet.cpp packet.h s1ap.h utils.h
	$(G++) -c -o packet.o packet.cpp

pgw.o:	diameter.h gtp.h network.h packet.h pgw.cpp pgw.h s1ap.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o pgw.o pgw.cpp

pgw_server.o:	diameter.h gtp.h network.h packet.h pgw.h pgw_server.cpp pgw_server.h s1ap.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o pgw_server.o pgw_server.cpp

ran.o:	diameter.h gtp.h network.h packet.h ran.cpp ran.h s1ap.h sctp_client.h security.h sync.h telecom.h tun.h udp_client.h udp_server.h utils.h
	$(G++) -c -o ran.o ran.cpp

ran_simulator.o:	diameter.h gtp.h network.h packet.h ran.h ran_simulator.cpp ran_simulator.h s1ap.h sctp_server.h sctp_client.h security.h sync.h telecom.h tun.h udp_client.h udp_server.h utils.h
	$(G++) -c -o ran_simulator.o ran_simulator.cpp

s1ap.o:	s1ap.cpp s1ap.h utils.h
	$(G++) -c -o s1ap.o s1ap.cpp

sctp_client.o:	diameter.h gtp.h network.h packet.h s1ap.h sctp_client.cpp sctp_client.h utils.h
	$(G++) -c -o sctp_client.o sctp_client.cpp

sctp_server.o:	diameter.h gtp.h network.h packet.h s1ap.h sctp_server.cpp sctp_server.h sync.h utils.h
	$(G++) -c -o sctp_server.o sctp_server.cpp	

security.o:	diameter.h gtp.h packet.h s1ap.h security.cpp security.h utils.h
	$(G++) -c -o security.o security.cpp

sgw.o:	diameter.h gtp.h network.h packet.h s1ap.h sgw.cpp sgw.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o sgw.o sgw.cpp

sgw_server.o:	diameter.h gtp.h network.h packet.h s1ap.h sgw.h sgw_server.cpp sgw_server.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o sgw_server.o sgw_server.cpp

sink.o:	diameter.h gtp.h network.h packet.h s1ap.h sink.cpp sink.h tun.h udp_client.h udp_server.h utils.h
	$(G++) -c -o sink.o sink.cpp

sink_server.o:	diameter.h gtp.h network.h packet.h s1ap.h sink.h sink_server.cpp sink_server.h tun.h udp_client.h udp_server.h utils.h
	$(G++) -c -o sink_server.o sink_server.cpp	

sync.o:	sync.cpp sync.h utils.h
	$(G++) -c -o sync.o sync.cpp

telecom.o:	telecom.cpp telecom.h utils.h
	$(G++) -c -o telecom.o telecom.cpp

tun.o:	diameter.h gtp.h network.h packet.h s1ap.h tun.cpp tun.h utils.h
	$(G++) -c -o tun.o tun.cpp

udp_client.o:	diameter.h gtp.h network.h packet.h s1ap.h udp_client.cpp udp_client.h utils.h
	$(G++) -c -o udp_client.o udp_client.cpp

udp_server.o:	diameter.h gtp.h network.h packet.h s1ap.h udp_server.cpp udp_server.h utils.h
	$(G++) -c -o udp_server.o udp_server.cpp

utils.o:	utils.cpp utils.h
	$(G++) -c -o utils.o utils.cpp
upf.o:  diameter.h gtp.h network.h packet.h upf.cpp s1ap.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o upf.o upf.cpp

upf_server.o:   diameter.h gtp.h network.h packet.h upf_server.cpp upf_server.h upf.h s1ap.h sync.h udp_client.h udp_server.h utils.h
	$(G++) -c -o upf_server.o upf_server.cpp

#-------------------------------------------#



#---------Prerequisites and Recipes---------#

HSS_P = diameter.o gtp.o hss.o hss_server.o mysql.o network.o packet.o s1ap.o sctp_server.o sync.o utils.o
HSS_R = $(G++) -o hss.out $(HSS_P) -pthread `mysql_config --cflags --libs`




AUSF_P = ngcode.pb.o ngcode.grpc.pb.o diameter.o gtp.o ausf.o ausf_server.o mysql.o network.o packet.o s1ap.o sctp_server.o sync.o utils.o
AUSF_R = $(G++) -o ausf.out  $(AUSF_P)  -pthread -lppconsul  `mysql_config --cflags --libs` $(LDFLAGS) -o $@


AMF_P = ngcode.pb.o ngcode.grpc.pb.o diameter.o gtp.o amf.o amf_server.o network.o packet.o s1ap.o sctp_client.o sctp_server.o security.o sync.o telecom.o udp_client.o utils.o
AMF_R = $(G++) -o amf.out $(AMF_P) -pthread -lppconsul -lcurlpp -lcurl -ljsoncpp -lcrypto $(LDFLAGS) -o $@


MME_P = diameter.o gtp.o mme.o mme_server.o network.o packet.o s1ap.o sctp_client.o sctp_server.o security.o sync.o telecom.o udp_client.o utils.o
MME_R = $(G++) -o mme.out $(MME_P) -pthread -lcrypto


SMF_P = ngcode.pb.o ngcode.grpc.pb.o diameter.o gtp.o smf.o smf_server.o network.o packet.o s1ap.o sctp_client.o sctp_server.o security.o sync.o telecom.o udp_client.o udp_server.o utils.o
SMF_R = $(G++) -o smf.out $(SMF_P) -pthread -lppconsul -lcrypto $(LDFLAGS) -o $@

AMQP_P = diameter.o gtp.o amqp.o amqp_server.o network.o packet.o s1ap.o sctp_client.o sctp_server.o security.o sync.o telecom.o udp_client.o udp_server.o utils.o
AMQP_R = $(G++) -o amqp.out $(AMQP_P) -pthread -lcrypto




PGW_P = diameter.o gtp.o network.o packet.o pgw.o pgw_server.o s1ap.o sync.o udp_client.o udp_server.o utils.o
PGW_R = $(G++) -o pgw.out $(PGW_P) -pthread

RAN_P = diameter.o gtp.o network.o packet.o ran.o ran_simulator.o s1ap.o sctp_client.o sctp_server.o security.o sync.o telecom.o tun.o udp_client.o udp_server.o utils.o
RAN_R = $(G++) -o ransim.out $(RAN_P) -pthread -lcrypto

SGW_P = diameter.o gtp.o network.o packet.o s1ap.o sgw.o sgw_server.o sync.o udp_client.o udp_server.o utils.o
SGW_R = $(G++) -o sgw.out $(SGW_P) -pthread

SINK_P = diameter.o gtp.o network.o packet.o s1ap.o sink.o sink_server.o tun.o udp_client.o udp_server.o utils.o
SINK_R = $(G++) -o sink.out $(SINK_P) -pthread

UPF_P = diameter.o gtp.o network.o packet.o upf.o upf_server.o s1ap.o sync.o udp_client.o udp_server.o utils.o
UPF_R = $(G++) -o upf.out $(UPF_P) -pthread

#-------------------------------------------#


#---------------Executable files------------#

hss.out:	$(HSS_P)
	$(HSS_R)

ausf.out:	$(AUSF_P)
	$(AUSF_R)


mme.out:	$(MME_P)
	$(MME_R)

smf.out:	$(SMF_P)
	$(SMF_R)

amf.out:	$(AMF_P)
	$(AMF_R)
	
pgw.out:	$(PGW_P)
	$(PGW_R)

ransim.out:	$(RAN_P)
	$(RAN_R)

sgw.out:	$(SGW_P)
	$(SGW_R)

sink.out:	$(SINK_P)	
	$(SINK_R)


upf.out:        $(UPF_P)
	$(UPF_R)

amqp.out:	$(AMQP_P)
	$(AMQP_R)

#-------------------------------------------#


#------------------Cleaner------------------#

clean:
	rm -f *~ *.o *.out

#-------------------------------------------#


#--------------Special Commands-------------#

# make -k (To keep going on even after encountering errors in making a former target)

#-------------------------------------------#
